/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>

#include <wrench/services/compute/cloud/CloudComputeService.h>
#include <wrench/wms/WMS.h>
#include <wrench/services/helper_services/standard_job_executor/StandardJobExecutorMessage.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeServiceOneShot.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/util/PointerUtil.h>
#include <wrench/util/TraceFileLoader.h>
#include <wrench/job/PilotJob.h>
#include "services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayer.h"
#include "batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.h"
#include "batch_schedulers/homegrown/conservative_bf/CONSERVATIVEBFBatchScheduler.h"
#include "batch_schedulers/homegrown/conservative_bf_core_level/CONSERVATIVEBFBatchSchedulerCoreLevel.h"
#include "batch_schedulers/batsched/BatschedBatchScheduler.h"
#include <wrench/failure_causes/FunctionalityNotAvailable.h>
#include <wrench/failure_causes/JobKilled.h>
#include <wrench/failure_causes/ServiceIsDown.h>
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/failure_causes/NotEnoughResources.h>
#include <wrench/failure_causes/JobTimeout.h>
#include <wrench/failure_causes/NotAllowed.h>


WRENCH_LOG_CATEGORY(wrench_core_batch_service, "Log category for Batch Service");

namespace wrench {
    // Do not remove
    BatchComputeService::~BatchComputeService() {}

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param compute_hosts: the list of names of the available compute hosts
     *                 - the hosts must be homogeneous (speed, number of cores, and RAM size)
     *                 - all cores are usable by the batch service on each host
     *                 - all RAM is usable by the batch service on each host
     * @param scratch_space_mount_point: the mount point of the scratch storage space for the service ("" means "no scratch space")
     * @param property_list: a property list that specifies BatchComputeServiceProperty values ({} means "use all defaults")
     * @param messagepayload_list: a message payload list that specifies BatchComputeServiceMessagePayload values ({} means "use all defaults")
     */
    BatchComputeService::BatchComputeService(const std::string &hostname,
                                             std::vector<std::string> compute_hosts,
                                             std::string scratch_space_mount_point,
                                             std::map<std::string, std::string> property_list,
                                             std::map<std::string, double> messagepayload_list
    ) : BatchComputeService(hostname, std::move(compute_hosts), ComputeService::ALL_CORES,
                            ComputeService::ALL_RAM, scratch_space_mount_point, std::move(property_list),
                            std::move(messagepayload_list), "") {}

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param compute_hosts: the list of names of the available compute hosts
     * @param cores_per_host: number of cores used per host
     *              - ComputeService::ALL_CORES to use all cores
     * @param ram_per_host: RAM per host (
     *              - ComputeService::ALL_RAM to use all RAM
     * @param scratch_space_mount_point: the mount point og the scratch storage space for the service ("" means "no scratch space")
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     *
     * @throw std::invalid_argument
     */
    BatchComputeService::BatchComputeService(const std::string hostname,
                                             std::vector<std::string> compute_hosts,
                                             unsigned long cores_per_host,
                                             double ram_per_host,
                                             std::string scratch_space_mount_point,
                                             std::map<std::string, std::string> property_list,
                                             std::map<std::string, double> messagepayload_list,
                                             std::string suffix) :
            ComputeService(hostname,
                           "batch" + suffix,
                           "batch" + suffix,
                           scratch_space_mount_point) {
        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        // Basic checks
        if (compute_hosts.empty()) {
            throw std::invalid_argument(
                    "BatchComputeService::BatchComputeService(): at least one compute hosts must be provided");
        }

        // Check Platform homogeneity
        double num_cores_available = Simulation::getHostNumCores(*(compute_hosts.begin()));
        double speed = Simulation::getHostFlopRate(*(compute_hosts.begin()));
        double ram_available = Simulation::getHostMemoryCapacity(*(compute_hosts.begin()));

        for (auto const &h : compute_hosts) {
            // Compute speed
            if (std::abs(speed - Simulation::getHostFlopRate(h)) > DBL_EPSILON) {
                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a batch service need "
                        "to be homogeneous (different flop rates detected)");
            }
            // RAM
            if (std::abs(ram_available - Simulation::getHostMemoryCapacity(h)) > DBL_EPSILON) {
                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a batch service need "
                        "to be homogeneous (different RAM capacities detected)");
            }
            // Num cores
            if (Simulation::getHostNumCores(h) != num_cores_available) {
                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a batch service need "
                        "to be homogeneous (different RAM capacities detected)");
            }
        }

        //create a map for host to cores
        int i = 0;
        for (auto h : compute_hosts) {
            this->nodes_to_cores_map.insert({h, num_cores_available});
            this->available_nodes_to_cores.insert({h, num_cores_available});
            this->host_id_to_names[i++] = h;
        }
        this->compute_hosts = compute_hosts;

        this->num_cores_per_node = this->nodes_to_cores_map.begin()->second;
        this->total_num_of_nodes = compute_hosts.size();

        // Check that the workload file is valid
        std::string workload_file = this->getPropertyValueAsString(
                BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE);
        if (not workload_file.empty()) {
            try {
                this->workload_trace = TraceFileLoader::loadFromTraceFile(
                        workload_file,
                        this->getPropertyValueAsBoolean(
                                BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE),
                        this->getPropertyValueAsDouble(
                                BatchComputeServiceProperty::SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE));
            } catch (std::exception &e) {
                throw;
            }
            // Fix value
            for (auto &j : this->workload_trace) {
                std::string id = std::get<0>(j);
                double submit_time = std::get<1>(j);
                double flops = std::get<2>(j);
                double requested_flops = std::get<3>(j);
                double requested_ram = std::get<4>(j);
                unsigned int num_nodes = std::get<5>(j);

                // Capping to max number of nodes, silently
                if (num_nodes > this->total_num_of_nodes) {
                    std::get<5>(j) = this->total_num_of_nodes;
                }
                // Capping to ram, silently
                if (requested_ram > ram_available) {
                    std::get<4>(j) = ram_available;
                }
            }
        }

        // Create a scheduler
#ifdef ENABLE_BATSCHED
        this->scheduler = std::unique_ptr<BatchScheduler>(new BatschedBatchScheduler(this));
#else
        auto batch_scheduling_alg = this->getPropertyValueAsString(
                BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM);
        if (this->scheduling_algorithms.find(batch_scheduling_alg) == this->scheduling_algorithms.end()) {
            throw std::invalid_argument(
                    " BatchComputeService::BatchComputeService(): unsupported scheduling algorithm " +
                    batch_scheduling_alg);
        }

        if (batch_scheduling_alg == "fcfs") {
            this->scheduler = std::unique_ptr<BatchScheduler>(new FCFSBatchScheduler(this));
        } else if (batch_scheduling_alg == "conservative_bf") {
            this->scheduler = std::unique_ptr<BatchScheduler>(new CONSERVATIVEBFBatchScheduler(this));
        } else if (batch_scheduling_alg == "conservative_bf_core_level") {
            this->scheduler = std::unique_ptr<BatchScheduler>(new CONSERVATIVEBFBatchSchedulerCoreLevel(this));
        }
