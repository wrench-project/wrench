/**
 * Copyright (c) 2017-2019. The WRENCH Team.
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
#include "helper_services/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/batch/BatchComputeService.h"
#include "wrench/services/compute/batch/BatchComputeServiceMessage.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/util/PointerUtil.h"
#include "wrench/util/TraceFileLoader.h"
#include "wrench/workflow/job/PilotJob.h"
#include "services/compute/batch/workload_helper_classes/WorkloadTraceFileReplayer.h"
#include "batch_schedulers/homegrown/fcfs/FCFSBatchScheduler.h"
#include "services/compute/batch/batch_schedulers/homegrown/conservative_bf/CONSERVATIVEBFBatchScheduler.h"
#include "batch_schedulers/batsched/BatschedBatchScheduler.h"
#include "wrench/workflow/failure_causes/JobTypeNotSupported.h"
#include "wrench/workflow/failure_causes/FunctionalityNotAvailable.h"
#include "wrench/workflow/failure_causes/JobKilled.h"
#include "wrench/workflow/failure_causes/NetworkError.h"
#include "wrench/workflow/failure_causes/NotEnoughResources.h"
#include "wrench/workflow/failure_causes/JobTimeout.h"
#include "wrench/workflow/failure_causes/NotAllowed.h"


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
    ) :
            BatchComputeService(hostname, std::move(compute_hosts), ComputeService::ALL_CORES,
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
                        "BatchComputeService::BatchComputeService(): Compute hosts for a batch service need to be homogeneous (different flop rates detected)");
            }
            // RAM
            if (std::abs(ram_available - Simulation::getHostMemoryCapacity(h)) > DBL_EPSILON) {

                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a batch service need to be homogeneous (different RAM capacities detected)");
            }
            // Num cores

            if (Simulation::getHostNumCores(h) != num_cores_available) {
                throw std::invalid_argument(
                        "BatchComputeService::BatchComputeService(): Compute hosts for a batch service need to be homogeneous (different RAM capacities detected)");

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

                this->workload_trace = TraceFileLoader::loadFromTraceFile(workload_file,
                                                                          this->getPropertyValueAsBoolean(BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE),
                                                                          this->getPropertyValueAsDouble(BatchComputeServiceProperty::SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE));
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
        auto batch_scheduling_alg = this->getPropertyValueAsString(BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM);
        if (this->scheduling_algorithms.find(batch_scheduling_alg) == this->scheduling_algorithms.end()) {
            throw std::invalid_argument(
                    " BatchComputeService::BatchComputeService(): unsupported scheduling algorithm " +  batch_scheduling_alg);
        }

        if (batch_scheduling_alg == "fcfs") {
            this->scheduler = std::unique_ptr<BatchScheduler>(new FCFSBatchScheduler(this));
        } else if (batch_scheduling_alg == "conservative_bf") {
            this->scheduler = std::unique_ptr<BatchScheduler>(new CONSERVATIVEBFBatchScheduler(this));
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
    std::map<std::string, double>
    BatchComputeService::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned long, unsigned long, double>> set_of_jobs) {

        try {
            auto estimates = this->scheduler->getStartTimeEstimates(set_of_jobs);
            return estimates;
        } catch (WorkflowExecutionException &e) {
            throw;
        } catch (std::exception &e) {
            throw WorkflowExecutionException(std::shared_ptr<FunctionalityNotAvailable>(
                    new FunctionalityNotAvailable(this->getSharedPtr<BatchComputeService>(), "start time estimates")));
        }
    }

    /**
    * @brief Gets the state of the batch queue
    * @return A vector of tuples:
    *              - std::string: username
    *              - int: job id
    *              - int: num hosts
    *              - int: num cores per host
    *              - int: time in seconds
    *              - double: submit time
    *              - double: start time (-1.0 if not started yet)
    */
    std::vector<std::tuple<std::string, int, int, int, int, double, double>> BatchComputeService::getQueue() {

        // Go through the currently running jobs
        std::vector<std::tuple<std::string, int, int, int, int, double, double>> queue_state;
        for (auto const &j : this->running_jobs) {
            auto tuple = std::make_tuple(
                    j->getUsername(),
                    j->getJobID(),
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
                    j->getJobID(),
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
                    j->getJobID(),
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
                  [](const std::tuple<std::string, int, int, int, int, double, double> j1,
                     const std::tuple<std::string, int, int, int, int, double, double> j2) -> bool {
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
    unsigned long BatchComputeService::parseUnsignedLongServiceSpecificArgument(std::string key,
                                                                                const std::map<std::string, std::string> &args) {
        unsigned long value = 0;
        auto it = args.find(key);
        if (it != args.end()) {
            if (sscanf((*it).second.c_str(), "%lu", &value) != 1) {
                throw std::invalid_argument(
                        "BatchComputeService::parseUnsignedLongServiceSpecificArgument(): Invalid " + key + " value '" + (*it).second + "'");
            }
        } else {
            throw std::invalid_argument(
                    "BatchComputeService::parseUnsignedLongServiceSpecificArgument(): Batch Service requires " + key + " argument to be specified for job submission"
            );
        }
        return value;
    }

    /**
     * @brief Helper function called by submitStandardJob() and submitWorkJob() to process a job submission
     * @param job
     * @param batch_job_args
     */
    void BatchComputeService::submitWorkflowJob(WorkflowJob *job, const std::map<std::string, std::string> &batch_job_args) {

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
        if ((num_hosts == 0) or (num_cores_per_host == 0) or  (time_asked_for_in_minutes == 0)) {
            throw std::invalid_argument("BatchComputeService::submitWorkflowJob(): service-specific arguments should have non-zero values");
        }

        // Create a Batch Job
        unsigned long jobid = this->generateUniqueJobID();
        auto batch_job = std::shared_ptr<BatchJob>(new BatchJob(job, jobid, time_asked_for_in_minutes,
                                                                num_hosts, num_cores_per_host, username,-1, S4U_Simulation::getClock()));

        // Set job display color for csv output
        auto it = batch_job_args.find("-color");
        if (it != batch_job_args.end()) {
            batch_job->csv_metadata = "color:" + (*it).second;
        }

        // Send a "run a batch job" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_standard_job_mailbox");
        try {
            S4U_Mailbox::dputMessage(this->mailbox_name,
                                     new BatchComputeServiceJobRequestMessage(
                                             answer_mailbox, batch_job,
                                             this->getMessagePayloadValue(
                                                     BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Standard Job?
        if (job->getType() == WorkflowJob::Type::STANDARD) {
            if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitStandardJobAnswerMessage>(message)) {
                // If no success, throw an exception
                if (not msg->success) {
                    throw WorkflowExecutionException(msg->failure_cause);
                }
                return;
            }
        }

        // Pilot Job?
        if (job->getType() == WorkflowJob::Type::PILOT) {
            if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitPilotJobAnswerMessage>(message)) {
                // If no success, throw an exception
                if (not msg->success) {
                    throw WorkflowExecutionException(msg->failure_cause);
                }
                return;
            }
        }

        throw std::runtime_error(
                "BatchComputeService::submitWorkflowJob(): Received an unexpected [" + message->getName() +
                "] message!");
    }


    /**
     * @brief Synchronously submit a standard job to the batch service
     *
     * @param job: a standard job
     * @param batch_job_args: batch-specific arguments
     *      - "-N": number of hosts
     *      - "-c": number of cores on each host
     *      - "-t": duration (in seconds)
     *      - "-u": username (optional)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     * @throw std::invalid_argument
     *
     */
    void BatchComputeService::submitStandardJob(StandardJob *job, const std::map<std::string, std::string> &batch_job_args) {

        try {
            this->submitWorkflowJob(job, batch_job_args);
        } catch (std::exception &e) {
            throw;
        }
    }

    /**
     * @brief Synchronously submit a pilot job to the compute service
     *
     * @param job: a pilot job
     * @param batch_job_args: batch-specific arguments
     *      - "-N": number of hosts
     *      - "-c": number of cores on each host
     *      - "-t": duration (in seconds)
     *      - "-u": username (optional)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void BatchComputeService::submitPilotJob(PilotJob *job, const std::map<std::string, std::string> &batch_job_args) {

        try {
            this->submitWorkflowJob(job, batch_job_args);
        } catch (std::exception &e) {
            throw;
        }
    }


    /**
     * @brief Helper function called by terminateStandardJob() and terminatePilotJob() to process a job submission
     * @param job
     */
    void BatchComputeService::terminateWorkflowJob(WorkflowJob *job) {

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_standard_job");

        // Send a "terminate a  job" message to the daemon's mailbox_name
        try {
            switch (job->getType()) {
                case WorkflowJob::Type::STANDARD: {
                    S4U_Mailbox::putMessage(this->mailbox_name,
                                            new ComputeServiceTerminateStandardJobRequestMessage(answer_mailbox,
                                                                                                 (StandardJob *) job,
                                                                                                 this->getMessagePayloadValue(
                                                                                                         BatchComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
                    break;
                }
                case WorkflowJob::Type::PILOT: {
                    S4U_Mailbox::putMessage(this->mailbox_name,
                                            new ComputeServiceTerminatePilotJobRequestMessage(answer_mailbox,
                                                                                              (PilotJob *) job,
                                                                                              this->getMessagePayloadValue(
                                                                                                      BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
                    break;
                }
            }
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        switch (job->getType()) {
            case WorkflowJob::Type::STANDARD: {
                if (auto msg = std::dynamic_pointer_cast<ComputeServiceTerminateStandardJobAnswerMessage>(message)) {
                    // If no success, throw an exception
                    if (not msg->success) {
                        throw WorkflowExecutionException(msg->failure_cause);
                    }
                    return;
                }
            }
            case WorkflowJob::Type::PILOT: {
                if (auto msg = std::dynamic_pointer_cast<ComputeServiceTerminatePilotJobAnswerMessage>(message)) {
                    // If no success, throw an exception
                    if (not msg->success) {
                        throw WorkflowExecutionException(msg->failure_cause);
                    }
                    return;
                }
            }

        }

        throw std::runtime_error("BatchComputeService::terminateWorkflowJob(): Received an unexpected [" +
                                 message->getName() +
                                 "] message!");
    }


    /**
     * @brief Terminate a standard job submitted to the compute service. Will throw a
     *        std::runtime_error exception if the job cannot be terminated, including
     *        if the cause is that the job is neither pending not running (perhaps alread
     *        terminated)
     *
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void BatchComputeService::terminateStandardJob(StandardJob *job) {

        try {
            this->terminateWorkflowJob(job);
        } catch (std::exception &e) {
            throw;
        }
    }


    /**
    * @brief Synchronously terminate a pilot job to the compute service
    *
    * @param job: a pilot job
    *
    * @throw WorkflowExecutionException
    * @throw std::runtime_error
    */
    void BatchComputeService::terminatePilotJob(PilotJob *job) {

        try {
            this->terminateWorkflowJob(job);
        } catch (std::exception &e) {
            throw;
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
    void BatchComputeService::sendPilotJobExpirationNotification(PilotJob *job) {
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
    void BatchComputeService::sendStandardJobFailureNotification(StandardJob *job, std::string job_id,
                                                                 std::shared_ptr<FailureCause> cause) {
        WRENCH_INFO("A standard job executor has failed because of timeout %s", job->getName().c_str());

        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto const &j : this->all_jobs) {
            if (j->getWorkflowJob() == job) {
                batch_job = j;
                break;
            }
        }

        this->scheduler->processJobFailure(batch_job);

        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServiceStandardJobFailedMessage(
                                            job, this->getSharedPtr<BatchComputeService>(), cause,
                                            this->getMessagePayloadValue(
                                                    BatchComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
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

        for (auto const &j: this->all_jobs) {
            if (j->getWorkflowJob() == job->getWorkflowJob()) {
                this->all_jobs.erase(j);
                break;
            }
        }
    }

    /**
     *
     * @param job
     */
    void BatchComputeService::processPilotJobTimeout(PilotJob *job) {
        auto cs = job->getComputeService();
        if (cs == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::terminate(): can't find compute service associated to pilot job");
        }
        try {
            cs->stop();
        } catch (wrench::WorkflowExecutionException &e) {
            return;
        }
    }

    /**
     *
     * @param job
     */
    void BatchComputeService::processStandardJobTimeout(StandardJob *job) {

        for (auto it = this->running_standard_job_executors.begin();
             it != this->running_standard_job_executors.end(); it++) {
            if ((*it)->getJob() == job) {
                (*it)->kill(false);
                // Make failed tasks ready again
                for (auto task : job->tasks) {
                    switch (task->getInternalState()) {
                        case WorkflowTask::InternalState::TASK_NOT_READY:
                        case WorkflowTask::InternalState::TASK_READY:
                        case WorkflowTask::InternalState::TASK_COMPLETED:
                            break;

                        case WorkflowTask::InternalState::TASK_RUNNING:
                            throw std::runtime_error(
                                    "BatchComputeService::processStandardJobTimeout: task state shouldn't be 'RUNNING'"
                                    "after a StandardJobExecutor was killed!");
                        case WorkflowTask::InternalState::TASK_FAILED:
                            // Making failed task READY again
                            task->setInternalState(WorkflowTask::InternalState::TASK_READY);
                            break;

                        default:
                            throw std::runtime_error(
                                    "BareMetalComputeService::terminateRunningStandardJob(): unexpected task state");

                    }
                }
                PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_standard_job_executors),
                                                       &(this->finished_standard_job_executors));
                break;
            }
        }
        this->finished_standard_job_executors.clear();

    }

    /**
    * @brief terminate a running standard job
    * @param job: the job
    */
    void BatchComputeService::terminateRunningStandardJob(StandardJob *job) {

        StandardJobExecutor *executor = nullptr;
        std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;
        for (it = this->running_standard_job_executors.begin();
             it != this->running_standard_job_executors.end(); it++) {
            if (((*it))->getJob() == job) {
                executor = (it->get());
            }
        }
        if (executor == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::terminateRunningStandardJob(): Cannot find standard job executor corresponding to job being terminated");
        }

        // Terminate the executor
        WRENCH_INFO("Terminating a standard job executor");
        executor->kill(true);
        // Do not update the resource availability, because this is done at a higher level

    }


    /**
    * @brief Declare all current jobs as failed (likely because the daemon is being terminated
    * or has timed out (because it's in fact a pilot job))
    */
    void BatchComputeService::failCurrentStandardJobs() {

        // LOCK
        this->acquireDaemonLock();

        WRENCH_INFO("Failing current standard jobs");

        {
            std::vector<std::shared_ptr<BatchJob>> to_erase;
            for (auto const &j : this->running_jobs) {
                WorkflowJob *workflow_job = j->getWorkflowJob();
                if (workflow_job->getType() == WorkflowJob::STANDARD) {
                    auto *job = (StandardJob *) workflow_job;
                    terminateRunningStandardJob(job);
                    this->sendStandardJobFailureNotification(job, std::to_string(j->getJobID()),
                                                             std::shared_ptr<FailureCause>(new JobKilled(workflow_job,
                                                                                                         this->getSharedPtr<BatchComputeService>())));
                    to_erase.push_back(j);
                }
            }

            for (auto const &j : to_erase) {
                this->running_jobs.erase(j);
                this->removeBatchJobFromJobsList(j);
            }
            to_erase.clear();
        }


        {
            std::vector<std::deque<std::shared_ptr<BatchJob>>::iterator> to_erase;

            for (auto it1 = this->batch_queue.begin(); it1 != this->batch_queue.end(); it1++) {
                WorkflowJob *workflow_job = (*it1)->getWorkflowJob();
                if (workflow_job->getType() == WorkflowJob::STANDARD) {
                    to_erase.push_back(it1);
                    auto *job = (StandardJob *) workflow_job;
                    this->sendStandardJobFailureNotification(job, std::to_string((*it1)->getJobID()),
                                                             std::shared_ptr<FailureCause>(new JobKilled(workflow_job,
                                                                                                         this->getSharedPtr<BatchComputeService>())));
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
                WorkflowJob *workflow_job = wj->getWorkflowJob();
                if (workflow_job->getType() == WorkflowJob::STANDARD) {
                    to_erase.push_back(wj);
                    auto *job = (StandardJob *) workflow_job;
                    this->sendStandardJobFailureNotification(job, std::to_string(wj->getJobID()),
                                                             std::shared_ptr<FailureCause>(new JobKilled(workflow_job,
                                                                                                         this->getSharedPtr<BatchComputeService>())));
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


    /**
     * @brief Terminate all running pilot jobs
     */
    void BatchComputeService::terminateRunningPilotJobs() {
        if (getPropertyValueAsBoolean(BatchComputeServiceProperty::SUPPORTS_PILOT_JOBS)) {
            WRENCH_INFO("Failing running pilot jobs");
            std::vector<std::shared_ptr<BatchJob>> to_erase;

            // LOCK
            this->acquireDaemonLock();

            // Stopping services
            for (auto &job : this->running_jobs) {
                if ((job)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                    auto p_job = (PilotJob *) ((job)->getWorkflowJob());
                    auto cs = p_job->getComputeService();
                    if (cs == nullptr) {
                        throw std::runtime_error(
                                "BatchComputeService::terminate(): can't find compute service associated to pilot job");
                    }
                    try {
                        cs->stop();
                    } catch (wrench::WorkflowExecutionException &e) {
                        // ignore
                    }
                    to_erase.push_back(job);
                }
            }

            // Cleaning up data structures
            for (auto &job : to_erase) {
                this->running_jobs.erase(job);
                this->removeBatchJobFromJobsList(job);
            }

            // UNLOCK
            this->releaseDaemonLock();
        }
    }

    /**
     * @brief Wait for and procress the next message
     * @return true if the service should keep going, false otherwise
     */
    bool BatchComputeService::processNextMessage() {

        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
            return false;
        }


        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());


        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            this->setStateToDown();
            this->failCurrentStandardJobs();
            this->terminateRunningPilotJobs();

            // Send back a synchronous reply!
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                BatchComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceResourceInformationRequestMessage>(message)) {
            processGetResourceInformation(msg->answer_mailbox);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<BatchComputeServiceJobRequestMessage>(message)) {
            processJobSubmission(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<StandardJobExecutorDoneMessage>(message)) {
            processStandardJobCompletion(msg->executor, msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<StandardJobExecutorFailedMessage>(message)) {
            processStandardJobFailure(msg->executor, msg->job, msg->cause);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceTerminateStandardJobRequestMessage>(message)) {
            processStandardJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServicePilotJobExpiredMessage>(message)) {
            processPilotJobCompletion(msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceTerminatePilotJobRequestMessage>(message)) {
            processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<AlarmJobTimeOutMessage>(message)) {
            processAlarmJobTimeout(msg->job);
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<BatchExecuteJobFromBatSchedMessage>(message)) {
            processExecuteJobFromBatSched(msg->batsched_decision_reply);
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

        // Check whether the job type is supported
        if ((job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) and
            (not getPropertyValueAsBoolean(BatchComputeServiceProperty::SUPPORTS_STANDARD_JOBS))) {
            S4U_Mailbox::dputMessage(answer_mailbox,
                                     new ComputeServiceSubmitStandardJobAnswerMessage(
                                             (StandardJob *) job->getWorkflowJob(),
                                             this->getSharedPtr<BatchComputeService>(),
                                             false,
                                             std::shared_ptr<FailureCause>(
                                                     new JobTypeNotSupported(
                                                             job->getWorkflowJob(),
                                                             this->getSharedPtr<BatchComputeService>())),
                                             this->getMessagePayloadValue(
                                                     BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        } else if ((job->getWorkflowJob()->getType() == WorkflowJob::PILOT) and
                   (not getPropertyValueAsBoolean(BatchComputeServiceProperty::SUPPORTS_PILOT_JOBS)
                   )) {
            S4U_Mailbox::dputMessage(answer_mailbox,
                                     new ComputeServiceSubmitPilotJobAnswerMessage(
                                             (PilotJob *) job->getWorkflowJob(),
                                             this->getSharedPtr<BatchComputeService>(),
                                             false,
                                             std::shared_ptr<FailureCause>(
                                                     new JobTypeNotSupported(
                                                             job->getWorkflowJob(),
                                                             this->getSharedPtr<BatchComputeService>())),
                                             this->getMessagePayloadValue(
                                                     BatchComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        // Check that the job can be admitted in terms of resources:
        //      - number of nodes,
        //      - number of cores per host
        //      - RAM
        unsigned long requested_hosts = job->getRequestedNumNodes();
        unsigned long requested_num_cores_per_host = job->getRequestedCoresPerNode();

        if (job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

            double required_ram_per_host = job->getMemoryRequirement();

            if ((requested_hosts > this->available_nodes_to_cores.size()) or
                (requested_num_cores_per_host >
                 Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first)) or
                (required_ram_per_host >
                 Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first))) {

                {
                    S4U_Mailbox::dputMessage(
                            answer_mailbox,
                            new ComputeServiceSubmitStandardJobAnswerMessage(
                                    (StandardJob *) job->getWorkflowJob(),
                                    this->getSharedPtr<BatchComputeService>(),
                                    false,
                                    std::shared_ptr<FailureCause>(
                                            new NotEnoughResources(
                                                    job->getWorkflowJob(),
                                                    this->getSharedPtr<BatchComputeService>())),
                                    this->getMessagePayloadValue(
                                            BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
                    return;
                }
            }
        }

        if ((requested_hosts > this->available_nodes_to_cores.size()) or
            (requested_num_cores_per_host >
             Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first))) {
            S4U_Mailbox::dputMessage(answer_mailbox,
                                     new ComputeServiceSubmitPilotJobAnswerMessage(
                                             (PilotJob *) job->getWorkflowJob(),
                                             this->getSharedPtr<BatchComputeService>(),
                                             false,
                                             std::shared_ptr<FailureCause>(
                                                     new NotEnoughResources(
                                                             job->getWorkflowJob(),
                                                             this->getSharedPtr<BatchComputeService>())),
                                             this->getMessagePayloadValue(
                                                     BatchComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        if (job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

            S4U_Mailbox::dputMessage(answer_mailbox,
                                     new ComputeServiceSubmitStandardJobAnswerMessage(
                                             (StandardJob *) job->getWorkflowJob(),
                                             this->getSharedPtr<BatchComputeService>(),
                                             true,
                                             nullptr,
                                             this->getMessagePayloadValue(
                                                     BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } else if (job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
            S4U_Mailbox::dputMessage(answer_mailbox,
                                     new ComputeServiceSubmitPilotJobAnswerMessage(
                                             (PilotJob *) job->getWorkflowJob(),
                                             this->getSharedPtr<BatchComputeService>(),
                                             true,
                                             nullptr,
                                             this->getMessagePayloadValue(
                                                     BatchComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        }

        // Add the RJMS delay to the job's requested time
        job->setRequestedTime(job->getRequestedTime() +
                              this->getPropertyValueAsUnsignedLong(BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY));
        this->all_jobs.insert(job);
        this->batch_queue.push_back(job);

        this->scheduler->processJobSubmission(job);
    }

    /**
     * @brief Process a pilot job completion
     *
     * @param job: the pilot job
     */
    void BatchComputeService::processPilotJobCompletion(PilotJob *job) {

        // Remove the job from the running job list
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto const &rj : this->running_jobs) {
            if (rj->getWorkflowJob() == job) {
                batch_job = rj;
                break;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::processPilotJobCompletion():  Pilot job completion message recevied but no such pilot jobs found in queue"
            );
        }

        this->removeJobFromRunningList(batch_job);
        this->freeUpResources(batch_job->getResourcesAllocated());
        if (this->pilot_job_alarms[job->getName()] != nullptr) {
            this->pilot_job_alarms[job->getName()]->kill();
            this->pilot_job_alarms.erase(job->getName());
        }

        // Let the scheduler know about the job completion
        this->scheduler->processJobCompletion(batch_job);

        // Forward the notification
        try {
            this->sendPilotJobExpirationNotification(job);
        } catch (std::runtime_error &e) {
            return;
        }
        return;
    }

    /**
     * @brief Process a pilot job termination request
     *
     * @param job: the job to terminate
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void BatchComputeService::processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox) {

        std::string job_id;
        for (auto it = this->batch_queue.begin(); it != this->batch_queue.end(); it++) {
            if ((*it)->getWorkflowJob() == job) {
                ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                S4U_Mailbox::dputMessage(answer_mailbox, answer_message);

                // notify scheduler of job termination
                this->scheduler->processJobTermination(*it);

                std::shared_ptr<BatchJob> to_erase = *it;
                this->batch_queue.erase(it);
                this->removeBatchJobFromJobsList(to_erase);
                return;
            }
        }

        for (auto it2 = this->waiting_jobs.begin(); it2 != this->waiting_jobs.end(); it2++) {
            if ((*it2)->getWorkflowJob() == job) {
                auto answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
                // forward this notification to batsched
                this->scheduler->processJobTermination(*it2);
                auto to_erase = *it2;
                this->waiting_jobs.erase(it2);
                this->removeBatchJobFromJobsList(to_erase);
                return;
            }
        }

        for (auto it1 = this->running_jobs.begin(); it1 != this->running_jobs.end();) {
            if ((*it1)->getWorkflowJob() == job) {
                this->processPilotJobTimeout((PilotJob *) (*it1)->getWorkflowJob());
                // Update the cores count in the available resources
                std::map<std::string, std::tuple<unsigned long, double>> resources = (*it1)->getResourcesAllocated();
                for (auto r : resources) {
                    this->available_nodes_to_cores[r.first] += std::get<0>(r.second);
                }
                auto answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                S4U_Mailbox::dputMessage(answer_mailbox, answer_message);


                this->scheduler->processJobTermination(*it1);

                //this is the list of unique pointers
                auto to_erase = *it1;
                this->running_jobs.erase(it1);
                this->removeBatchJobFromJobsList(to_erase);
                return;
            } else {
                ++it1;
            }
        }

        // If we got here, we're in trouble
        WRENCH_INFO("Trying to terminate a pilot job that's neither pending nor running!");
        std::string msg = "Job cannot be terminated because it's neither pending nor running";
        auto answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                job, this->getSharedPtr<BatchComputeService>(), false,
                std::shared_ptr<FailureCause>(new NotAllowed(this->getSharedPtr<BatchComputeService>(), msg)),
                this->getMessagePayloadValue(
                        BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
    }

    /**
     * @brief Process a standard job completion
     * @param executor: the standard job executor
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void
    BatchComputeService::processStandardJobCompletion(std::shared_ptr<StandardJobExecutor> executor, StandardJob *job) {
        bool executor_on_the_list = false;
        std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;

        for (it = this->running_standard_job_executors.begin();
             it != this->running_standard_job_executors.end(); it++) {
            if (*it == executor) {
                PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_standard_job_executors),
                                                       &(this->finished_standard_job_executors));
                executor_on_the_list = true;
                this->standard_job_alarms[job->getName()]->kill();
                this->standard_job_alarms.erase(job->getName());
                break;
            }
        }
        this->finished_standard_job_executors.clear();


        if (not executor_on_the_list) {
            WRENCH_WARN("BatchComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list - Likely getting wires crossed due to concurrent completion and time-outs.. ignoring")
            return;
//            throw std::runtime_error(
//                    "BatchComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }

        // Look for the corresponding batch job
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto const &rj : this->running_jobs) {
            if (rj->getWorkflowJob() == job) {
                batch_job = rj;
                break;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
        }

        WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());

//        std::cerr << "BEFORE FREEING UP RES\n";
//        for (auto r : this->compute_hosts) {
//            std::cerr << "-----> " << this->available_nodes_to_cores[r]  << "\n";
//        }

        // Free up resources (by finding the corresponding BatchJob)
        this->freeUpResources(batch_job->getResourcesAllocated());

        // Remove the job from the running job list
        this->removeJobFromRunningList(batch_job);



//        std::cerr << "AFTER FREEING UP RES\n";
//        for (auto r : this->compute_hosts) {
//            std::cerr << "-----> " << this->available_nodes_to_cores[r]  << "\n";
//        }
        // notify the scheduled of the job completion
        this->scheduler->processJobCompletion(batch_job);

        // Send the callback to the originator
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServiceStandardJobDoneMessage(
                                         job, this->getSharedPtr<BatchComputeService>(),
                                         this->getMessagePayloadValue(
                                                 BatchComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));

        //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
        // this job is not used anymore anywhere)
        this->removeBatchJobFromJobsList(batch_job);

        return;
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
     * @brief Process a work failure
     * @param worker_thread: the worker thread that did the work
     * @param work: the work
     * @param cause: the cause of the failure
     */
    void BatchComputeService::processStandardJobFailure(std::shared_ptr<StandardJobExecutor> executor,
                                                        StandardJob *job,
                                                        std::shared_ptr<FailureCause> cause) {

        bool executor_on_the_list = false;
        std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;
        for (it = this->running_standard_job_executors.begin();
             it != this->running_standard_job_executors.end(); it++) {
            if ((*it) == executor) {
                PointerUtil::moveSharedPtrFromSetToSet(it, &(this->running_standard_job_executors),
                                                       &(this->finished_standard_job_executors));
                executor_on_the_list = true;
                this->standard_job_alarms[job->getName()]->kill();
                this->standard_job_alarms.erase(job->getName());
                break;
            }
        }
        this->finished_standard_job_executors.clear();

        if (not executor_on_the_list) {
            throw std::runtime_error(
                    "BatchComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }

        // Free up resources (by finding the corresponding BatchJob)
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto const &rj : this->running_jobs) {
            if (rj->getWorkflowJob() == job) {
                batch_job = rj;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchComputeService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
        }

        this->freeUpResources(batch_job->getResourcesAllocated());
        this->removeJobFromRunningList(batch_job);

        WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

        // notify the scheduler of the failure
        this->scheduler->processJobFailure(batch_job);

        this->sendStandardJobFailureNotification(job, std::to_string((batch_job->getJobID())), cause);
        //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
        // this job is not used anymore anywhere)
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
     *
     * @param resources
     * @param workflow_job
     * @param batch_job
     * @param num_nodes_allocated
     * @param allocated_time: in seconds
     * @param cores_per_node_asked_for
     */
    void
    BatchComputeService::startJob(std::map<std::string, std::tuple<unsigned long, double>> resources,
                                  WorkflowJob *workflow_job,
                                  std::shared_ptr<BatchJob> batch_job, unsigned long num_nodes_allocated,
                                  unsigned long allocated_time,
                                  unsigned long cores_per_node_asked_for) {


        switch (workflow_job->getType()) {
            case WorkflowJob::STANDARD: {
                auto job = (StandardJob *) workflow_job;
                WRENCH_INFO("Creating a StandardJobExecutor for a standard job on %ld nodes with %ld cores per node",
                            num_nodes_allocated, cores_per_node_asked_for);
                // Create a standard job executor
                std::shared_ptr<StandardJobExecutor> executor = std::shared_ptr<StandardJobExecutor>(
                        new StandardJobExecutor(
                                this->simulation,
                                this->mailbox_name,
                                std::get<0>(*resources.begin()),
                                (StandardJob *) workflow_job,
                                resources,
                                this->getScratch(),
                                false,
                                nullptr,
                                {{StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD,
                                         this->getPropertyValueAsString(
                                                 BatchComputeServiceProperty::TASK_STARTUP_OVERHEAD)},
                                 {StandardJobExecutorProperty::SIMULATE_COMPUTATION_AS_SLEEP,
                                         this->getPropertyValueAsString(
                                                 BatchComputeServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP)},
                                 {StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM,
                                         this->getPropertyValueAsString(
                                                 BatchComputeServiceProperty::TASK_SELECTION_ALGORITHM)}
                                },
                                {}));
                executor->start(executor, true, false); // Daemonized, no auto-restart
                batch_job->setBeginTimestamp(S4U_Simulation::getClock());
                batch_job->setEndingTimestamp(S4U_Simulation::getClock() + allocated_time);
                this->running_standard_job_executors.insert(executor);

//          this->running_jobs.insert(std::move(batch_job_ptr));
                this->timeslots.push_back(batch_job->getEndingTimestamp());
                //remember the allocated resources for the job
                batch_job->setAllocatedResources(resources);

                SimulationMessage *msg =
                        new AlarmJobTimeOutMessage(batch_job, 0);

                std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
                                                                              batch_job->getEndingTimestamp(),
                                                                              this->hostname,
                                                                              this->mailbox_name, msg,
                                                                              "batch_standard");
                standard_job_alarms[job->getName()] = alarm_ptr;


                return;
            }

            case WorkflowJob::PILOT: {
                auto job = (PilotJob *) workflow_job;
                WRENCH_INFO("Allocating %ld nodes with %ld cores per node to a pilot job for %lu seconds",
                            num_nodes_allocated, cores_per_node_asked_for, allocated_time);

                std::vector<std::string> nodes_for_pilot_job = {};
                for (auto r : resources) {
                    nodes_for_pilot_job.push_back(std::get<0>(r));
                }
                std::string host_to_run_on = nodes_for_pilot_job[0];

                //set the ending timestamp of the batchjob (pilotjob)

                // Create and launch a compute service for the pilot job
                // (We use a TTL for user information purposes, but an alarm will take care of this)
                std::shared_ptr<ComputeService> cs = std::shared_ptr<ComputeService>(
                        new BareMetalComputeService(host_to_run_on,
                                                    resources,
                                                    {{BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                                                     {BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,    "false"}},
                                                    {},
                                                    allocated_time, job, "pilot_job", getScratch()
                        ));
                cs->simulation = this->simulation;
                job->setComputeService(cs);

                try {
                    cs->start(cs, true, false); // Daemonized, no auto-restart
                    batch_job->setBeginTimestamp(S4U_Simulation::getClock());
                    double ending_timestamp = S4U_Simulation::getClock() + (double)allocated_time;
                    batch_job->setEndingTimestamp(ending_timestamp);
                } catch (std::runtime_error &e) {
                    throw;
                }

                // Put the job in the running queue
//          this->running_jobs.insert(std::move(batch_job_ptr));
                this->timeslots.push_back(batch_job->getEndingTimestamp());

                //remember the allocated resources for the job
                batch_job->setAllocatedResources(resources);


                // Send the "Pilot job has started" callback
                // Note the getCallbackMailbox instead of the popCallbackMailbox, because
                // there will be another callback upon termination.
                S4U_Mailbox::dputMessage(job->getCallbackMailbox(),
                                         new ComputeServicePilotJobStartedMessage(
                                                 job, this->getSharedPtr<BatchComputeService>(),
                                                 this->getMessagePayloadValue(
                                                         BatchComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));

                SimulationMessage *msg =
                        new AlarmJobTimeOutMessage(batch_job, 0);

                std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
                                                                              batch_job->getEndingTimestamp(),
                                                                              host_to_run_on,
                                                                              this->mailbox_name, msg,
                                                                              "batch_pilot");

                this->pilot_job_alarms[job->getName()] = alarm_ptr;

                return;
            }
                break;
        }
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
        ComputeServiceResourceInformationAnswerMessage *answer_message = new ComputeServiceResourceInformationAnswerMessage(
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
                new WorkloadTraceFileReplayer(S4U_Simulation::getHostName(),
                                              this->getSharedPtr<BatchComputeService>(),
                                              this->num_cores_per_node,
                                              this->getPropertyValueAsBoolean(
                                                      BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE),
                                              this->workload_trace)
        );
        try {
            this->workload_trace_replayer->simulation = this->simulation;
            this->workload_trace_replayer->start(this->workload_trace_replayer, true,
                                                 false); // Daemonized, no auto-restart
        } catch (std::runtime_error &e) {
            throw;
        }
        return;
    }


/**
 * @brief Process a "terminate standard job message"
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void BatchComputeService::processStandardJobTerminationRequest(StandardJob *job,
                                                                   std::string answer_mailbox) {

        std::shared_ptr<BatchJob> batch_job = nullptr;
        // Is it running?
        bool is_running = false;
        for (auto const &j : this->running_jobs) {
            auto workflow_job = j->getWorkflowJob();
            if ((workflow_job->getType() == WorkflowJob::STANDARD) and ((StandardJob *) workflow_job == job)) {
                batch_job = j;
                is_running = true;
            }
        }

        bool is_pending = false;
        auto batch_pending_it = this->batch_queue.end();
        if (batch_job == nullptr) {
            // Is it pending?
            for (auto it1 = this->batch_queue.begin(); it1 != this->batch_queue.end(); it1++) {
                WorkflowJob *workflow_job = (*it1)->getWorkflowJob();
                if ((workflow_job->getType() == WorkflowJob::STANDARD) and ((StandardJob *) workflow_job == job)) {
                    batch_pending_it = it1;
                    is_pending = true;
                }
            }
        }

        bool is_waiting = false;

        if (batch_job == nullptr && batch_pending_it == this->batch_queue.end()) {
            // Is it waiting?
            for (auto const &j : this->waiting_jobs) {
                WorkflowJob *workflow_job = j->getWorkflowJob();
                if ((workflow_job->getType() == WorkflowJob::STANDARD) and ((StandardJob *) workflow_job == job)) {
                    batch_job = j;
                    is_waiting = true;
                }
            }
        }

        if (!is_pending && !is_running && !is_waiting) {
            std::string msg = "Job cannot be terminated because it is neither pending, not running, not waiting";
            // Send a failure reply
            ComputeServiceTerminateStandardJobAnswerMessage *answer_message =
                    new ComputeServiceTerminateStandardJobAnswerMessage(
                            job, this->getSharedPtr<BatchComputeService>(), false, std::shared_ptr<FailureCause>(
                                    new NotAllowed(this->getSharedPtr<BatchComputeService>(),
                                                   msg)),
                            this->getMessagePayloadValue(
                                    BatchComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
            return;
        }

        // Notify the scheduler of the failuire
        this->scheduler->processJobFailure(batch_job);

        // Is it running?
        if (is_running) {
            terminateRunningStandardJob(job);
            this->running_jobs.erase(batch_job);
            this->removeBatchJobFromJobsList(batch_job);
        }
        if (is_pending) {
            auto to_free = *batch_pending_it;
            this->batch_queue.erase(batch_pending_it);
            this->removeBatchJobFromJobsList(to_free);
        }
        if (is_waiting) {
            this->waiting_jobs.erase(batch_job);
            this->removeBatchJobFromJobsList(batch_job);
        }
        auto answer_message =
                new ComputeServiceTerminateStandardJobAnswerMessage(
                        job, this->getSharedPtr<BatchComputeService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                BatchComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);

    }


    /**
     * @brief Process a Batch job timeout
     * @param job: the batch job
     */
    void BatchComputeService::processAlarmJobTimeout(std::shared_ptr<BatchJob> job) {

        if (this->running_jobs.find(job) == this->running_jobs.end()) {
            WRENCH_INFO("BatchComputeService::processAlarmJobTimeout(): Received a time out message for an unknown batch job (%ld)... ignoring",
                        (unsigned long) job.get());
            return;
        }
        if (job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
            this->processStandardJobTimeout((StandardJob *) (job->getWorkflowJob()));
            this->removeJobFromRunningList(job);
            this->freeUpResources(job->getResourcesAllocated());
            this->sendStandardJobFailureNotification((StandardJob *) job->getWorkflowJob(),
                                                     std::to_string(job->getJobID()),
                                                     std::shared_ptr<FailureCause>(
                                                             new JobTimeout(job->getWorkflowJob())));
            return;
        } else if (job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
            WRENCH_INFO("Terminating pilot job %s", job->getWorkflowJob()->getName().c_str());
            auto *pilot_job = (PilotJob *) job->getWorkflowJob();
            auto cs = pilot_job->getComputeService();
            try {
                cs->stop();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "BatchComputeService::processAlarmJobTimeout(): Not able to terminate the pilot job"
                );
            }
            this->processPilotJobCompletion(pilot_job);
            return;
        } else {
            throw std::runtime_error(
                    "BatchComputeService::processAlarmJobTimeout(): Alarm about unknown job type " +
                    std::to_string(job->getWorkflowJob()->getType())
            );
        }
    }

    /**
     * @brief Method to hand incoming batsched message
     *
     * @param bat_sched_reply
     */
    void BatchComputeService::processExecuteJobFromBatSched(std::string bat_sched_reply) {
        nlohmann::json execute_events = nlohmann::json::parse(bat_sched_reply);
        WorkflowJob *workflow_job = nullptr;
        std::shared_ptr<BatchJob> batch_job = nullptr;
        for (auto it1 = this->waiting_jobs.begin(); it1 != this->waiting_jobs.end(); it1++) {
            if (std::to_string((*it1)->getJobID()) == execute_events["job_id"]) {
                batch_job = (*it1);
                workflow_job = batch_job->getWorkflowJob();
                this->waiting_jobs.erase(batch_job);
                this->running_jobs.insert(batch_job);
                break;
            }
        }
        if (workflow_job == nullptr) {
            //throw std::runtime_error("BatchComputeService::processExecuteJobFromBatSched(): Job received from batsched that does not belong to the list of jobs batchservice has");
            WRENCH_WARN("BatchComputeService::processExecuteJobFromBatSched(): Job received from batsched that does not belong to the list of known jobs... ignoring (Batsched seems to send this back even when a job has been actively terminated)");
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
            double ram_capacity = S4U_Simulation::getHostMemoryCapacity(this->host_id_to_names[node]); // Use the whole RAM
            this->available_nodes_to_cores[this->host_id_to_names[node]] -= cores_per_node_asked_for;
            resources.insert(std::make_pair(this->host_id_to_names[node],std::make_tuple( cores_per_node_asked_for,
                                                                                          ram_capacity)));
        }

        startJob(resources, workflow_job, batch_job, num_nodes_allocated, time_in_seconds,
                 cores_per_node_asked_for);
    }


};
