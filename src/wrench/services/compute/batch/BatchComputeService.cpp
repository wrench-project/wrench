/**
 * Copyright (c) 2017-2022. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>

#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeServiceOneShot.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/util/TraceFileLoader.h>
#include <wrench/job/PilotJob.h>
#include "wrench/services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayer.h"
#include "wrench/services/compute/batch/batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.h"
#include "wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/ConservativeBackfillingBatchScheduler.h"
#include "wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/ConservativeBackfillingBatchSchedulerCoreLevel.h"
#include "wrench/services/compute/batch/batch_schedulers/homegrown/easy_bf/EasyBackfillingBatchScheduler.h"
#include "wrench/services/compute/batch/batch_schedulers/batsched/BatschedBatchScheduler.h"
#include <wrench/failure_causes/FunctionalityNotAvailable.h>
#include <wrench/failure_causes/JobKilled.h>
#include <wrench/failure_causes/ServiceIsDown.h>
#include <wrench/failure_causes/NotEnoughResources.h>
#include <wrench/failure_causes/JobTimeout.h>
#include <wrench/failure_causes/NotAllowed.h>


WRENCH_LOG_CATEGORY(wrench_core_batch_service, "Log category for Batch Service");

namespace wrench {
    // Do not remove
    BatchComputeService::~BatchComputeService() = default;

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param compute_hosts: the list of names of the available compute hosts
     *                 - the hosts must be homogeneous (speed, number of cores, and RAM size)
     *                 - all cores are usable by the BatchComputeService service on each host
     *                 - all RAM is usable by the BatchComputeService service on each host
     * @param scratch_space_mount_point: the mount point of the scratch storage space for the service ("" means "no scratch space")
     * @param property_list: a property list that specifies BatchComputeServiceProperty values ({} means "use all defaults")
     * @param messagepayload_list: a message payload list that specifies BatchComputeServiceMessagePayload values ({} means "use all defaults")
     */
    BatchComputeService::BatchComputeService(const std::string &hostname,
                                             std::vector<std::string> compute_hosts,
                                             const std::string& scratch_space_mount_point,
                                             WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                             WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) : BatchComputeService(hostname, std::move(compute_hosts), ComputeService::ALL_CORES,
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
     * @param suffix: suffix to append to the service name and commport
     *
     */
    BatchComputeService::BatchComputeService(const std::string &hostname,
                                             std::vector<std::string> compute_hosts,
                                             unsigned long cores_per_host,
                                             sg_size_t ram_per_host,
                                             std::string scratch_space_mount_point,
                                             WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                             WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list,
                                             const std::string &suffix) : ComputeService(hostname,
                                                                                         "BatchComputeService" + suffix,
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

        // Get the hosts
        for (const auto &h: compute_hosts) {
            this->compute_hosts.push_back(S4U_Simulation::get_host_or_vm_by_name(h));
        }

        //        for (const auto &h : this->compute_hosts) {
        //            std::cerr << "==> " << h->get_name() << "\n";
        //        }

        // Check Platform homogeneity
        auto first_host = *(this->compute_hosts.begin());
        auto num_cores_available = (first_host->get_core_count());
        double speed = first_host->get_speed();
        sg_size_t ram_available = S4U_Simulation::getHostMemoryCapacity(first_host);

        for (auto const &h: this->compute_hosts) {
            // Compute speed
            if (std::abs(speed - h->get_speed()) > DBL_EPSILON) {
                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a BatchComputeService service need "
                        "to be homogeneous (different flop rates detected)");
            }
            // RAM
            if (ram_available != S4U_Simulation::getHostMemoryCapacity(h)) {
                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a BatchComputeService service need "
                        "to be homogeneous (different RAM capacities detected)");
            }
            // Num cores
            if (static_cast<double>(h->get_core_count()) != num_cores_available) {
                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a BatchComputeService service need "
                        "to be homogeneous (different RAM capacities detected)");
            }
        }


        //create a map for host to cores
        int i = 0;
        for (const auto &h: this->compute_hosts) {
            this->nodes_to_cores_map[h] = num_cores_available;
            this->available_nodes_to_cores[h] = num_cores_available;
            this->host_id_to_names[i++] = h;
        }

        this->num_cores_per_node = this->nodes_to_cores_map.begin()->second;
        this->total_num_of_nodes = compute_hosts.size();

        // Check that the workload file is valid
        std::string workload_file = this->getPropertyValueAsString(
                BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE);
        if (not workload_file.empty()) {
            this->workload_trace = TraceFileLoader::loadFromTraceFile(
                    workload_file,
                    this->getPropertyValueAsBoolean(
                            BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE),
                    this->getPropertyValueAsDouble(
                            BatchComputeServiceProperty::SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE));
            // Fix value
            for (auto &j: this->workload_trace) {
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
        auto batch_scheduling_alg = this->getPropertyValueAsString(
                BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM);
        if (this->scheduling_algorithms.find(batch_scheduling_alg) == this->scheduling_algorithms.end()) {
            throw std::invalid_argument(
                    " BatchComputeService::BatchComputeService(): unsupported scheduling algorithm " +
                    batch_scheduling_alg);
        }
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
            this->scheduler = std::make_unique<FCFSBatchScheduler>(this);
        } else if (batch_scheduling_alg == "conservative_bf") {
            this->scheduler = std::make_unique<ConservativeBackfillingBatchScheduler>(
                this,
                this->getPropertyValueAsUnsignedLong(BatchComputeServiceProperty::BACKFILLING_DEPTH));
        } else if (batch_scheduling_alg == "easy_bf_depth0") {
            this->scheduler = std::make_unique<EasyBackfillingBatchScheduler>(
                this,
                0,
                this->getPropertyValueAsUnsignedLong(BatchComputeServiceProperty::BACKFILLING_DEPTH));
        } else if (batch_scheduling_alg == "easy_bf_depth1") {
            this->scheduler = std::make_unique<EasyBackfillingBatchScheduler>(
                this,
                1,
                this->getPropertyValueAsUnsignedLong(BatchComputeServiceProperty::BACKFILLING_DEPTH));
        } else if (batch_scheduling_alg == "conservative_bf_core_level") {
            this->scheduler = std::make_unique<ConservativeBackfillingBatchSchedulerCoreLevel>(
                this,
                this->getPropertyValueAsUnsignedLong(BatchComputeServiceProperty::BACKFILLING_DEPTH));
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
            std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs) {
        try {
            auto estimates = this->scheduler->getStartTimeEstimates(std::move(set_of_jobs));
            return estimates;
        } catch (ExecutionException &e) {
            throw;
        } catch (std::exception &e) {
            throw ExecutionException(std::make_shared<FunctionalityNotAvailable>(
                    this->getSharedPtr<BatchComputeService>(), "start time estimates"));
        }
    }

    /**
    * @brief Gets the state of the BatchComputeService queue
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
        for (auto const &j: this->running_jobs) {
            auto batch_job = j.second;
            auto tuple = std::make_tuple(
                    batch_job->getUsername(),
                    batch_job->getCompoundJob()->getName(),
                    batch_job->getRequestedNumNodes(),
                    batch_job->getRequestedCoresPerNode(),
                    batch_job->getRequestedTime(),
                    batch_job->getArrivalTimestamp(),
                    batch_job->getBeginTimestamp());
            queue_state.emplace_back(tuple);
        }

#ifdef ENABLE_BATSCHED
        //  Go through the waiting jobs (BATSCHED only)
        for (auto const &j: this->waiting_jobs) {
            auto tuple = std::make_tuple(
                    j->getUsername(),
                    j->getCompoundJob()->getName(),
                    j->getRequestedNumNodes(),
                    j->getRequestedCoresPerNode(),
                    j->getRequestedTime(),
                    j->getArrivalTimestamp(),
                    -1.0);
            queue_state.emplace_back(tuple);
        }
#endif

        // Go through the pending jobs
        for (auto const &j: this->batch_queue) {
            auto tuple = std::make_tuple(
                    j->getUsername(),
                    j->getCompoundJob()->getName(),
                    j->getRequestedNumNodes(),
                    j->getRequestedCoresPerNode(),
                    j->getRequestedTime(),
                    j->getArrivalTimestamp(),
                    -1.0);
            queue_state.emplace_back(tuple);
        }

        // Sort all jobs by  arrival  time
        std::sort(queue_state.begin(), queue_state.end(),
                  [](const std::tuple<std::string, std::string, int, int, int, double, double> &j1,
                     const std::tuple<std::string, std::string, int, int, int, double, double> &j2) -> bool {
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
     */
    unsigned long BatchComputeService::parseUnsignedLongServiceSpecificArgument(
            const std::string &key,
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
                    " argument to be specified for job submission");
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
        unsigned long time_asked_for_in_seconds = 0;
        num_hosts = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-N", batch_job_args);
        num_cores_per_host = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-c", batch_job_args);
        time_asked_for_in_seconds = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-t",
                                                                                                  batch_job_args);

        std::string username = "you";
        if (batch_job_args.find("-u") != batch_job_args.end()) {
            username = batch_job_args.at("-u");
        }

        // Sanity check
        if ((num_hosts == 0) or (num_cores_per_host == 0) or (time_asked_for_in_seconds == 0)) {
            throw std::invalid_argument(
                    "BatchComputeService::submitCompoundJob(): service-specific arguments should have non-zero values");
        }

        // Create a Batch Job
        unsigned long jobid = wrench::BatchComputeService::generateUniqueJobID();
        auto batch_job = std::make_shared<BatchJob>(job, jobid, time_asked_for_in_seconds,
                                                    num_hosts, num_cores_per_host, username, -1,
                                                    S4U_Simulation::getClock());

        // Set job display color for csv output
        auto it = batch_job_args.find("-color");
        if (it != batch_job_args.end()) {
            batch_job->csv_metadata = "color:" + (*it).second;
        }

        // Send a "run a BatchComputeService job" message to the daemon's commport
        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();
        this->commport->dputMessage(
                new BatchComputeServiceJobRequestMessage(
                        answer_commport, batch_job,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ComputeServiceSubmitCompoundJobAnswerMessage>(
                this->network_timeout,
                "BatchComputeService::submitCompoundJob(): Received an");
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

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        // Send a "terminate a  job" message to the daemon's commport
        this->commport->putMessage(
                new ComputeServiceTerminateCompoundJobRequestMessage(
                        answer_commport,
                        job,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ComputeServiceTerminateCompoundJobAnswerMessage>(
                this->network_timeout,
                "BatchComputeService::terminateCompoundJob(): Received an");

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
            startBackgroundWorkloadProcess();
        }

        // Start the Scratch Storage Service
        this->startScratchStorageService();

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
    void BatchComputeService::sendPilotJobExpirationNotification(const std::shared_ptr<PilotJob> &job) {
        job->popCallbackCommPort()->dputMessage(
                new ComputeServicePilotJobExpiredMessage(
                        job, this->getSharedPtr<BatchComputeService>(),
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Send back notification that a standard job has failed
     * @param job: the job that has failed
     * @param job_id: the job's id
     * @param cause: the failure cause
     */
    void BatchComputeService::sendCompoundJobFailureNotification(const std::shared_ptr<CompoundJob> &job, const std::string &job_id,
                                                                 const std::shared_ptr<FailureCause> &cause) {
        WRENCH_INFO("Sending compound job failure notification for job %s", job->getName().c_str());

        std::shared_ptr<BatchJob> batch_job = this->all_jobs[job];

        try {
            job->popCallbackCommPort()->putMessage(
                    new ComputeServiceCompoundJobFailedMessage(
                            job, this->getSharedPtr<BatchComputeService>(),
                            this->getMessagePayloadValue(
                                    BatchComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &e) {
            return;// ignore
        }
    }

    /**
     * @brief Increase resource availabilities based on freed resources
     * @param resources: a set of tuples as follows:
     *              - host
     *              - number of cores (unsigned long)
     *              - bytes of RAM (double)
     */
    void BatchComputeService::freeUpResources(const std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> &resources) {
        for (auto r: resources) {
            this->available_nodes_to_cores[r.first] += std::get<0>(r.second);
        }
    }

    /**
     * @brief ...
     * @param job
     */
    void BatchComputeService::removeJobFromRunningList(const std::shared_ptr<BatchJob> &job) {
        if (this->running_jobs.find(job->getCompoundJob()) == this->running_jobs.end()) {
            throw std::runtime_error("BatchComputeService::removeJobFromRunningList(): Cannot find job!");
        }
        this->running_jobs.erase(job->getCompoundJob());
    }

    /**
     * @brief Remove a batch job from the list of all known jobs
     * @param batch_job: the batch job
     */
    void BatchComputeService::removeBatchJobFromJobsList(const std::shared_ptr<BatchJob> &batch_job) {
        if (batch_job == nullptr) {
            return;
        }
        this->all_jobs.erase(batch_job->getCompoundJob());
    }

    /**
     * @brief Process a compound job's timeout
     *
     * @param compound_job: The compound job that has timed out
     */
    void BatchComputeService::processCompoundJobTimeout(const std::shared_ptr<CompoundJob> &compound_job) {
        if (this->running_bare_metal_one_shot_compute_services.find(compound_job) ==
            this->running_bare_metal_one_shot_compute_services.end()) {
            throw std::runtime_error("BatchComputeService::processCompoundJobTimeout(): Unknown compound job");
        }

        auto executor = this->running_bare_metal_one_shot_compute_services[compound_job];
        WRENCH_INFO("Terminating a one-shot bare-metal service (due to a time out)");
        executor->stop(true, ComputeService::TerminationCause::TERMINATION_JOB_TIMEOUT);
    }

    /**
    * @brief terminate a running standard job
    * @param job: the job
    * @param termination_cause: the termination cause
    */
    void BatchComputeService::terminateRunningCompoundJob(const std::shared_ptr<CompoundJob> &job,
                                                          ComputeService::TerminationCause termination_cause) {
        if (this->running_bare_metal_one_shot_compute_services.find(job) ==
            this->running_bare_metal_one_shot_compute_services.end()) {
            throw std::runtime_error("BatchComputeService::terminateRunningCompoundJob(): Unknown compound job");
        }

        auto executor = this->running_bare_metal_one_shot_compute_services[job];

        WRENCH_INFO("Terminating a one-shot bare-metal service");
        executor->stop(false, termination_cause);// failure notifications sent by me later, if needed
    }

    /**
    * @brief Declare all current jobs as failed (likely because the daemon is being terminated
    * or has timed out (because it's in fact a pilot job))
    *
    * @param send_failure_notifications: whether to send failure notifications or not
    * @param termination_cause: the termination cause
    */
    void BatchComputeService::terminate(bool send_failure_notifications,
                                        ComputeService::TerminationCause termination_cause) {
        this->acquireDaemonLock();

        WRENCH_INFO("Terminating all current compound jobs");

        while (not this->running_jobs.empty()) {
            auto batch_job = (*(this->running_jobs.begin())).second;
            auto compound_job = batch_job->getCompoundJob();
            terminateRunningCompoundJob(compound_job, termination_cause);
            // Popping, because I am terminating it, so the executor won't pop, and right now
            // if I am at the top of the stack!
            if (compound_job->getCallbackCommPort() == this->commport) {
                compound_job->popCallbackCommPort();
            }
            if (send_failure_notifications) {
                this->sendCompoundJobFailureNotification(
                        compound_job, std::to_string(batch_job->getJobID()),
                        std::shared_ptr<FailureCause>(
                                new JobKilled(compound_job)));
            }
            this->running_jobs.erase(compound_job);
        }

        while (not this->batch_queue.empty()) {
            auto batch_job = (*(this->batch_queue.begin()));
            auto compound_job = batch_job->getCompoundJob();
            //            WRENCH_INFO("SIMPLY REMOVING COMPOUND JOB %s FROM PENDING LIST", compound_job->getName().c_str());
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
            for (auto const &action: compound_job->getActions()) {
                action->setFailureCause(failure_cause);
            }
            if (send_failure_notifications) {
                this->sendCompoundJobFailureNotification(
                        compound_job, std::to_string(batch_job->getJobID()),
                        std::shared_ptr<FailureCause>(
                                new JobKilled(compound_job)));
            }
            this->batch_queue.pop_front();
        }

#ifdef ENABLE_BATSCHED
        while (not this->waiting_jobs.empty()) {
            auto batch_job = (*(this->waiting_jobs.begin()));
            auto compound_job = batch_job->getCompoundJob();
            //            WRENCH_INFO("SIMPLY REMOVING COMPOUND JOB %s FROM WAITING LIST", compound_job->getName().c_str());

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
            for (auto const &action: compound_job->getActions()) {
                action->setFailureCause(failure_cause);
            }

            if (send_failure_notifications) {
                this->sendCompoundJobFailureNotification(
                        compound_job, std::to_string(batch_job->getJobID()),
                        std::shared_ptr<FailureCause>(
                                new JobKilled(compound_job)));
            }

            this->waiting_jobs.erase(batch_job);
        }
#endif

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
        S4U_Daemon::cleanup(has_returned_from_main, return_value);
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
     * @brief Wait for and process the next message
     * @return true if the service should keep going, false otherwise
     */
    bool BatchComputeService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &e) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
            return false;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            this->setStateToDown();
            this->terminate(msg->send_failure_notifications, msg->termination_cause);
            //            this->terminateRunningPilotJobs();

            // Send back a synchronous reply!
            try {
                msg->ack_commport->putMessage(
                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceResourceInformationRequestMessage>(message)) {
            processGetResourceInformation(msg->answer_commport, msg->key);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<BatchComputeServiceJobRequestMessage>(message)) {
            processJobSubmission(msg->job, msg->answer_commport);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceCompoundJobDoneMessage>(message)) {
            processCompoundJobCompletion(std::dynamic_pointer_cast<BareMetalComputeServiceOneShot>(msg->compute_service), msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceCompoundJobFailedMessage>(message)) {
            processCompoundJobFailure(std::dynamic_pointer_cast<BareMetalComputeServiceOneShot>(msg->compute_service), msg->job, nullptr);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceTerminateCompoundJobRequestMessage>(message)) {
            processCompoundJobTerminationRequest(msg->job, msg->answer_commport);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<AlarmJobTimeOutMessage>(message)) {
            processAlarmJobTimeout(msg->job);
            return true;

#ifdef ENABLE_BATSCHED
            } else if (auto msg = std::dynamic_pointer_cast<BatchExecuteJobFromBatSchedMessage>(message)) {
            processExecuteJobFromBatSched(msg->batsched_decision_reply);
            return true;
#endif

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage>(message)) {
            processIsThereAtLeastOneHostWithAvailableResources(msg->answer_commport, msg->num_cores, msg->ram);
            return true;

        } else {
            throw std::runtime_error(
                    "BatchComputeService::processNextMessage(): Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Process a job submission
     *
     * @param job: the BatchComputeService job object
     * @param answer_commport: the commport to which answer messages should be sent
     */
    void BatchComputeService::processJobSubmission(const std::shared_ptr<BatchJob> &job,
                                                   S4U_CommPort *answer_commport) {
        WRENCH_INFO("Asked to run a BatchComputeService job with id %ld", job->getJobID());

        // Check that the job can be admitted in terms of resources:
        //      - number of nodes,
        //      - number of cores per host
        //      - RAM (only for standard jobs)
        unsigned long requested_hosts = job->getRequestedNumNodes();
        unsigned long requested_num_cores_per_host = job->getRequestedCoresPerNode();

        sg_size_t required_ram_per_host = job->getMemoryRequirement();

        if ((requested_hosts > this->available_nodes_to_cores.size()) or
            (requested_num_cores_per_host >
             static_cast<unsigned long>(this->available_nodes_to_cores.begin()->first->get_core_count())) or
            (required_ram_per_host >
             S4U_Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first))) {
            {
                answer_commport->dputMessage(
                        new ComputeServiceSubmitCompoundJobAnswerMessage(
                                job->getCompoundJob(),
                                this->getSharedPtr<BatchComputeService>(),
                                false,
                                std::make_shared<NotEnoughResources>(
                                    job->getCompoundJob(),
                                    this->getSharedPtr<BatchComputeService>()),
                                this->getMessagePayloadValue(
                                        BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
                return;
            }
        }

        // SUCCESS!
        answer_commport->dputMessage(
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
        this->all_jobs[job->getCompoundJob()] = job;
        this->batch_queue.push_back(job);

        this->scheduler->processJobSubmission(job);
    }

    /**
     * @brief Process a standard job completion
     * @param executor: the standard job executor
     * @param job: the job
     *
     */
    void BatchComputeService::processCompoundJobCompletion(
            const std::shared_ptr<BareMetalComputeServiceOneShot> &executor,
            const std::shared_ptr<CompoundJob> &job) {
        if (this->running_bare_metal_one_shot_compute_services.find(job) ==
            this->running_bare_metal_one_shot_compute_services.end()) {
            // warning
            WRENCH_WARN(
                    "BatchComputeService::processCompoundJobCompletion(): Received a compound job completion, but "
                    "the executor is not in the executor list - Likely getting wires crossed due to concurrent "
                    "completion and time-outs.. ignoring");
            return;
        }

        this->running_bare_metal_one_shot_compute_services.erase(job);
        this->compound_job_alarms[job]->kill();
        this->compound_job_alarms.erase(job);

        // Look for the corresponding BatchComputeService job
        if (this->running_jobs.find(job) == this->running_jobs.end()) {
            throw std::runtime_error(
                    "BatchComputeService::processCompoundJobCompletion(): Received a compound job completion, "
                    "but the job is not in the running job list");
        }

        auto batch_job = this->running_jobs[job];

        WRENCH_INFO("A compound job executor has completed job %s", job->getName().c_str());

        // Free up resources (by finding the corresponding BatchJob)
        this->freeUpResources(batch_job->getResourcesAllocated());

        // Remove the job from the running job list
        this->removeJobFromRunningList(batch_job);

        // notify the scheduler of the job completion
        this->scheduler->processJobCompletion(batch_job);

        // Send the callback to the originator
        job->popCallbackCommPort()->dputMessage(
                new ComputeServiceCompoundJobDoneMessage(
                        job, this->getSharedPtr<BatchComputeService>(),
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD)));

        //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
        // this job is not used anymore anywhere)
        this->removeBatchJobFromJobsList(batch_job);
    }

    /**
     * @brief Helper function to remove a job from the BatchComputeService queue
     * @param job: the job to remove
     */
    void BatchComputeService::removeJobFromBatchQueue(const std::shared_ptr<BatchJob> &job) {
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
    void BatchComputeService::processCompoundJobFailure(const std::shared_ptr<BareMetalComputeServiceOneShot> &executor,
                                                        const std::shared_ptr<CompoundJob> &job,
                                                        const std::shared_ptr<FailureCause> &cause) {
        if (this->running_bare_metal_one_shot_compute_services.find(job) ==
            this->running_bare_metal_one_shot_compute_services.end()) {
            throw std::runtime_error(
                    "BatchComputeService::processCompoundJobFailure(): Received a compound job failure, "
                    "but the executor is not in the executor list");
        }

        this->running_bare_metal_one_shot_compute_services.erase(job);
        this->compound_job_alarms[job]->kill();
        this->compound_job_alarms.erase(job);

        // Free up resources (by finding the corresponding BatchJob)
        if (this->running_jobs.find(job) == this->running_jobs.end()) {
            throw std::runtime_error(
                    "BatchComputeService::processCompoundJobFailure(): Received a compound job failure, "
                    "but the job is not in the running job list");
        }
        auto batch_job = this->running_jobs[job];

        this->freeUpResources(batch_job->getResourcesAllocated());
        this->removeJobFromRunningList(batch_job);

        WRENCH_INFO("A compound job executor has failed to perform job %s", job->getName().c_str());

        // notify the scheduler of the failure
        //        std::cerr << "IN processCompoundJobFailure\n";
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
            const std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> &resources,
            const std::shared_ptr<CompoundJob> &compound_job,
            const std::shared_ptr<BatchJob> &batch_job, unsigned long num_nodes_allocated,
            unsigned long allocated_time,
            unsigned long cores_per_node_asked_for) {
        WRENCH_INFO(
                "Creating a BareMetalComputeServiceOneShot for a compound job on %ld nodes with %ld cores per node",
                num_nodes_allocated, cores_per_node_asked_for);

        compound_job->pushCallbackCommPort(this->commport);

        std::map<std::string, std::tuple<unsigned long, sg_size_t>> resources_by_hostname;
        for (auto const &h: resources) {
            resources_by_hostname[h.first->get_name()] = h.second;
        }
        auto executor = std::shared_ptr<BareMetalComputeServiceOneShot>(
                new BareMetalComputeServiceOneShot(
                        compound_job,
                        this->hostname,
                        resources_by_hostname,
                        {{BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD,
                          this->getPropertyValueAsString(
                                  BatchComputeServiceProperty::THREAD_STARTUP_OVERHEAD)}},
                        {},
                        nullptr,
                        "_one_shot_bm_",
                        this->getScratch()));

        executor->simulation_ = this->simulation_;
        executor->start(executor, true, false);// Daemonized, no auto-restart
        batch_job->setBeginTimestamp(S4U_Simulation::getClock());
        batch_job->setEndingTimestamp(S4U_Simulation::getClock() + static_cast<double>(allocated_time));
        this->running_bare_metal_one_shot_compute_services[compound_job] = executor;

        //          this->running_jobs.insert(std::move(batch_job_ptr));
        this->timeslots.push_back(batch_job->getEndingTimestamp());
        //remember the allocated resources for the job
        batch_job->setAllocatedResources(resources);

        auto msg = new AlarmJobTimeOutMessage(batch_job, 0);

        std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation_,
                                                                      batch_job->getEndingTimestamp(),
                                                                      this->hostname,
                                                                      this->commport, msg,
                                                                      "batch_standard");
        compound_job_alarms[compound_job] = alarm_ptr;
    }

    /**
     * @brief Construct a dict for resource information
     * @param key: the desired key
     * @return a dictionary
     */
    std::map<std::string, double> BatchComputeService::constructResourceInformation(const std::string &key) {
        // Build a dictionary
        std::map<std::string, double> dict;

        if (key == "num_hosts") {
            // Num hosts
            dict.insert(std::make_pair(this->getName(), static_cast<double>(this->nodes_to_cores_map.size())));

        } else if (key == "num_cores") {
            for (const auto &h: this->nodes_to_cores_map) {
                dict.insert(std::make_pair(h.first->get_name(), static_cast<double>(h.second)));
            }

        } else if (key == "num_idle_cores") {
            // Num idle cores per hosts
            for (const auto &h: this->available_nodes_to_cores) {
                dict.insert(std::make_pair(h.first->get_name(), static_cast<double>(h.second)));
            }

        } else if (key == "flop_rates") {
            // Flop rate per host
            for (const auto &h: this->nodes_to_cores_map) {
                dict.insert(std::make_pair(h.first->get_name(), h.first->get_speed()));
            }

        } else if (key == "ram_capacities") {
            // RAM capacity per host
            for (const auto &h: this->nodes_to_cores_map) {
                dict.insert(std::make_pair(h.first->get_name(), S4U_Simulation::getHostMemoryCapacity(h.first)));
            }

        } else if (key == "ram_availabilities") {
            // RAM availability per host  (0 if something is running, full otherwise)
            for (const auto &h: this->available_nodes_to_cores) {
                if (h.second < S4U_Simulation::getHostMemoryCapacity(h.first)) {
                    dict.insert(std::make_pair(h.first->get_name(), 0.0));
                } else {
                    dict.insert(std::make_pair(h.first->get_name(), S4U_Simulation::getHostMemoryCapacity(h.first)));
                }
            }
        } else {
            throw std::runtime_error("BatchComputeService::processGetResourceInformation(): unknown key");
        }
        return dict;
    }

    /**
    * @brief Process a "get resource description message"
    * @param answer_commport: the commport to which the description message should be sent
    * @param key: the desired resource information (i.e., dictionary key) that's needed)
    */
    void BatchComputeService::processGetResourceInformation(S4U_CommPort *answer_commport,
                                                            const std::string &key) {
        auto dict = this->constructResourceInformation(key);

        // Send the reply
        auto *answer_message = new ComputeServiceResourceInformationAnswerMessage(
                dict,
                this->getMessagePayloadValue(
                        ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
        answer_commport->dputMessage(answer_message);
    }

    /**
     * @brief Start the process that will replay the background load
     *
     */
    void BatchComputeService::startBackgroundWorkloadProcess() {
        if (this->workload_trace.empty()) {
            throw std::runtime_error(
                    "BatchComputeService::startBackgroundWorkloadProcess(): no workload trace file specified");
        }

        // Create the trace replayer process
        this->workload_trace_replayer = std::make_shared<WorkloadTraceFileReplayer>(

                S4U_Simulation::getHostName(),
                this->getSharedPtr<BatchComputeService>(),
                this->num_cores_per_node,
                this->getPropertyValueAsBoolean(
                        BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE),
                this->workload_trace);
        this->workload_trace_replayer->setSimulation(this->simulation_);
        this->workload_trace_replayer->start(this->workload_trace_replayer, true,
                                             false);// Daemonized, no auto-restart
    }

    /**
     * @brief Process a "terminate compound job message"
     *
     * @param job: the job to terminate
     * @param answer_commport: the commport to which the answer message should be sent
     */
    void BatchComputeService::processCompoundJobTerminationRequest(const std::shared_ptr<CompoundJob> &job,
                                                                   S4U_CommPort *answer_commport) {
        std::shared_ptr<BatchJob> batch_job = nullptr;
        // Is it running?
        bool is_running = false;
        bool is_pending = false;
        bool is_waiting = false;
        auto batch_pending_it = this->batch_queue.end();

        if (this->running_jobs.find(job) != this->running_jobs.end()) {
            batch_job = this->running_jobs[job];
            is_running = true;
        }

        if (not is_running) {
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

#ifdef ENABLE_BATSCHED
            if (not is_pending) {
                if (batch_job == nullptr && batch_pending_it == this->batch_queue.end()) {
                    // Is it waiting?
                    for (auto const &j: this->waiting_jobs) {
                        auto compound_job = j->getCompoundJob();
                        if (compound_job == job) {
                            batch_job = j;
                            is_waiting = true;
                            break;
                        }
                    }
                }
            }
#endif
        }

        //        WRENCH_INFO("pending: %d   running: %d   waiting: %d", is_pending, is_running, is_waiting);
        if (!is_pending && !is_running && !is_waiting) {
            std::string msg = "Job cannot be terminated because it is neither pending, not running, not waiting";
            // Send a failure reply
            auto answer_message =
                    new ComputeServiceTerminateCompoundJobAnswerMessage(
                            job, this->getSharedPtr<BatchComputeService>(), false, std::shared_ptr<FailureCause>(new NotAllowed(this->getSharedPtr<BatchComputeService>(), msg)),
                            this->getMessagePayloadValue(
                                    BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
            answer_commport->dputMessage(answer_message);
            return;
        }

        if (is_running) {
            this->scheduler->processJobTermination(batch_job);
            terminateRunningCompoundJob(job, ComputeService::TerminationCause::TERMINATION_JOB_KILLED);
            this->freeUpResources(batch_job->getResourcesAllocated());
            this->running_jobs.erase(batch_job->getCompoundJob());
            this->removeBatchJobFromJobsList(batch_job);
        }
        if (is_pending) {
            this->scheduler->processJobTermination((*batch_pending_it));
            auto to_free = *batch_pending_it;
            this->batch_queue.erase(batch_pending_it);
            this->removeBatchJobFromJobsList(to_free);
        }
#ifdef ENABLE_BATSCHED
        if (is_waiting) {
            this->scheduler->processJobTermination(batch_job);
            this->waiting_jobs.erase(batch_job);
            this->removeBatchJobFromJobsList(batch_job);
        }
#endif
        auto answer_message =
                new ComputeServiceTerminateCompoundJobAnswerMessage(
                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD));
        answer_commport->dputMessage(answer_message);
    }

    /**
     * @brief Process a Batch batch_job timeout
     *
     * @param batch_job: the batch job
     */
    void BatchComputeService::processAlarmJobTimeout(const std::shared_ptr<BatchJob> &batch_job) {
        std::shared_ptr<CompoundJob> compound_job = nullptr;

        for (auto const &j: this->running_jobs) {
            if (j.second == batch_job) {
                compound_job = j.first;
                break;
            }
        }
        if (not compound_job) {
            // time out
            WRENCH_INFO(
                    "BatchComputeService::processAlarmJobTimeout(): Received a time out message for an unknown "
                    "BatchComputeService batch_job (%ld)... ignoring",
                    reinterpret_cast<unsigned long>(batch_job.get()));
            return;
        }
        this->processCompoundJobTimeout(compound_job);
    }

#ifdef ENABLE_BATSCHED
    /**
     * @brief Method to hand incoming batsched message
     *
     * @param bat_sched_reply
     */
    void BatchComputeService::processExecuteJobFromBatSched(const std::string &bat_sched_reply) {
        nlohmann::json execute_events = nlohmann::json::parse(bat_sched_reply);
        std::shared_ptr<CompoundJob> compound_job = nullptr;
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto it1 = this->waiting_jobs.begin(); it1 != this->waiting_jobs.end(); it1++) {
            if (std::to_string((*it1)->getJobID()) == execute_events["job_id"]) {
                batch_job = (*it1);
                compound_job = batch_job->getCompoundJob();
                this->waiting_jobs.erase(batch_job);
                this->running_jobs[batch_job->getCompoundJob()] = batch_job;
                break;
            }
        }
        if (compound_job == nullptr) {
            WRENCH_WARN(
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
        for (auto alloc: allocations) {
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

        std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> resources = {};
        std::vector<simgrid::s4u::Host *> hosts_assigned = {};
        std::map<simgrid::s4u::Host *, unsigned long>::iterator it;

        for (auto node: node_resources) {
            sg_size_t ram_capacity = S4U_Simulation::getHostMemoryCapacity(
                    this->host_id_to_names[node]);// Use the whole RAM
            this->available_nodes_to_cores[this->host_id_to_names[node]] -= cores_per_node_asked_for;
            resources.insert(std::make_pair(this->host_id_to_names[node], std::make_tuple(
                                                                                  cores_per_node_asked_for, ram_capacity)));
        }

        startJob(resources, compound_job, batch_job, num_nodes_allocated, time_in_seconds,
                 cores_per_node_asked_for);
    }
#endif

    /**
     * @brief Process a host available resource request
     * @param answer_commport: the answer commport
     * @param num_cores: the desired number of cores
     * @param ram: the desired RAM
     */
    void BatchComputeService::processIsThereAtLeastOneHostWithAvailableResources(S4U_CommPort *answer_commport,
                                                                                 unsigned long num_cores, sg_size_t ram) {
        throw std::runtime_error(
                "BatchComputeService::processIsThereAtLeastOneHostWithAvailableResources(): A BatchComputeService compute service does not support this operation");
    }

    /**
     * @brief Method to validate a job's service-specific arguments
     * @param job: the job
     * @param service_specific_args: the service-specific arguments
     */
    void BatchComputeService::validateServiceSpecificArguments(const std::shared_ptr<CompoundJob> &job,
                                                               std::map<std::string, std::string> &service_specific_args) {
        // Check that -N, -t, and -c are specified
        // -user is optional
        // everything else must be a task
        bool found_dash_N = false;
        bool found_dash_t = false;
        bool found_dash_c = false;

        for (auto const &arg: service_specific_args) {
            auto key = arg.first;
            auto value = arg.second;

            if (key == "-N") {
                found_dash_N = true;
                unsigned long num_nodes;
                if ((sscanf(value.c_str(), "%lu", &num_nodes) != 1) or (num_nodes == 0)) {
                    throw std::invalid_argument("Invalid service-specific argument {\"" + key + "\",\"" + value + "\"}");
                }
                if (this->compute_hosts.size() < num_nodes) {
                    throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<ComputeService>()));
                }
            } else if (key == "-t") {
                found_dash_t = true;
                unsigned long requested_time;
                if ((sscanf(value.c_str(), "%lu", &requested_time) != 1) or (requested_time == 0)) {
                    throw std::invalid_argument("Invalid service-specific argument {\"" + key + "\",\"" + value + "\"}");
                }
            } else if (key == "-c") {
                found_dash_c = true;
                unsigned long num_cores;
                if ((sscanf(value.c_str(), "%lu", &num_cores) != 1) or (num_cores == 0)) {
                    throw std::invalid_argument("Invalid service-specific argument {\"" + key + "\",\"" + value + "\"}");
                }
                if (this->num_cores_per_node < num_cores) {
                    throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<ComputeService>()));
                }
                if (job->getMinimumRequiredNumCores() > num_cores) {
                    throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<ComputeService>()));
                }
            } else if (key == "-u" || key == "-color") {
                // nothing
            } else {
                // It has to be an action
                bool found_task = false;
                if (job != nullptr) {
                    for (auto const &action: job->getActions()) {
                        if (action->getName() == key) {
                            found_task = true;
                            break;
                        }
                    }
                }
                if (not found_task) {
                    throw std::invalid_argument("Invalid service-specific argument {" + key + "," + value + "}: Job does not have any task with name " + key);
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

        // Double check that memory requirements of all tasks can be met
        if (job->getMinimumRequiredMemory() > S4U_Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first)) {
            throw ExecutionException(std::make_shared<NotEnoughResources>(job, this->getSharedPtr<ComputeService>()));
        }
    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool BatchComputeService::supportsStandardJobs() {
        return true;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool BatchComputeService::supportsCompoundJobs() {
        return true;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool BatchComputeService::supportsPilotJobs() {
        return true;
    }

}// namespace wrench