#endif

        // Initialize it
        this->scheduler->init();
    }

    /**
     * @brief Retrieve start time estimates for a set of job configurations
     *
     * @param set_of_jobs: the set of job configurations, each of them with an id. Each configuration
     *         is a tuple as follows:
     *             - a configuration id (std::string)
     *             - a number of hosts (unsigned long)
     *             - a number of cores per host (unsigned long)
     *             - a duration in seconds (double)
     *
     * @return start date predictions in seconds (as a map of ids). A prediction that's negative
     *         means that the job configuration can not run on the service (e.g., not enough hosts,
     *         not enough cores per host)
     */
    std::map<std::string, double> BatchComputeService::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) {
        try {
            auto estimates = this->scheduler->getStartTimeEstimates(set_of_jobs);
            return estimates;
        } catch (ExecutionException &e) {
            throw;
        } catch (std::exception &e) {
            throw ExecutionException(std::shared_ptr<FunctionalityNotAvailable>(
                    new FunctionalityNotAvailable(
                            this->getSharedPtr<BatchComputeService>(), "start time estimates")));
        }
    }

    /**
    * @brief Gets the state of the batch queue
    * @return A vector of tuples:
    *              - std::string: username
    *              - string: job name
    *              - int: num hosts
    *              - int: num cores per host
    *              - int: time in seconds
    *              - double: submit time
    *              - double: start time (-1.0 if not started yet)
    */
    std::vector<std::tuple<std::string, std::string, int, int, int, double, double>> BatchComputeService::getQueue() {
        // Go through the currently running jobs
        std::vector<std::tuple<std::string, std::string, int, int, int, double, double>> queue_state;
        for (auto const &j : this->running_jobs) {
            auto tuple = std::make_tuple(
                    j->getUsername(),
                    j->getCompoundJob()->getName(),
                    j->getRequestedNumNodes(),
                    j->getRequestedCoresPerNode(),
                    j->getRequestedTime(),
                    j->getArrivalTimestamp(),
                    j->getBeginTimestamp()
            );
            queue_state.push_back(tuple);
        }

        //  Go through the waiting jobs (BATSCHED only)
        for (auto const &j : this->waiting_jobs) {
            auto tuple = std::make_tuple(
                    j->getUsername(),
                    j->getCompoundJob()->getName(),
                    j->getRequestedNumNodes(),
                    j->getRequestedCoresPerNode(),
                    j->getRequestedTime(),
                    j->getArrivalTimestamp(),
                    -1.0
            );
            queue_state.push_back(tuple);
        }

        // Go through the pending jobs
        for (auto const &j : this->batch_queue) {
            auto tuple = std::make_tuple(
                    j->getUsername(),
                    j->getCompoundJob()->getName(),
                    j->getRequestedNumNodes(),
                    j->getRequestedCoresPerNode(),
                    j->getRequestedTime(),
                    j->getArrivalTimestamp(),
                    -1.0
            );
            queue_state.push_back(tuple);
        }

        // Sort all jobs by  arrival  time
        std::sort(queue_state.begin(), queue_state.end(),
                  [](const std::tuple<std::string, std::string, int, int, int, double, double> j1,
                     const std::tuple<std::string, std::string, int, int, int, double, double> j2) -> bool {
                      if (std::get<6>(j1) == std::get<6>(j2)) {
                          return (std::get<1>(j1) > std::get<1>(j2));
                      } else {
                          return (std::get<6>(j1) > std::get<6>(j2));
                      }
                  });

        return queue_state;
    }

    /**
     * @brief Helper function for service-specific job arguments
     * @param key: the argument key ("-N", "-c", "-t")
     * @param args: the argument map
     * @return the value of the argument
     * @throw std::invalid_argument
     */
    unsigned long BatchComputeService::parseUnsignedLongServiceSpecificArgument(
            std::string key,
            const std::map<std::string, std::string> &args) {
        unsigned long value = 0;
        auto it = args.find(key);
        if (it != args.end()) {
            if (sscanf((*it).second.c_str(), "%lu", &value) != 1) {
                throw std::invalid_argument(
                        "BatchComputeService::parseUnsignedLongServiceSpecificArgument(): Invalid " + key + " value '" +
                        (*it).second + "'");
            }
        } else {
            throw std::invalid_argument(
                    "BatchComputeService::parseUnsignedLongServiceSpecificArgument(): Batch Service requires " + key +
                    " argument to be specified for job submission"
            );
        }
        return value;
    }

    /**
     * @brief Method to submit a job to the service
     * @param job
     * @param batch_job_args
     */
    void BatchComputeService::submitCompoundJob(std::shared_ptr<CompoundJob> job,
                                                const std::map<std::string, std::string> &batch_job_args) {
        assertServiceIsUp();

        // Get all arguments
        unsigned long num_hosts = 0;
        unsigned long num_cores_per_host = 0;
        unsigned long time_asked_for_in_minutes = 0;
        try {
            num_hosts = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-N", batch_job_args);
            num_cores_per_host = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-c", batch_job_args);
            time_asked_for_in_minutes = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-t",
                                                                                                      batch_job_args);
        } catch (std::invalid_argument &e) {
            throw;
        }

        std::string username = "you";
        if (batch_job_args.find("-u") != batch_job_args.end()) {
            username = batch_job_args.at("-u");
        }

        // Sanity check
        if ((num_hosts == 0) or (num_cores_per_host == 0) or (time_asked_for_in_minutes == 0)) {
            throw std::invalid_argument(
                    "BatchComputeService::submitCompoundJob(): service-specific arguments should have non-zero values");
        }

        // Create a Batch Job
        unsigned long jobid = this->generateUniqueJobID();
        auto batch_job = std::shared_ptr<BatchJob>(new BatchJob(job, jobid, time_asked_for_in_minutes,
                                                                num_hosts, num_cores_per_host, username, -1,
                                                                S4U_Simulation::getClock()));

        // Set job display color for csv output
        auto it = batch_job_args.find("-color");
        if (it != batch_job_args.end()) {
            batch_job->csv_metadata = "color:" + (*it).second;
        }

        // Send a "run a batch job" message to the daemon's mailbox_name
        auto answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_standard_job_mailbox");
        try {
            S4U_Mailbox::dputMessage(
                    this->mailbox_name,
                    new BatchComputeServiceJobRequestMessage(
                            answer_mailbox, batch_job,
                            this->getMessagePayloadValue(
                                    BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        // Get the answer
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }
        auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitCompoundJobAnswerMessage>(message);
        if (!msg) {
            throw std::runtime_error(
                    "BatchComputeService::submitCompoundJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
        if (!msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
    }



    /**
     * @brief Helper function called by terminateStandardJob() and terminatePilotJob() to process a job submission
     * @param job
     */
    void BatchComputeService::terminateCompoundJob(std::shared_ptr<CompoundJob> job) {
        assertServiceIsUp();

        auto answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_compound_job");

        // Send a "terminate a  job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new ComputeServiceTerminateCompoundJobRequestMessage(
                            answer_mailbox,
                            job,
                            this->getMessagePayloadValue(
                                    BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        auto msg = dynamic_cast<ComputeServiceTerminateCompoundJobAnswerMessage *>(message.get());

        if (not msg) {
            throw std::runtime_error("BatchComputeService::terminateCompoundJob(): Received an unexpected [" +
                                     message->getName() +
                                     "] message!");
        }

        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
    }



    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BatchComputeService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("Batch Service starting");

        this->scheduler->launch();

        // Start the workload trace replayer if needed
        if (not this->workload_trace.empty()) {
            try {
                startBackgroundWorkloadProcess();
            } catch (std::runtime_error &e) {
                throw;
            }
        }

        /** Main loop **/
        while (processNextMessage()) {
            this->scheduler->processQueuedJobs();
        }

        return 0;
    }

    /**
     * @brief Send back notification that a pilot job has expired
     * @param job
     */
    void BatchComputeService::sendPilotJobExpirationNotification(std::shared_ptr<PilotJob> job) {
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(
                                         job, this->getSharedPtr<BatchComputeService>(),
                                         this->getMessagePayloadValue(
                                                 BatchComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Send back notification that a standard job has failed
     * @param job
     */
    void BatchComputeService::sendCompoundJobFailureNotification(std::shared_ptr<CompoundJob> job, std::string job_id,
                                                                 std::shared_ptr<FailureCause> cause) {
        WRENCH_INFO("A standard job executor has failed because of timeout %s", job->getName().c_str());

        // TODO: Replace my a map
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto const &j : this->all_jobs) {
            if (j->getCompoundJob() == job) {
                batch_job = j;
                break;
            }
        }

        this->scheduler->processJobFailure(batch_job);

        job->printCallbackMailboxStack();
        try {
            S4U_Mailbox::putMessage(
                    job->popCallbackMailbox(),
                    new ComputeServiceCompoundJobFailedMessage(
                            job, this->getSharedPtr<BatchComputeService>(),
                            this->getMessagePayloadValue(
                                    BatchComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            return; // ignore
        }
    }

    /**
     * @brief Increase resource availabilities based on freed resources
     * @param resources: a set of tuples as follows:
     *              - hostname (string)
     *              - number of cores (unsigned long)
     *              - bytes of RAM (double)
     */
    void BatchComputeService::freeUpResources(std::map<std::string, std::tuple<unsigned long, double>> resources) {
        for (auto r : resources) {
            this->available_nodes_to_cores[r.first] += std::get<0>(r.second);
        }
    }

    /**
     * @brief ...
     * @param job
     */
    void BatchComputeService::removeJobFromRunningList(std::shared_ptr<BatchJob> job) {
        if (this->running_jobs.find(job) == this->running_jobs.end()) {
            throw std::runtime_error("BatchComputeService::removeJobFromRunningList(): Cannot find job!");
        }
        this->running_jobs.erase(job);
    }

    /**
     *
     * @param job
     */
    void BatchComputeService::removeBatchJobFromJobsList(std::shared_ptr<BatchJob> job) {
        if (job == nullptr) {
            return;
        }

        // TODO: Replace this by some map?
        for (auto const &j: this->all_jobs) {
            if (j->getCompoundJob() == job->getCompoundJob()) {
                this->all_jobs.erase(j);
                break;
            }
        }
    }

//    /**
//     *
//     * @param job
//     */
//    void BatchComputeService::processPilotJobTimeout(std::shared_ptr<PilotJob> job) {
//        auto cs = job->getComputeService();
//        if (cs == nullptr) {
//            throw std::runtime_error(
//                    "BatchComputeService::terminate(): can't find compute service associated to pilot job");
//        }
//        try {
//            cs->stop(false);
//        } catch (wrench::ExecutionException &e) {}
//    }

    /**
     *
     * @param job
     */
    void BatchComputeService::processCompoundJobTimeout(std::shared_ptr<CompoundJob> job) {
        // TODO: Do a map?
        for (auto it = this->running_bare_metal_one_shot_compute_services.begin();
             it != this->running_bare_metal_one_shot_compute_services.end(); it++) {
            if ((*it)->job == job) {
                // I'll get a bunch fo failure notifications, which is fine
                (*it)->stop(true, ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT);
//                this->running_bare_metal_one_shot_compute_services.erase(it);
                break;
            }
        }
    }

    /**
    * @brief terminate a running standard job
    * @param job: the job
    */
    void BatchComputeService::terminateRunningCompoundJob(std::shared_ptr<CompoundJob> job, ComputeService::TerminationCause termination_cause) {
        BareMetalComputeServiceOneShot *executor = nullptr;
        std::set<std::shared_ptr<BareMetalComputeServiceOneShot>>::iterator it;
        for (it = this->running_bare_metal_one_shot_compute_services.begin();
             it != this->running_bare_metal_one_shot_compute_services.end(); it++) {
            if (((*it))->job == job) {
                executor = (it->get());
            }
        }
        if (executor == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::terminateCurrentCompoundJob(): Cannot find one-shot bare-metal compute service corresponding to job being terminated");
        }

        // Terminate the executor
        WRENCH_INFO("Terminating a one-shot bare-metal service");
        executor->stop(false, termination_cause); // failure notifications sent by me later, if needed
    }

    /**
    * @brief Declare all current jobs as failed (likely because the daemon is being terminated
    * or has timed out (because it's in fact a pilot job))
    */
    void BatchComputeService::terminate(bool send_failure_notifications, ComputeService::TerminationCause termination_cause) {

        // LOCK
        this->acquireDaemonLock();

        WRENCH_INFO("Terminating all current compound jobs");

        {
            std::vector<std::shared_ptr<BatchJob>> to_erase;
            for (auto const &j : this->running_jobs) {
                auto compound_job = j->getCompoundJob();
                terminateRunningCompoundJob(compound_job, termination_cause);
                // Popping, because I am terminating it, so the executor won't pop, and right now
                // if I am at the top of the stack!
                if (compound_job->getCallbackMailbox() == this->mailbox_name) {
                    compound_job->popCallbackMailbox();
                }
                if (send_failure_notifications) {
                    WRENCH_INFO("SENDING FAILURE NOTIFICATION");
                    this->sendCompoundJobFailureNotification(
                            compound_job, std::to_string(j->getJobID()),
                            std::shared_ptr<FailureCause>(
                                    new JobKilled(compound_job)));
                }
                to_erase.push_back(j);
            }

            for (auto const &j : to_erase) {
                this->running_jobs.erase(j);
                this->removeBatchJobFromJobsList(j);
            }
            to_erase.clear();
        }

        {
            std::vector<std::deque<std::shared_ptr<BatchJob>>::iterator> to_erase;

            // TODO: Do in a simple while loop that removes as it goes
            for (auto it1 = this->batch_queue.begin(); it1 != this->batch_queue.end(); it1++) {
                auto compound_job = (*it1)->getCompoundJob();
                WRENCH_INFO("SIMPLY REMOVING COMPOUNG JOB %s FROM PENDING LIST", compound_job->getName().c_str());
                std::shared_ptr<FailureCause> failure_cause;
                switch (termination_cause) {
                    case ComputeService::TerminationCause::TERMINATION_JOB_KILLED:
                        failure_cause = std::make_shared<JobKilled>(compound_job);
                        break;
                    case ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED:
                        failure_cause = std::make_shared<ServiceIsDown>(this->getSharedPtr<ComputeService>());
                        break;
                    case ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT:
                        failure_cause = std::make_shared<JobTimeout>(compound_job);
                        break;
                    default:
                        failure_cause = std::make_shared<JobKilled>(compound_job);
                        break;
                }
                for (auto const &action : compound_job->getActions()) {
                    action->setFailureCause(failure_cause);
                }
                to_erase.push_back(it1);
                if (send_failure_notifications) {
                    WRENCH_INFO("SENDING FAILURE NOTIFICATION");
                    this->sendCompoundJobFailureNotification(
                            compound_job, std::to_string((*it1)->getJobID()),
                            std::shared_ptr<FailureCause>(
                                    new JobKilled(compound_job)));
                }
            }

            for (auto const &j : to_erase) {
                this->batch_queue.erase(j);
                this->removeBatchJobFromJobsList(*j);
            }
            to_erase.clear();
        }

        {
            std::vector<std::shared_ptr<BatchJob>> to_erase;

            for (auto const &wj : this->waiting_jobs) {
                auto compound_job = wj->getCompoundJob();
                to_erase.push_back(wj);
                WRENCH_INFO("SIMPLY REMOVING COMPOUNG JOB %s FROM WAITING LIST", compound_job->getName().c_str());

                std::shared_ptr<FailureCause> failure_cause;
                switch (termination_cause) {
                    case ComputeService::TerminationCause::TERMINATION_JOB_KILLED:
                        failure_cause = std::make_shared<JobKilled>(compound_job);
                        break;
                    case ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED:
                        failure_cause = std::make_shared<ServiceIsDown>(this->getSharedPtr<ComputeService>());
                        break;
                    case ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT:
                        failure_cause = std::make_shared<JobTimeout>(compound_job);
                        break;
                    default:
                        failure_cause = std::make_shared<JobKilled>(compound_job);
                        break;
                }
                for (auto const &action : compound_job->getActions()) {
                    action->setFailureCause(failure_cause);
                }

                if (send_failure_notifications) {
                    this->sendCompoundJobFailureNotification(
                            compound_job, std::to_string(wj->getJobID()),
                            std::shared_ptr<FailureCause>(
                                    new JobKilled(compound_job)));
                }
            }

            for (auto const &j : to_erase) {
                this->waiting_jobs.erase(j);
                this->removeBatchJobFromJobsList(j);
            }
            to_erase.clear();
        }

        // UNLOCK
        this->releaseDaemonLock();
    }

    /**
     * @brief Cleanup method
     *
     * @param has_returned_from_main: whether main() returned
     * @param return_value: the return value (if main() returned)
     */
    void BatchComputeService::cleanup(bool has_returned_from_main, int return_value) {
        this->scheduler->shutdown();

        // Do the default behavior (which will throw as this is not a fault-tolerant service)
        Service::cleanup(has_returned_from_main, return_value);
    }

//    /**
//     * @brief Terminate all running pilot jobs
//     */
//    void BatchComputeService::terminateRunningPilotJobs() {
//        if (getPropertyValueAsBoolean(BatchComputeServiceProperty::SUPPORTS_PILOT_JOBS)) {
//            WRENCH_INFO(
//                    "Failing running pilot jobs");
//            std::vector<std::shared_ptr<BatchJob>> to_erase;
//
//            // LOCK
//            this->acquireDaemonLock();
//
//            // Stopping services
//            for (auto &job : this->running_jobs) {
//                if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job->getCompoundJob())) {
//                    auto cs = pjob->getComputeService();
//                    if (cs == nullptr) {
//                        throw std::runtime_error(
//                                "BatchComputeService::terminate(): can't find compute service associated to pilot job");
//                    }
//                    try {
//                        cs->stop(false);
//                    } catch (wrench::ExecutionException &e) {
//                        // ignore
//                    }
//                    to_erase.push_back(job);
//                }
//            }
//
//            // Cleaning up data structures
//            for (auto &job : to_erase) {
//                this->running_jobs.erase(job);
//                this->removeBatchJobFromJobsList(job);
//            }
//
//            // UNLOCK
//            this->releaseDaemonLock();
//        }
//    }

    /**
     * @brief Wait for and procress the next message
     * @return true if the service should keep going, false otherwise
     */
    bool BatchComputeService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) { WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
            return false;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->setStateToDown();
            this->terminate(msg->send_failure_notifications, (ComputeService::TerminationCause)(msg->termination_cause)); // TODO: this cast is ugly
//            this->terminateRunningPilotJobs();

            // Send back a synchronous reply!
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                BatchComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
            processGetResourceInformation(msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<BatchComputeServiceJobRequestMessage *>(message.get())) {
            processJobSubmission(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceCompoundJobDoneMessage *>(message.get())) {
            processCompoundJobCompletion(std::dynamic_pointer_cast<BareMetalComputeServiceOneShot>(msg->compute_service), msg->job);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceCompoundJobFailedMessage *>(message.get())) {
            processCompoundJobFailure(std::dynamic_pointer_cast<BareMetalComputeServiceOneShot>(msg->compute_service), msg->job, nullptr);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceTerminateCompoundJobRequestMessage *>(message.get())) {
            processCompoundJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
            processPilotJobCompletion(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {
            processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<AlarmJobTimeOutMessage *>(message.get())) {
            processAlarmJobTimeout(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<BatchExecuteJobFromBatSchedMessage *>(message.get())) {
            processExecuteJobFromBatSched(msg->batsched_decision_reply);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage *>(message.get())) {
            processIsThereAtLeastOneHostWithAvailableResources(msg->answer_mailbox, msg->num_cores, msg->ram);
            return true;

        } else {
            throw std::runtime_error(
                    "BatchComputeService::processNextMessage(): Unexpected [" + message->getName() + "] message");
            return false;
        }
    }

    /**
     * @brief Process a job submission
     *
     * @param job: the batch job object
     * @param answer_mailbox: the mailbox to which answer messages should be sent
     */
    void BatchComputeService::processJobSubmission(std::shared_ptr<BatchJob> job, std::string answer_mailbox) {
        WRENCH_INFO("Asked to run a batch job with id %ld", job->getJobID());


        // Check that the job can be admitted in terms of resources:
        //      - number of nodes,
        //      - number of cores per host
        //      - RAM (only for standard jobs)
        unsigned long requested_hosts = job->getRequestedNumNodes();
        unsigned long requested_num_cores_per_host = job->getRequestedCoresPerNode();

        double required_ram_per_host = job->getMemoryRequirement();

        if ((requested_hosts > this->available_nodes_to_cores.size()) or
            (requested_num_cores_per_host >
             Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first)) or
            (required_ram_per_host >
             Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first))) {
            {
                S4U_Mailbox::dputMessage(
                        answer_mailbox,
                        new ComputeServiceSubmitCompoundJobAnswerMessage(
                                job->getCompoundJob(),
                                this->getSharedPtr<BatchComputeService>(),
                                false,
                                std::shared_ptr<FailureCause>(
                                        new NotEnoughResources(
                                                job->getCompoundJob(),
                                                this->getSharedPtr<BatchComputeService>())),
                                this->getMessagePayloadValue(
                                        BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
                return;
            }
        }


        // SUCCESS!
        S4U_Mailbox::dputMessage(answer_mailbox,
                                 new ComputeServiceSubmitCompoundJobAnswerMessage(
                                         job->getCompoundJob(),
                                         this->getSharedPtr<BatchComputeService>(),
                                         true,
                                         nullptr,
                                         this->getMessagePayloadValue(
                                                 BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));

        // Add the RJMS delay to the job's requested time
        job->setRequestedTime(job->getRequestedTime() +
                              this->getPropertyValueAsUnsignedLong(
                                      BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY));
        this->all_jobs.insert(job);
        this->batch_queue.push_back(job);

        this->scheduler->processJobSubmission(job);
    }

//    /**
//     * @brief Process a pilot job completion
//     *
//     * @param job: the pilot job
//     */
//    void BatchComputeService::processPilotJobCompletion(std::shared_ptr<PilotJob> job) {
//        // Remove the job from the running job list
//        std::shared_ptr<BatchJob> batch_job = nullptr;
//        for (auto const &rj : this->running_jobs) {
//            if (rj->getCompoundJob() == job) {
//                batch_job = rj;
//                break;
//            }
//        }
//
//        if (batch_job == nullptr) {
//            throw std::runtime_error(
//                    "BatchComputeService::processPilotJobCompletion():  Pilot job completion message received "
//                    "but no such pilot jobs found in queue"
//            );
//        }
//
//        this->removeJobFromRunningList(batch_job);
//        this->freeUpResources(batch_job->getResourcesAllocated());
//        if (this->pilot_job_alarms[job->getName()] != nullptr) {
//            this->pilot_job_alarms[job->getName()]->kill();
//            this->pilot_job_alarms.erase(job->getName());
//        }
//
//        // Let the scheduler know about the job completion
//        this->scheduler->processJobCompletion(batch_job);
//
//        // Forward the notification
//        try {
//            this->sendPilotJobExpirationNotification(job);
//        } catch (std::runtime_error &e) {}
//    }

//    /**
//     * @brief Process a pilot job termination request
//     *
//     * @param job: the job to terminate
//     * @param answer_mailbox: the mailbox to which the answer message should be sent
//     */
//    void BatchComputeService::processPilotJobTerminationRequest(
//            std::shared_ptr<PilotJob> job, std::string answer_mailbox) {
//        std::string job_id;
//        for (auto it = this->batch_queue.begin(); it != this->batch_queue.end(); it++) {
//            if ((*it)->getCompoundJob() == job) {
//                auto *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
//                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
//                        this->getMessagePayloadValue(
//                                BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
//                S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
//
//                // notify scheduler of job termination
//                this->scheduler->processJobTermination(*it);
//
//                std::shared_ptr<BatchJob> to_erase = *it;
//                this->batch_queue.erase(it);
//                this->removeBatchJobFromJobsList(to_erase);
//                return;
//            }
//        }
//
//        for (auto it2 = this->waiting_jobs.begin(); it2 != this->waiting_jobs.end(); it2++) {
//            if ((*it2)->getCompoundJob() == job) {
//                auto answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
//                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
//                        this->getMessagePayloadValue(
//                                BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
//                S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
//                // forward this notification to batsched
//                this->scheduler->processJobTermination(*it2);
//                auto to_erase = *it2;
//                this->waiting_jobs.erase(it2);
//                this->removeBatchJobFromJobsList(to_erase);
//                return;
//            }
//        }
//
//        for (auto it1 = this->running_jobs.begin(); it1 != this->running_jobs.end();) {
//            if ((*it1)->getCompoundJob() == job) {
//                this->processPilotJobTimeout(std::dynamic_pointer_cast<PilotJob>((*it1)->getCompoundJob()));
//                // Update the cores count in the available resources
//                std::map<std::string, std::tuple<unsigned long, double>> resources = (*it1)->getResourcesAllocated();
//                for (auto r : resources) {
//                    this->available_nodes_to_cores[r.first] += std::get<0>(r.second);
//                }
//                auto answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
//                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
//                        this->getMessagePayloadValue(
//                                BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
//                S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
//
//
//                this->scheduler->processJobTermination(*it1);
//
//                //this is the list of unique pointers
//                auto to_erase = *it1;
//                this->running_jobs.erase(it1);
//                this->removeBatchJobFromJobsList(to_erase);
//                return;
//            } else {
//                ++it1;
//            }
//        }
//
//        // If we got here, we're in trouble
//        WRENCH_INFO("Trying to terminate a pilot job that's neither pending nor running!");
//        std::string msg = "Job cannot be terminated because it's neither pending nor running";
//        auto answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
//                job, this->getSharedPtr<BatchComputeService>(), false,
//                std::shared_ptr<FailureCause>(new NotAllowed(this->getSharedPtr<BatchComputeService>(), msg)),
//                this->getMessagePayloadValue(
//                        BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
//        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
//    }

    /**
     * @brief Process a standard job completion
     * @param executor: the standard job executor
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void BatchComputeService::processCompoundJobCompletion(
            std::shared_ptr<BareMetalComputeServiceOneShot> executor,
            std::shared_ptr<CompoundJob> job) {
        bool executor_on_the_list = false;
        std::set<std::shared_ptr<BareMetalComputeServiceOneShot>>::iterator it;

        for (it = this->running_bare_metal_one_shot_compute_services.begin();
             it != this->running_bare_metal_one_shot_compute_services.end(); it++) {
            if (*it == executor) {
                PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_bare_metal_one_shot_compute_services),
                                                       &(this->finished_bare_metal_one_shot_compute_services));
                executor_on_the_list = true;
                this->compound_job_alarms[job]->kill();
                this->compound_job_alarms.erase(job);
                break;
            }
        }
        this->finished_bare_metal_one_shot_compute_services.clear();

        if (not executor_on_the_list) {
            // warning
            WRENCH_WARN(
                    "BatchComputeService::processCompoundJobCompletion(): Received a compound job completion, but "
                    "the executor is not in the executor list - Likely getting wires crossed due to concurrent "
                    "completion and time-outs.. ignoring")
            return;
        }

        // Look for the corresponding batch job
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto const &rj : this->running_jobs) {
            if (rj->getCompoundJob() == job) {
                batch_job = rj;
                break;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::processCompoundJobCompletion(): Received a compound job completion, "
                    "but the job is not in the running job list");
        }

        WRENCH_INFO("A compound job executor has completed job %s", job->getName().c_str());

        // Free up resources (by finding the corresponding BatchJob)
        this->freeUpResources(batch_job->getResourcesAllocated());

        // Remove the job from the running job list
        this->removeJobFromRunningList(batch_job);

        // notify the scheduled of the job completion
        this->scheduler->processJobCompletion(batch_job);

        // Send the callback to the originator
        S4U_Mailbox::dputMessage(
                job->popCallbackMailbox(),
                new ComputeServiceCompoundJobDoneMessage(
                        job, this->getSharedPtr<BatchComputeService>(),
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD)));

        //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
        // this job is not used anymore anywhere)
        this->removeBatchJobFromJobsList(batch_job);
    }

    /**
     * @brief Helper function to remove a job from the batch queue
     * @param job: the job to remove
     */
    void BatchComputeService::removeJobFromBatchQueue(std::shared_ptr<BatchJob> job) {
        for (auto it = this->batch_queue.begin(); it != this->batch_queue.end(); it++) {
            if ((*it) == job) {
                this->batch_queue.erase(it);
                return;
            }
        }
    }

    /**
     * @brief Process a compound job failure
     * @param executor: the executor (one-shot bare-metal)
     * @param job: the compound job
     * @param cause: the cause of the failure
     */
    void BatchComputeService::processCompoundJobFailure(std::shared_ptr<BareMetalComputeServiceOneShot> executor,
                                                        std::shared_ptr<CompoundJob> job,
                                                        std::shared_ptr<FailureCause> cause) {
        bool executor_on_the_list = false;
        std::set<std::shared_ptr<BareMetalComputeServiceOneShot>>::iterator it;
        for (it = this->running_bare_metal_one_shot_compute_services.begin();
             it != this->running_bare_metal_one_shot_compute_services.end(); it++) {
            if ((*it) == executor) {
                PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_bare_metal_one_shot_compute_services),
                                                       &(this->finished_bare_metal_one_shot_compute_services));
                executor_on_the_list = true;
                this->compound_job_alarms[job]->kill();
                this->compound_job_alarms.erase(job);
                break;
            }
        }
        this->finished_bare_metal_one_shot_compute_services.clear();

        if (not executor_on_the_list) {
            throw std::runtime_error(
                    "BatchComputeService::processCompoundJobFailure(): Received a compound job failure, "
                    "but the executor is not in the executor list");
        }

        // Free up resources (by finding the corresponding BatchJob)
        std::shared_ptr<BatchJob> batch_job = nullptr;
        // TODO: to a map
        for (auto const &rj : this->running_jobs) {
            if (rj->getCompoundJob() == job) {
                batch_job = rj;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::processCompoundJobFailure(): Received a compound job failure, "
                    "but the job is not in the running job list");
        }

        this->freeUpResources(batch_job->getResourcesAllocated());
        this->removeJobFromRunningList(batch_job);

        WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

        // notify the scheduler of the failure
        this->scheduler->processJobFailure(batch_job);

        this->sendCompoundJobFailureNotification(job, std::to_string((batch_job->getJobID())), cause);
        // Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method
        // to make sure this job is not used anymore anywhere)
        this->removeBatchJobFromJobsList(batch_job);
    }

    /**
     * @brief
     * @return
     */
    unsigned long BatchComputeService::generateUniqueJobID() {
        static unsigned long jobid = 1;
        return jobid++;
    }

    /**
     * @brief Starts a job on a set of resources
     * @param resources
     * @param compound_job
     * @param batch_job
     * @param num_nodes_allocated
     * @param allocated_time: in seconds
     * @param cores_per_node_asked_for
     */
    void BatchComputeService::startJob(
            std::map<std::string, std::tuple<unsigned long, double>> resources,
            std::shared_ptr<CompoundJob> compound_job,
            std::shared_ptr<BatchJob> batch_job, unsigned long num_nodes_allocated,
            unsigned long allocated_time,
            unsigned long cores_per_node_asked_for) {

//        if (auto sjob = std::dynamic_pointer_cast<StandardJob>(workflow_job)) {
        WRENCH_INFO(
                "Creating a BareMetalComputeServiceOneShot for a compound job on %ld nodes with %ld cores per node",
                num_nodes_allocated, cores_per_node_asked_for);


        // Create a bare-metal-service job executor

//        BareMetalComputeServiceOneShot(std::shared_ptr<CompoundJob> job,
//        const std::string &hostname,
//        std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
//        std::map<std::string, std::string> property_list,
//        std::map<std::string, double> messagepayload_list,
//        double ttl,
//        std::shared_ptr<PilotJob> pj, std::string suffix,
//                std::shared_ptr<StorageService> scratch_space); // reference to upper level scratch space

        compound_job->pushCallbackMailbox(this->mailbox_name);
        auto executor = std::shared_ptr<BareMetalComputeServiceOneShot>(new
                                                                                BareMetalComputeServiceOneShot(
                compound_job,
                this->hostname,
                resources,
                {{BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD,
                         this->getPropertyValueAsString(
                                 BatchComputeServiceProperty::TASK_STARTUP_OVERHEAD)}
                },
                {},
                DBL_MAX,
                nullptr,
                "one_shot_bm_",
                this->getScratch()
        ));

        executor->simulation = this->simulation;
        executor->start(executor, true, false); // Daemonized, no auto-restart
        batch_job->setBeginTimestamp(S4U_Simulation::getClock());
        batch_job->setEndingTimestamp(S4U_Simulation::getClock() + allocated_time);
        this->running_bare_metal_one_shot_compute_services.insert(executor);

//          this->running_jobs.insert(std::move(batch_job_ptr));
        this->timeslots.push_back(batch_job->getEndingTimestamp());
        //remember the allocated resources for the job
        batch_job->setAllocatedResources(resources);

        SimulationMessage *msg = new AlarmJobTimeOutMessage(batch_job, 0);

        std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
                                                                      batch_job->getEndingTimestamp(),
                                                                      this->hostname,
                                                                      this->mailbox_name, msg,
                                                                      "batch_standard");
        compound_job_alarms[compound_job] = alarm_ptr;
        return;

//        } else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(workflow_job)) {
//            // pilot job
//            WRENCH_INFO(
//                    "Allocating %ld nodes with %ld cores per node to a pilot job for %lu seconds",
//                    num_nodes_allocated, cores_per_node_asked_for, allocated_time);
//
//            std::vector<std::string> nodes_for_pilot_job = {};
//            for (auto r : resources) {
//                nodes_for_pilot_job.push_back(std::get<0>(r));
//            }
//            std::string host_to_run_on = nodes_for_pilot_job[0];
//
//            //set the ending timestamp of the batchjob (pilotjob)
//
//            // Create and launch a compute service for the pilot job
//            // (We use a TTL for user information purposes, but an alarm will take care of this)
//            std::shared_ptr<ComputeService> cs = std::shared_ptr<ComputeService>(
//                    new BareMetalComputeService(
//                            host_to_run_on,
//                            resources,
//                            {{BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
//                             {BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,    "false"}},
//                            {},
//                            allocated_time, pjob, "pilot_job", getScratch()
//                    ));
//            cs->setSimulation(this->simulation);
//            pjob->setComputeService(cs);
//
//            try {
//                cs->start(cs, true, false); // Daemonized, no auto-restart
//                batch_job->setBeginTimestamp(S4U_Simulation::getClock());
//                double ending_timestamp = S4U_Simulation::getClock() + (double) allocated_time;
//                batch_job->setEndingTimestamp(ending_timestamp);
//            } catch (std::runtime_error &e) {
//                throw;
//            }
//
//            // Put the job in the running queue
////          this->running_jobs.insert(std::move(batch_job_ptr));
//            this->timeslots.push_back(batch_job->getEndingTimestamp());
//
//            //remember the allocated resources for the job
//            batch_job->setAllocatedResources(resources);
//
//            // Send the "Pilot job has started" callback
//            // Note the getCallbackMailbox instead of the popCallbackMailbox, because
//            // there will be another callback upon termination.
//            S4U_Mailbox::dputMessage(
//                    pjob->getCallbackMailbox(),
//                    new ComputeServicePilotJobStartedMessage(
//                            pjob, this->getSharedPtr<BatchComputeService>(),
//                            this->getMessagePayloadValue(
//                                    BatchComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
//
//            SimulationMessage *msg =
//                    new AlarmJobTimeOutMessage(batch_job, 0);
//
//            std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
//                                                                          batch_job->getEndingTimestamp(),
//                                                                          host_to_run_on,
//                                                                          this->mailbox_name, msg,
//                                                                          "batch_pilot");
//
//            this->pilot_job_alarms[pjob->getName()] = alarm_ptr;
//        }
    }

    /**
    * @brief Process a "get resource description message"
    * @param answer_mailbox: the mailbox to which the description message should be sent
    */
    void BatchComputeService::processGetResourceInformation(const std::string &answer_mailbox) {
        // Build a dictionary
        std::map<std::string, std::map<std::string, double>> dict;

        // Num hosts
        std::map<std::string, double> num_hosts;
        num_hosts.insert(std::make_pair(this->getName(), (double) (this->nodes_to_cores_map.size())));
        dict.insert(std::make_pair("num_hosts", num_hosts));

        // Num cores per hosts
        std::map<std::string, double> num_cores;
        for (auto h : this->nodes_to_cores_map) {
            num_cores.insert(std::make_pair(h.first, (double) (h.second)));
        }
        dict.insert(std::make_pair("num_cores", num_cores));

        // Num idle cores per hosts
        std::map<std::string, double> num_idle_cores;
        for (auto h : this->available_nodes_to_cores) {
            num_idle_cores.insert(std::make_pair(h.first, (double) (h.second)));
        }
        dict.insert(std::make_pair("num_idle_cores", num_idle_cores));

        // Flop rate per host
        std::map<std::string, double> flop_rates;
        for (auto h : this->nodes_to_cores_map) {
            flop_rates.insert(std::make_pair(h.first, S4U_Simulation::getHostFlopRate(h.first)));
        }
        dict.insert(std::make_pair("flop_rates", flop_rates));

        // RAM capacity per host
        std::map<std::string, double> ram_capacities;
        for (auto h : this->nodes_to_cores_map) {
            ram_capacities.insert(std::make_pair(h.first, S4U_Simulation::getHostMemoryCapacity(h.first)));
        }
        dict.insert(std::make_pair("ram_capacities", ram_capacities));

        // RAM availability per host  (0 if something is running, full otherwise)
        std::map<std::string, double> ram_availabilities;
        for (auto h : this->available_nodes_to_cores) {
            if (h.second < S4U_Simulation::getHostMemoryCapacity(h.first)) {
                ram_availabilities.insert(std::make_pair(h.first, 0.0));
            } else {
                ram_availabilities.insert(std::make_pair(h.first, S4U_Simulation::getHostMemoryCapacity(h.first)));
            }
        }
        dict.insert(std::make_pair("ram_availabilities", ram_availabilities));

        std::map<std::string, double> ttl;
        ttl.insert(std::make_pair(this->getName(), DBL_MAX));
        dict.insert(std::make_pair("ttl", ttl));

        // Send the reply
        auto *answer_message = new ComputeServiceResourceInformationAnswerMessage(
                dict,
                this->getMessagePayloadValue(
                        ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
    }

    /**
     * @brief Start the process that will replay the background load
     *
     * @throw std::runtime_error
     */
    void BatchComputeService::startBackgroundWorkloadProcess() {
        if (this->workload_trace.empty()) {
            throw std::runtime_error(
                    "BatchComputeService::startBackgroundWorkloadProcess(): no workload trace file specified");
        }

        // Create the trace replayer process
        this->workload_trace_replayer = std::shared_ptr<WorkloadTraceFileReplayer>(
                new WorkloadTraceFileReplayer(
                        S4U_Simulation::getHostName(),
                        this->getSharedPtr<BatchComputeService>(),
                        this->num_cores_per_node,
                        this->getPropertyValueAsBoolean(
                                BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE),
                        this->workload_trace)
        );
        try {
            this->workload_trace_replayer->setSimulation(this->simulation);
            this->workload_trace_replayer->start(this->workload_trace_replayer, true,
                                                 false); // Daemonized, no auto-restart
        } catch (std::runtime_error &e) {
            throw;
        }
    }

    /**
     * @brief Process a "terminate compound job message"
     *
     * @param job: the job to terminate
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void BatchComputeService::processCompoundJobTerminationRequest(std::shared_ptr<CompoundJob> job,
                                                                   std::string answer_mailbox) {
        std::shared_ptr<BatchJob> batch_job = nullptr;
        // Is it running?
        bool is_running = false;
        // TODO: ALL BELOW in maps!
        for (auto const &j : this->running_jobs) {
            auto cj = j->getCompoundJob();
            if (cj == job) {
                batch_job = j;
                is_running = true;
                break;
            }
        }

        bool is_pending = false;
        auto batch_pending_it = this->batch_queue.end();
        if (batch_job == nullptr) {
            // Is it pending?
            for (auto it1 = this->batch_queue.begin(); it1 != this->batch_queue.end(); it1++) {
                auto compound_job = (*it1)->getCompoundJob();
                if (compound_job == job) {
                    batch_pending_it = it1;
                    is_pending = true;
                    break;
                }
            }
        }

        bool is_waiting = false;

        if (batch_job == nullptr && batch_pending_it == this->batch_queue.end()) {
            // Is it waiting?
            for (auto const &j : this->waiting_jobs) {
                auto compound_job = j->getCompoundJob();
                if (compound_job == job) {
                    batch_job = j;
                    is_waiting = true;
                    break;
                }
            }
        }

//        WRENCH_INFO("pending: %d   running: %d   waiting: %d", is_pending, is_running, is_waiting);

        if (!is_pending && !is_running && !is_waiting) {
            std::string msg = "Job cannot be terminated because it is neither pending, not running, not waiting";
            // Send a failure reply
            auto answer_message =
                    new ComputeServiceTerminateCompoundJobAnswerMessage(
                            job, this->getSharedPtr<BatchComputeService>(), false, std::shared_ptr<FailureCause>(
                                    new NotAllowed(this->getSharedPtr<BatchComputeService>(),
                                                   msg)),
                            this->getMessagePayloadValue(
                                    BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
            return;
        }

        // Is it running?
        if (is_running) {
            this->scheduler->processJobTermination(batch_job);
            terminateRunningCompoundJob(job, ComputeService::TerminationCause::TERMINATION_JOB_KILLED);
            this->freeUpResources(batch_job->getResourcesAllocated());
            this->running_jobs.erase(batch_job);
            this->removeBatchJobFromJobsList(batch_job);
        }
        if (is_pending) {
            this->scheduler->processJobTermination(*batch_pending_it);
            auto to_free = *batch_pending_it;
            this->batch_queue.erase(batch_pending_it);
            this->removeBatchJobFromJobsList(to_free);
        }
        if (is_waiting) {
            this->scheduler->processJobTermination(batch_job);
            this->waiting_jobs.erase(batch_job);
            this->removeBatchJobFromJobsList(batch_job);
        }
        auto answer_message =
                new ComputeServiceTerminateCompoundJobAnswerMessage(
                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
    }


    /**
     * @brief Process a Batch bach_job timeout
     * @param bach_job: the batch bach_job
     */
    void BatchComputeService::processAlarmJobTimeout(std::shared_ptr<BatchJob> bach_job) {
        if (this->running_jobs.find(bach_job) == this->running_jobs.end()) {
            // time out
            WRENCH_INFO(
                    "BatchComputeService::processAlarmJobTimeout(): Received a time out message for an unknown "
                    "batch bach_job (%ld)... ignoring",
                    (unsigned long) bach_job.get());
            return;
        }
        auto compound_job = bach_job->getCompoundJob();

        this->processCompoundJobTimeout(compound_job);

        // We will get job failed messages stuff and process then
//        this->removeJobFromRunningList(bach_job);
//        this->freeUpResources(bach_job->getResourcesAllocated());
//        this->sendCompoundJobFailureNotification(compound_job,
//                                                 std::to_string(bach_job->getJobID()),
//                                                 std::shared_ptr<FailureCause>(
//                                                         new JobTimeout(bach_job->getCompoundJob())));
        return;


    }

    /**
     * @brief Method to hand incoming batsched message
     *
     * @param bat_sched_reply
     */
    void BatchComputeService::processExecuteJobFromBatSched(std::string bat_sched_reply) {
        nlohmann::json execute_events = nlohmann::json::parse(bat_sched_reply);
        std::shared_ptr<CompoundJob> compound_job = nullptr;
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto it1 = this->waiting_jobs.begin(); it1 != this->waiting_jobs.end(); it1++) {
            if (std::to_string((*it1)->getJobID()) == execute_events["job_id"]) {
                batch_job = (*it1);
                compound_job = batch_job->getCompoundJob();
                this->waiting_jobs.erase(batch_job);
                this->running_jobs.insert(batch_job);
                break;
            }
        }
        if (compound_job == nullptr) { WRENCH_WARN(
                    "BatchComputeService::processExecuteJobFromBatSched(): Job received from batsched that does not "
                    "belong to the list of known jobs... just telling Batsched that the job completed (Batsched seems "
                    "to send this back even when a job has been actively terminated)");
            this->scheduler->processUnknownJobTermination(execute_events["job_id"]);
            return;
        }

        /* Get the nodes and cores per nodes asked for */
        std::string nodes_allocated_by_batsched = execute_events["alloc"];
        batch_job->csv_allocated_processors = nodes_allocated_by_batsched;
        std::vector<std::string> allocations;
        boost::split(allocations, nodes_allocated_by_batsched, boost::is_any_of(" "));
        std::vector<unsigned long> node_resources;
        for (auto alloc:allocations) {
            std::vector<std::string> each_allocations;
            boost::split(each_allocations, alloc, boost::is_any_of("-"));
            if (each_allocations.size() < 2) {
                std::string start_node = each_allocations[0];
                std::string::size_type sz;
                unsigned long start = std::stoul(start_node, &sz);
                node_resources.push_back(start);
            } else {
                std::string start_node = each_allocations[0];
                std::string end_node = each_allocations[1];
                std::string::size_type sz;
                unsigned long start = std::stoul(start_node, &sz);
                unsigned long end = std::stoul(end_node, &sz);
                for (unsigned long i = start; i <= end; i++) {
                    node_resources.push_back(i);
                }
            }
        }

        unsigned long num_nodes_allocated = node_resources.size();
        unsigned long time_in_seconds = batch_job->getRequestedTime();
        unsigned long cores_per_node_asked_for = batch_job->getRequestedCoresPerNode();

        std::map<std::string, std::tuple<unsigned long, double>> resources = {};
        std::vector<std::string> hosts_assigned = {};
        std::map<std::string, unsigned long>::iterator it;

        for (auto node:node_resources) {
            double ram_capacity = S4U_Simulation::getHostMemoryCapacity(
                    this->host_id_to_names[node]); // Use the whole RAM
            this->available_nodes_to_cores[this->host_id_to_names[node]] -= cores_per_node_asked_for;
            resources.insert(std::make_pair(this->host_id_to_names[node], std::make_tuple(
                    cores_per_node_asked_for, ram_capacity)));
        }

        startJob(resources, compound_job, batch_job, num_nodes_allocated, time_in_seconds,
                 cores_per_node_asked_for);
    }


    /**
     * @brief Process a host available resource request
     * @param answer_mailbox: the answer mailbox
     * @param num_cores: the desired number of cores
     * @param ram: the desired RAM
     */
    void BatchComputeService::processIsThereAtLeastOneHostWithAvailableResources(const std::string &answer_mailbox,
                                                                                 unsigned long num_cores, double ram) {
        throw std::runtime_error(
                "BatchComputeService::processIsThereAtLeastOneHostWithAvailableResources(): A batch compute service does not support this operation");
    }


    void BatchComputeService::validateServiceSpecificArguments(std::shared_ptr<CompoundJob> job,
                                                               std::map<std::string, std::string> &service_specific_args) {
        // Check that -N, -t, and -c are specified
        // -user is optional
        // everything else must be a task1
        bool found_dash_N = false;
        bool found_dash_t = false;
        bool found_dash_c = false;

        for (auto const &arg : service_specific_args) {
            auto key = arg.first;
            auto value = arg.second;

            if (key == "-N") {
                found_dash_N = true;
                unsigned long num_nodes;
                if (sscanf(value.c_str(),"%lu", &num_nodes) != 1) {
                    throw std::invalid_argument("Invalid service-specific argument {\"" + key + "\",\"" + value +"\"}");
                }
                if (this->compute_hosts.size() < num_nodes) {
                    throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<ComputeService>()));
                }
            } else if (key == "-t") {
                found_dash_t = true;
                unsigned long requested_time;
                if (sscanf(value.c_str(),"%lu", &requested_time) != 1) {
                    throw std::invalid_argument("Invalid service-specific argument {\"" + key + "\",\"" + value +"\"}");
                }
            } else if (key == "-c") {
                found_dash_c = true;
                unsigned long num_cores;
                if (sscanf(value.c_str(),"%lu", &num_cores) != 1) {
                    throw std::invalid_argument("Invalid service-specific argument {\"" + key + "\",\"" + value +"\"}");
                }
                if (this->num_cores_per_node < num_cores) {
                    throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<ComputeService>()));
                }
            } else if (key == "-u") {
                // nothing
            } else if (key == "-color") {
                // nothing
            } else {
                // It has to be an action
                bool found_task = false;
                if (job != nullptr) {
                    // TODO: Should we have an ID/Action map in the job instead of just a set of actions?
                    for (auto const &action : job->getActions()) {
                        if (action->getName() == key) {
                            found_task = true;
                            break;
                        }
                    }
                }
                if (not found_task) {
                    throw std::invalid_argument("Invalid service-specific argument {" + key + "," + value +"}: Job does not have any task1 with name " + key);
                }
            }
        }
        if (not found_dash_t) {
            throw std::invalid_argument("Compute service requires a '-t' service-specific argument");
        }
        if (not found_dash_N) {
            throw std::invalid_argument("Compute service requires a '-N' service-specific argument");
        }
        if (not found_dash_c) {
            throw std::invalid_argument("Compute service requires a '-c' service-specific argument");
        }

    }

}
