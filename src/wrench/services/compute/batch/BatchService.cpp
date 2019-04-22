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

#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/batch/BatchService.h"
#include "wrench/services/compute/batch/BatchServiceMessage.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/util/MessageManager.h"
#include "wrench/util/PointerUtil.h"
#include "wrench/util/TraceFileLoader.h"
#include "wrench/workflow/job/PilotJob.h"
#include "WorkloadTraceFileReplayer.h"

#ifdef ENABLE_BATSCHED

#include <zmq.hpp>
#include <zmq.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <fstream>

#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service, "Log category for Batch Service");

namespace wrench {

    /**
     * @brief Destructor
     */
    BatchService::~BatchService() {
    }

    /**
     * @brief Retrieve start time estimates for a set of job configurations
     * @param set_of_jobs: the set of job configurations, each of them with an id. Each configuration
     *         is a tuple as follows:
     *             - a configuration id (std::string)
     *             - a number of hosts (unsigned int)
     *             - a number of cores per host (unsigned int)
     *             - a duration in seconds (double)
     * @return start date predictions in seconds (as a map of ids). A prediction that's negative
     *         means that the job configuration can not run on the service (e.g., not enough hosts,
     *         not enough cores per host)
     */
    std::map<std::string, double>
    BatchService::getStartTimeEstimates(
            std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) {

#ifdef ENABLE_BATSCHED
        std::set<std::string> supported_algorithms = {"conservative_bf", "fast_conservative_bf", "fcfs_fast"};
        if (supported_algorithms.find(this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM)) != supported_algorithms.end()) {
            return getStartTimeEstimatesFromBatsched(set_of_jobs);
        } else {
            throw WorkflowExecutionException(std::shared_ptr<FunctionalityNotAvailable>(
                    new FunctionalityNotAvailable(this, "queue wait time prediction")));
        }
#else
        if ((this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM) == "FCFS") and
          (this->getPropertyValueAsString(BatchServiceProperty::HOST_SELECTION_ALGORITHM) == "FIRSTFIT")) {
        return getStartTimeEstimatesForFCFS(set_of_jobs);
      } else {
        throw WorkflowExecutionException(std::shared_ptr<FunctionalityNotAvailable>(
                new FunctionalityNotAvailable(this, "start time estimates")));
      }
#endif
    }

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param compute_hosts: the list of names of the available compute hosts
     *                 - the hosts must be homogeneous (speed, number of cores, and RAM size)
     *                 - all cores are usable by the batch service on each host
     *                 - all RAM is usable by the batch service on each host
     * @param scratch_space_size: the size for the scratch storage space for the service (0 means "no scratch space")
     * @param property_list: a property list that specifies BatchServiceProperty values ({} means "use all defaults")
     * @param messagepayload_list: a message payload list that specifies BatchServiceMessagePayload values ({} means "use all defaults")
     */
    BatchService::BatchService(std::string &hostname,
                               std::vector<std::string> compute_hosts,
                               double scratch_space_size,
                               std::map<std::string, std::string> property_list,
                               std::map<std::string, std::string> messagepayload_list
    ) :
            BatchService(hostname, std::move(compute_hosts), ComputeService::ALL_CORES,
                         ComputeService::ALL_RAM, scratch_space_size, std::move(property_list),
                         std::move(messagepayload_list), "") {}

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param compute_hosts: the list of names of the available compute hosts
     * @param cores_per_host: number of cores used per host
     *              - ComputeService::ALL_CORES to use all cores
     * @param ram_per_host: RAM per host (
     *              - ComputeService::ALL_RAM to use all RAM
     * @param scratch_space_size: the size for the scratch storage space for the service (0 means "no scratch space")
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     *
     * @throw std::invalid_argument
     */
    BatchService::BatchService(std::string hostname,
                               std::vector<std::string> compute_hosts,
                               unsigned long cores_per_host,
                               double ram_per_host,
                               double scratch_space_size,
                               std::map<std::string, std::string> property_list,
                               std::map<std::string, std::string> messagepayload_list,
                               std::string suffix) :
            ComputeService(hostname,
                           "batch" + suffix,
                           "batch" + suffix,
                           scratch_space_size) {

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        // Basic checks
        if (cores_per_host == 0) {
            throw std::invalid_argument("BatchService::BatchService(): compute hosts should have at least one core");
        }

        if (ram_per_host <= 0) {
            throw std::invalid_argument("BatchService::BatchService(): compute hosts should have at least some RAM");
        }

        if (compute_hosts.empty()) {
            throw std::invalid_argument("BatchService::BatchService(): at least one compute hosts must be provided");
        }

        // Check Platform homogeneity
        double num_cores_available = Simulation::getHostNumCores(*(compute_hosts.begin()));
        double speed = Simulation::getHostFlopRate(*(compute_hosts.begin()));
        double ram_available = Simulation::getHostMemoryCapacity(*(compute_hosts.begin()));

        for (auto const &h : compute_hosts) {
            // Compute speed
            if (std::abs(speed - Simulation::getHostFlopRate(h)) > DBL_EPSILON) {
                throw std::invalid_argument(
                        "BatchService::BatchService(): Compute hosts for a batch service need to be homogeneous (different flop rates detected)");
            }
            // RAM
            if (std::abs(ram_available - Simulation::getHostMemoryCapacity(h)) > DBL_EPSILON) {

                throw std::invalid_argument(
                        "BatchService::BatchService(): Compute hosts for a batch service need to be homogeneous (different RAM capacities detected)");
            }
            // Num cores

            if (Simulation::getHostNumCores(h) != num_cores_available) {
                throw std::invalid_argument(
                        "BatchService::BatchService(): Compute hosts for a batch service need to be homogeneous (different RAM capacities detected)");

            }
        }

        // Check that ALL_CORES and ALL_RAM, or actual maximum values, were passed
        // (this may change one day...)
        if (((cores_per_host != ComputeService::ALL_CORES) and (cores_per_host != num_cores_available)) or
            ((ram_per_host != ComputeService::ALL_RAM) and (ram_per_host != ram_available))) {
            throw std::invalid_argument(
                    "BatchService::BatchService(): A Batch Service must be given ALL core and RAM resources of hosts "
                    "(e.g., passing ComputeService::ALL_CORES and ComputeService::ALL_RAM)");
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
        std::string workload_file = this->getPropertyValueAsString(BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE);
        if (not workload_file.empty()) {
            try {
                this->workload_trace = TraceFileLoader::loadFromTraceFile(workload_file, 0);
            } catch (std::exception &e) {
                throw;
            }
            for (auto &j : this->workload_trace) {
                std::string id = std::get<0>(j);
                double submit_time = std::get<1>(j);
                double flops = std::get<2>(j);
                double requested_flops = std::get<3>(j);
                double requested_ram = std::get<4>(j);
                unsigned int num_nodes = std::get<5>(j);

                if (num_nodes > this->total_num_of_nodes) {
                    std::get<5>(j) = this->total_num_of_nodes;
                }
                if (requested_ram > ram_available) {
                    std::get<4>(j) = ram_available;
                }
            }
        }


#ifdef ENABLE_BATSCHED
        // Launch the Batsched process
        this->startBatsched();

        // Create the output CSV file if needed
        std::string output_csv_file = this->getPropertyValueAsString(BatchServiceProperty::OUTPUT_CSV_JOB_LOG);
        if (not output_csv_file.empty()) {
            std::ofstream file;
            file.open(output_csv_file, std::ios_base::out);
            if (not file) {
                throw std::invalid_argument("BatchService(): Unable to create CSV output file " +
                                            output_csv_file + " (as specified by the BatchServiceProperty::OUTPUT_CSV_JOB_LOG property)");
            }
        }
#else
        if (this->scheduling_algorithms.find(
              this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM))
          == this->scheduling_algorithms.end()) {
        throw std::invalid_argument(" BatchService::BatchService(): unsupported scheduling algorithm " +
                                    this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM));
      }
#endif

    }

    /**
     * @brief Helper function for service-specific job arguments
     * @param key: the argument key ("-N", "-c", "-t")
     * @param args: the argument map
     * @return the value of the argument
     * @throw std::invalid_argument
     */
    unsigned long BatchService::parseUnsignedLongServiceSpecificArgument(std::string key, std::map<std::string, std::string> &args) {
        unsigned long value = 0;
        auto it = args.find(key);
        if (it != args.end()) {
            if (sscanf((*it).second.c_str(), "%lu", &value) != 1) {
                throw std::invalid_argument(
                        "BatchService::submitStandardJob(): Invalid " + key + " value '" + (*it).second + "'");
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Batch Service requires -N argument to be specified for job submission"
            );
        }
        return value;
    }

    /**
     * @brief Helper function called by submitStandardJob() and submitWorkJob() to process a job submission
     * @param job
     * @param batch_job_args
     */
    void BatchService::submitWorkflowJob(WorkflowJob *job, std::map<std::string, std::string> &batch_job_args) {

        assertServiceIsUp();
//
//        if (this->state == Service::DOWN) {
//            throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
//        }

        // Get all arguments
        unsigned long num_hosts = 0;
        unsigned long num_cores_per_host = 0;
        unsigned long time_asked_for_in_minutes = 0;
        try {
            num_hosts = BatchService::parseUnsignedLongServiceSpecificArgument("-N", batch_job_args);
            num_cores_per_host = BatchService::parseUnsignedLongServiceSpecificArgument("-c", batch_job_args);
            time_asked_for_in_minutes = BatchService::parseUnsignedLongServiceSpecificArgument("-t", batch_job_args);
        } catch (std::invalid_argument &e) {
            throw;
        }

        // Create a Batch Job
        unsigned long jobid = this->generateUniqueJobID();
        auto *batch_job = new BatchJob(job, jobid, time_asked_for_in_minutes,
                                       num_hosts, num_cores_per_host, -1, S4U_Simulation::getClock());

        // Set job display color for csv output
        auto it = batch_job_args.find("-color");
        if (it != batch_job_args.end()) {
            batch_job->csv_metadata = "color:" + (*it).second;
        }

        // Send a "run a batch job" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_standard_job_mailbox");
        try {
            S4U_Mailbox::dputMessage(this->mailbox_name,
                                    new BatchServiceJobRequestMessage(
                                            answer_mailbox, batch_job,
                                            this->getMessagePayloadValueAsDouble(
                                                    BatchServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Standard Job?
        if (job->getType() == WorkflowJob::Type::STANDARD) {
            if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
                // If no success, throw an exception
                if (not msg->success) {
                    throw WorkflowExecutionException(msg->failure_cause);
                }
                return;
            }
        }

        // Pilot Job?
        if (job->getType() == WorkflowJob::Type::PILOT) {
            if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
                // If no success, throw an exception
                if (not msg->success) {
                    throw WorkflowExecutionException(msg->failure_cause);
                }
                return;
            }
        }

        throw std::runtime_error(
                "BatchService::submitWorkflowJob(): Received an unexpected [" + message->getName() +
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
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 * @throw std::invalid_argument
 *
 */
    void BatchService::submitStandardJob(StandardJob *job, std::map<std::string, std::string> &batch_job_args) {

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
 *
 * @throw WorkflowExecutionException
 * @throw std::runtime_error
 */
    void BatchService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &batch_job_args) {

        try {
            this->submitWorkflowJob(job, batch_job_args);
        } catch (std::exception &e) {
            throw;
        }
    }


    /**
     * @brief Helper function called by terminateStandardJob() and terminateWorkJob() to process a job submission
     * @param job
     */
    void BatchService::terminateWorkflowJob(WorkflowJob *job) {

        assertServiceIsUp();

//        if (this->state == Service::DOWN) {
//            throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
//        }

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_standard_job");

        // Send a "terminate a  job" message to the daemon's mailbox_name
        try {
            switch (job->getType()) {
                case WorkflowJob::Type::STANDARD: {
                    S4U_Mailbox::putMessage(this->mailbox_name,
                                            new ComputeServiceTerminateStandardJobRequestMessage(answer_mailbox, (StandardJob *)job,
                                                                                                 this->getMessagePayloadValueAsDouble(
                                                                                                         BatchServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
                    break;
                }
                case WorkflowJob::Type::PILOT: {
                    S4U_Mailbox::putMessage(this->mailbox_name,
                                            new ComputeServiceTerminatePilotJobRequestMessage(answer_mailbox, (PilotJob *)job,
                                                                                              this->getMessagePayloadValueAsDouble(
                                                                                                      BatchServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
                    break;
                }
            }
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        switch (job->getType()) {
            case WorkflowJob::Type::STANDARD: {
                if (auto msg = dynamic_cast<ComputeServiceTerminateStandardJobAnswerMessage *>(message.get())) {
                    // If no success, throw an exception
                    if (not msg->success) {
                        throw WorkflowExecutionException(msg->failure_cause);
                    }
                    return;
                }
            }
            case WorkflowJob::Type::PILOT: {
                if (auto msg = dynamic_cast<ComputeServiceTerminatePilotJobAnswerMessage *>(message.get())) {
                    // If no success, throw an exception
                    if (not msg->success) {
                        throw WorkflowExecutionException(msg->failure_cause);
                    }
                    return;
                }
            }

        }

        throw std::runtime_error("BatchService::terminateWorkflowJob(): Received an unexpected [" +
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
    void BatchService::terminateStandardJob(StandardJob *job) {

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
    void BatchService::terminatePilotJob(PilotJob *job) {

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
    int BatchService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("Batch Service starting");

#ifdef ENABLE_BATSCHED
        // Start the Batsched Network Listener
        try {
            this->startBatschedNetworkListener();
        } catch (std::runtime_error &e) {
            throw;
        }
#endif

        // Start the workload trace replayer if needed
        if (not this->workload_trace.empty()) {
            try {
                startBackgroundWorkloadProcess();
            } catch (std::runtime_error &e) {
                throw;
            }
        }

        /** Main loop **/
        bool keep_going = true;
        while (keep_going) {
            keep_going = processNextMessage();

#ifdef ENABLE_BATSCHED
            //        if (keep_going && this->isBatschedReady()) {
            if (keep_going) {
                this->sendAllQueuedJobsToBatsched();
            }
#else
            if (keep_going) {
          while (this->scheduleOneQueuedJob());
        }
#endif
        }

        return 0;
    }

/**
 * @brief Send back notification that a pilot job has expired
 * @param job
 */
    void BatchService::sendPilotJobExpirationNotification(PilotJob *job) {
        try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServicePilotJobExpiredMessage(
                                             job, this,
                                             this->getMessagePayloadValueAsDouble(
                                                     BatchServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            return; // ignore
        }
    }

/**
 * @brief Send back notification that a standard job has failed
 * @param job
 */
    void BatchService::sendStandardJobFailureNotification(StandardJob *job, std::string job_id, std::shared_ptr<FailureCause> cause) {
        WRENCH_INFO("A standard job executor has failed because of timeout %s", job->getName().c_str());

#ifdef ENABLE_BATSCHED
        this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "COMPLETED_FAILED", "", "JOB_COMPLETED");
        BatchJob *batch_job = nullptr;
        for (auto const &j : this->all_jobs) {
            if (j->getWorkflowJob() == job) {
                batch_job = j.get();
                break;
            }
        }
        this->appendJobInfoToCSVOutputFile(batch_job, "FAILED");


#endif

        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServiceStandardJobFailedMessage(
                                            job, this, cause,
                                            this->getMessagePayloadValueAsDouble(
                                                    BatchServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
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
    void BatchService::freeUpResources(std::map<std::string, std::tuple<unsigned long, double>> resources) {
        for (auto r : resources) {
            this->available_nodes_to_cores[r.first] += std::get<0>(r.second);
        }
    }

/**
 * @brief ...
 * @param job
 */
    void BatchService::removeJobFromRunningList(BatchJob *job) {
        if (this->running_jobs.find(job) == this->running_jobs.end()) {
            throw std::runtime_error("BatchService::removeJobFromRunningList(): Cannot find job!");
        }
        this->running_jobs.erase(job);
    }

/**
 *
 * @param job
 */
    void BatchService::freeJobFromJobsList(BatchJob *job) {
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
    void BatchService::processPilotJobTimeout(PilotJob *job) {
        auto cs = job->getComputeService();
        if (cs == nullptr) {
            throw std::runtime_error(
                    "BatchService::terminate(): can't find compute service associated to pilot job");
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
    void BatchService::processStandardJobTimeout(StandardJob *job) {
        std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;

        for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
            if (((*it).get())->getJob() == job) {
                ((*it).get())->kill(false);
                // Make failed tasks ready again
                for (auto task : job->tasks) {
                    switch (task->getInternalState()) {
                        case WorkflowTask::InternalState::TASK_NOT_READY:
                        case WorkflowTask::InternalState::TASK_READY:
                        case WorkflowTask::InternalState::TASK_COMPLETED:
                            break;

                        case WorkflowTask::InternalState::TASK_RUNNING:
                            throw std::runtime_error("BatchService::processStandardJobTimeout: task state shouldn't be 'RUNNING'"
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
    void BatchService::terminateRunningStandardJob(StandardJob *job) {

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
                    "BatchService::terminateRunningStandardJob(): Cannot find standard job executor corresponding to job being terminated");
        }

        // Terminate the executor
        WRENCH_INFO("Terminating a standard job executor");
        executor->kill(true);
        // Do not update the resource availability, because this is done at a higher level

    }

/**
 *
 * @param host_selection_algorithm
 * @param num_nodes
 * @param cores_per_node
 * @param ram_per_node
 * @return
 */
    std::map<std::string, std::tuple<unsigned long, double>>
    BatchService::scheduleOnHosts(std::string host_selection_algorithm,
                                  unsigned long num_nodes,
                                  unsigned long cores_per_node,
                                  double ram_per_node) {

        if (ram_per_node == ComputeService::ALL_RAM) {
            ram_per_node = Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first);
        }
        if (cores_per_node == ComputeService::ALL_CORES) {
            cores_per_node = Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first);
        }

        if (ram_per_node > Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("BatchService::scheduleOnHosts(): Asking for too much RAM per host");
        }
        if (num_nodes > this->available_nodes_to_cores.size()) {
            throw std::runtime_error("BatchService::scheduleOnHosts(): Asking for too many hosts");
        }
        if (cores_per_node > Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first)) {
            throw std::runtime_error("BatchService::scheduleOnHosts(): Asking for too many cores per host");
        }

        std::map<std::string, std::tuple<unsigned long, double>> resources = {};
        std::vector<std::string> hosts_assigned = {};
        if (host_selection_algorithm == "FIRSTFIT") {
            std::map<std::string, unsigned long>::iterator map_it;
            unsigned long host_count = 0;
            for (map_it = this->available_nodes_to_cores.begin();
                 map_it != this->available_nodes_to_cores.end(); map_it++) {
                if ((*map_it).second >= cores_per_node) {
                    //Remove that many cores from the available_nodes_to_core
                    (*map_it).second -= cores_per_node;
                    hosts_assigned.push_back((*map_it).first);
                    resources.insert(std::make_pair((*map_it).first, std::make_tuple(cores_per_node, ram_per_node)));
                    if (++host_count >= num_nodes) {
                        break;
                    }
                }
            }
            if (resources.size() < num_nodes) {
                resources = {};
                std::vector<std::string>::iterator vector_it;
                for (vector_it = hosts_assigned.begin(); vector_it != hosts_assigned.end(); vector_it++) {
                    available_nodes_to_cores[*vector_it] += cores_per_node;
                }
            }
        } else if (host_selection_algorithm == "BESTFIT") {
            while (resources.size() < num_nodes) {
                unsigned long target_slack = 0;
                std::string target_host = "";
                unsigned long target_num_cores = 0;

                for (auto h : this->available_nodes_to_cores) {
                    std::string hostname = std::get<0>(h);
                    unsigned long num_available_cores = std::get<1>(h);
                    if (num_available_cores < cores_per_node) {
                        continue;
                    }
                    unsigned long tentative_target_num_cores = std::min(num_available_cores, cores_per_node);
                    unsigned long tentative_target_slack =
                            num_available_cores - tentative_target_num_cores;

                    if (target_host.empty() ||
                        (tentative_target_num_cores > target_num_cores) ||
                        ((tentative_target_num_cores == target_num_cores) &&
                         (target_slack > tentative_target_slack))) {
                        target_host = hostname;
                        target_num_cores = tentative_target_num_cores;
                        target_slack = tentative_target_slack;
                    }
                }
                if (target_host.empty()) {
                    WRENCH_INFO("Didn't find a suitable host");
                    resources = {};
                    std::vector<std::string>::iterator it;
                    for (it = hosts_assigned.begin(); it != hosts_assigned.end(); it++) {
                        available_nodes_to_cores[*it] += cores_per_node;
                    }
                    break;
                }
                this->available_nodes_to_cores[target_host] -= cores_per_node;
                hosts_assigned.push_back(target_host);
                resources.insert(std::make_pair(target_host, std::make_tuple(cores_per_node, ComputeService::ALL_RAM)));
            }
        } else if (host_selection_algorithm == "ROUNDROBIN") {
            static unsigned long round_robin_host_selector_idx = 0;
            unsigned long cur_host_idx = round_robin_host_selector_idx;
            unsigned long host_count = 0;
            do {
                cur_host_idx = (cur_host_idx + 1) % available_nodes_to_cores.size();
                auto it = this->compute_hosts.begin();
                it = it + cur_host_idx;
                std::string cur_host_name = *it;
                unsigned long num_available_cores = available_nodes_to_cores[cur_host_name];
                if (num_available_cores >= cores_per_node) {
                    available_nodes_to_cores[cur_host_name] -= cores_per_node;
                    hosts_assigned.push_back(cur_host_name);
                    resources.insert(std::make_pair(cur_host_name, std::make_tuple(cores_per_node, ram_per_node)));
                    if (++host_count >= num_nodes) {
                        break;
                    }
                }
            } while (cur_host_idx != round_robin_host_selector_idx);
            if (resources.size() < num_nodes) {
                resources = {};
                std::vector<std::string>::iterator it;
                for (it = hosts_assigned.begin(); it != hosts_assigned.end(); it++) {
                    available_nodes_to_cores[*it] += cores_per_node;
                }
            } else {
                round_robin_host_selector_idx = cur_host_idx;
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::scheduleOnHosts(): We don't support " + host_selection_algorithm +
                    " as host selection algorithm"
            );
        }

        return resources;
    }

/**
 *
 * @param job_selection_algorithm
 * @return
 */
    BatchJob *BatchService::pickJobForScheduling(std::string job_selection_algorithm) {
        if (job_selection_algorithm == "FCFS") {
            BatchJob *batch_job = *this->pending_jobs.begin();
            return batch_job;
        } else {
            throw std::runtime_error("Unsupported batch scheduling algorithm " + job_selection_algorithm);
        }
        // Getting here is an error
        return nullptr;
    }

/**
 * @brief Schedule one queued job
 * @return true if a job was scheduled, false otherwise
 */
    bool BatchService::scheduleOneQueuedJob() {

        if (this->pending_jobs.empty()) {
            return false;
        }

        BatchJob *batch_job = pickJobForScheduling(
                this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM));
        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchService::scheduleAllQueuedJobs(): Found no job in pending queue to dispatch"
            );
        }
        WorkflowJob *workflow_job = batch_job->getWorkflowJob();

        /* Get the nodes and cores per nodes asked for */
        unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();
        unsigned long num_nodes_asked_for = batch_job->getNumNodes();
        unsigned long allocated_time = batch_job->getAllocatedTime();

//      WRENCH_INFO("Trying to see if I can run job (batch_job = %ld)(%s)",
//                  (unsigned long)batch_job,
//                  workflow_job->getName().c_str());

        //Try to schedule hosts based on host selection algorithm
        // Asking for the FULL RAM (TODO: Change this?)
        std::map<std::string, std::tuple<unsigned long, double>> resources = this->scheduleOnHosts(
                this->getPropertyValueAsString(BatchServiceProperty::HOST_SELECTION_ALGORITHM),
                num_nodes_asked_for, cores_per_node_asked_for, ComputeService::ALL_RAM);


        if (resources.empty()) {
            WRENCH_INFO("Can't run job %s right now", workflow_job->getName().c_str());
            return false;
        }

        WRENCH_INFO("Running job %s", workflow_job->getName().c_str());

        // Remove it from the pending list
        for (auto it = this->pending_jobs.begin(); it != this->pending_jobs.end(); it++) {
            if ((*it) == batch_job) {
                this->pending_jobs.erase(it);
                break;
            }
        }

        this->running_jobs.insert(batch_job);

        startJob(resources, workflow_job, batch_job, num_nodes_asked_for, allocated_time,
                 cores_per_node_asked_for);
        return true;

    }

/**
* @brief Declare all current jobs as failed (likely because the daemon is being terminated
* or has timed out (because it's in fact a pilot job))
*/
    void BatchService::failCurrentStandardJobs() {

        // LOCK
        this->acquireDaemonLock();

        WRENCH_INFO("Failing current standard jobs");

        {
            std::vector<BatchJob *> to_erase;
            for (auto const &j : this->running_jobs) {
                WorkflowJob *workflow_job = j->getWorkflowJob();
                if (workflow_job->getType() == WorkflowJob::STANDARD) {
                    auto *job = (StandardJob *) workflow_job;
                    terminateRunningStandardJob(job);
                    this->sendStandardJobFailureNotification(job, std::to_string(j->getJobID()),
                                                             std::shared_ptr<FailureCause>(new JobKilled(workflow_job, this)));
                    to_erase.push_back(j);
                }
            }

            for (auto const &j : to_erase) {
                this->running_jobs.erase(j);
                this->freeJobFromJobsList(j);
            }
            to_erase.clear();
        }


        {
            std::vector<std::deque<BatchJob *>::iterator> to_erase;

            for (auto it1 = this->pending_jobs.begin(); it1 != this->pending_jobs.end(); it1++) {
                WorkflowJob *workflow_job = (*it1)->getWorkflowJob();
                if (workflow_job->getType() == WorkflowJob::STANDARD) {
                    to_erase.push_back(it1);
                    auto *job = (StandardJob *) workflow_job;
                    this->sendStandardJobFailureNotification(job, std::to_string((*it1)->getJobID()),
                                                             std::shared_ptr<FailureCause>(new JobKilled(workflow_job, this)));
                }
            }

            for (auto const &j : to_erase) {
                this->pending_jobs.erase(j);
                this->freeJobFromJobsList(*j);
            }
            to_erase.clear();
        }

        {
            std::vector<BatchJob *> to_erase;

            for (auto const &wj : this->waiting_jobs) {
                WorkflowJob *workflow_job = wj->getWorkflowJob();
                if (workflow_job->getType() == WorkflowJob::STANDARD) {
                    to_erase.push_back(wj);
                    auto *job = (StandardJob *) workflow_job;
                    this->sendStandardJobFailureNotification(job, std::to_string(wj->getJobID()),
                                                             std::shared_ptr<FailureCause>(new JobKilled(workflow_job, this)));
                }
            }

            for (auto const &j : to_erase) {
                this->waiting_jobs.erase(j);
                this->freeJobFromJobsList(j);
            }
            to_erase.clear();
        }

        // UNLOCK
        this->releaseDaemonLock();
    }


    /**
    * @brief Terminate the daemon, dealing with pending/running jobs
    */
    void BatchService::cleanup(bool has_returned_from_main, int return_value) {

#ifdef ENABLE_BATSCHED
        // Kill the batsched process
        this->stopBatsched();
#endif
        // Do the default behavior (which will throw as this is not a fault-tolerant service)
        Service::cleanup(has_returned_from_main, return_value);

    }


/**
 * @brief Terminate all running pilot jobs
 */
    void BatchService::terminateRunningPilotJobs() {
        if (getPropertyValueAsBoolean(BatchServiceProperty::SUPPORTS_PILOT_JOBS)) {
            WRENCH_INFO("Failing running pilot jobs");
            std::vector<BatchJob *> to_erase;

            // LOCK
            this->acquireDaemonLock();

            // Stopping services
            for (auto &job : this->running_jobs) {
                if ((job)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                    auto p_job = (PilotJob *) ((job)->getWorkflowJob());
                    auto cs = p_job->getComputeService();
                    if (cs == nullptr) {
                        throw std::runtime_error(
                                "BatchService::terminate(): can't find compute service associated to pilot job");
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
                this->freeJobFromJobsList(job);
            }

            // UNLOCK
            this->releaseDaemonLock();
        }
    }

/**
 * @brief Wait for and procress the next message
 * @return true if the service should keep going, false otherwise
 */
    bool BatchService::processNextMessage() {

        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;

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


        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->setStateToDown();
            this->failCurrentStandardJobs();
            this->terminateRunningPilotJobs();

            // Send back a synchronous reply!
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValueAsDouble(
                                                BatchServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
            processGetResourceInformation(msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<BatchServiceJobRequestMessage *>(message.get())) {
            processJobSubmission(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
            processStandardJobCompletion(msg->executor, msg->job);
            return true;

        } else if (auto msg = dynamic_cast<StandardJobExecutorFailedMessage *>(message.get())) {
            processStandardJobFailure(msg->executor, msg->job, msg->cause);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceTerminateStandardJobRequestMessage *>(message.get())) {
            processStandardJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
            processPilotJobCompletion(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {
            processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<AlarmJobTimeOutMessage *>(message.get())) {
            if (this->running_jobs.find(msg->job) == this->running_jobs.end()) {
                WRENCH_INFO("Received a time out message (%ld) for an unknown batch job (%ld)... ignoring",
                            (unsigned long) (message.get()),
                            (unsigned long) msg->job);
//          WRENCH_INFO("----> %ld", (unsigned long) msg->job->getWorkflowJob());
                return true;
            }
            if (msg->job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
                this->processStandardJobTimeout((StandardJob *) (msg->job->getWorkflowJob()));
                this->removeJobFromRunningList(msg->job);
                this->freeUpResources(msg->job->getResourcesAllocated());
                this->sendStandardJobFailureNotification((StandardJob *) msg->job->getWorkflowJob(),
                                                         std::to_string(msg->job->getJobID()),
                                                         std::shared_ptr<FailureCause>(new JobTimeout(msg->job->getWorkflowJob())));
                return true;
            } else if (msg->job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                WRENCH_INFO("Terminating pilot job %s", msg->job->getWorkflowJob()->getName().c_str());
                auto *pilot_job = (PilotJob *) msg->job->getWorkflowJob();
                ComputeService *cs = pilot_job->getComputeService();
                try {
                    cs->stop();
                } catch (wrench::WorkflowExecutionException &e) {
                    throw std::runtime_error(
                            "BatchService::processNextMessage(): Not able to terminate the pilot job"
                    );
                }
                this->processPilotJobCompletion(pilot_job);
                return true;
            } else {
                throw std::runtime_error(
                        "BatchService::processNextMessage(): Alarm about unknown job type " +
                        std::to_string(msg->job->getWorkflowJob()->getType())
                );
            }

#ifdef ENABLE_BATSCHED
            //      } else if (auto msg = dynamic_cast<BatchSchedReadyMessage *>(message.get())) {
//        is_bat_sched_ready = true;
//        return true;

        } else if (auto msg = dynamic_cast<BatchExecuteJobFromBatSchedMessage *>(message.get())) {
            processExecuteJobFromBatSched(msg->batsched_decision_reply);
            return true;
//      } else if (auto msg = dynamic_cast<AlarmNotifyBatschedMessage *>(message.get())) {
//        //first forward this notification to the batsched
//        this->notifyJobEventsToBatSched(msg->job_id, "SUCCESS", "COMPLETED_SUCCESSFULLY", "");
//        return true;
#endif
        } else {
            throw std::runtime_error(
                    "BatchService::processNextMessage(): Unknown message type: " +
                    std::to_string(message->payload));
            return false;
        }
    }

/**
 * @brief Process a job submission
 *
 * @param job: the batch job object
 * @param answer_mailbox: the mailbox to which answer messages should be sent
 */
    void BatchService::processJobSubmission(BatchJob *job, std::string answer_mailbox) {

        WRENCH_INFO("Asked to run a batch job with id %ld", job->getJobID());

        auto unique_ref_to_batch_job  = std::unique_ptr<BatchJob>(job); // for freeing upon return

        // Check whether the job type is supported
        if ((job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) and (not getPropertyValueAsBoolean(BatchServiceProperty::SUPPORTS_STANDARD_JOBS))) {
            try {
                S4U_Mailbox::dputMessage(answer_mailbox,
                                         new ComputeServiceSubmitStandardJobAnswerMessage(
                                                 (StandardJob *) job->getWorkflowJob(),
                                                 this,
                                                 false,
                                                 std::shared_ptr<FailureCause>(
                                                         new JobTypeNotSupported(
                                                                 job->getWorkflowJob(),
                                                                 this)),
                                                 this->getMessagePayloadValueAsDouble(
                                                         BatchServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
            return;
        } else if ((job->getWorkflowJob()->getType() == WorkflowJob::PILOT) and (not getPropertyValueAsBoolean(BatchServiceProperty::SUPPORTS_PILOT_JOBS)
        )) {
            try {
                S4U_Mailbox::dputMessage(answer_mailbox,
                                         new ComputeServiceSubmitPilotJobAnswerMessage(
                                                 (PilotJob *) job->getWorkflowJob(),
                                                 this,
                                                 false,
                                                 std::shared_ptr<FailureCause>(
                                                         new JobTypeNotSupported(
                                                                 job->getWorkflowJob(),
                                                                 this)),
                                                 this->getMessagePayloadValueAsDouble(
                                                         BatchServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
            return;
        }

        // Check that the job can be admitted in terms of resources:
        //      - number of nodes,
        //      - number of cores per host
        //      - RAM
        unsigned long requested_hosts = job->getNumNodes();
        unsigned long requested_num_cores_per_host = job->getAllocatedCoresPerNode();

        if (job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

            double required_ram_per_host = job->getMemoryRequirement();

            if ((requested_hosts > this->available_nodes_to_cores.size()) or
                (requested_num_cores_per_host >
                 Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first)) or
                (required_ram_per_host >
                 Simulation::getHostMemoryCapacity(this->available_nodes_to_cores.begin()->first))) {

                {
                    try {
                        S4U_Mailbox::dputMessage(
                                answer_mailbox,
                                new ComputeServiceSubmitStandardJobAnswerMessage(
                                        (StandardJob *) job->getWorkflowJob(),
                                        this,
                                        false,
                                        std::shared_ptr<FailureCause>(
                                                new NotEnoughResources(
                                                        job->getWorkflowJob(),
                                                        this)),
                                        this->getMessagePayloadValueAsDouble(
                                                BatchServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
                    } catch (std::shared_ptr<NetworkError> &cause) {}
                    return;
                }
            }
        }

        if ((requested_hosts > this->available_nodes_to_cores.size()) or
            (requested_num_cores_per_host >
             Simulation::getHostNumCores(this->available_nodes_to_cores.begin()->first))) {
            try {
                S4U_Mailbox::dputMessage(answer_mailbox,
                                         new ComputeServiceSubmitPilotJobAnswerMessage(
                                                 (PilotJob *) job->getWorkflowJob(),
                                                 this,
                                                 false,
                                                 std::shared_ptr<FailureCause>(
                                                         new NotEnoughResources(
                                                                 job->getWorkflowJob(),
                                                                 this)),
                                                 this->getMessagePayloadValueAsDouble(
                                                         BatchServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {}
            return;
        }

        if (job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

            try {
                S4U_Mailbox::dputMessage(answer_mailbox,
                                         new ComputeServiceSubmitStandardJobAnswerMessage(
                                                 (StandardJob *) job->getWorkflowJob(), this,
                                                 true,
                                                 nullptr,
                                                 this->getMessagePayloadValueAsDouble(
                                                         BatchServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
        } else if (job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
            try {
                S4U_Mailbox::dputMessage(answer_mailbox,
                                         new ComputeServiceSubmitPilotJobAnswerMessage(
                                                 (PilotJob *) job->getWorkflowJob(), this,
                                                 true,
                                                 nullptr,
                                                 this->getMessagePayloadValueAsDouble(
                                                         BatchServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
        }

        // Add the RJMS delay to the job's requested time
        job->setAllocatedTime(job->getAllocatedTime() +
                              this->getPropertyValueAsUnsignedLong(BatchServiceProperty::BATCH_RJMS_DELAY));
        this->all_jobs.insert(std::move(unique_ref_to_batch_job));
        this->pending_jobs.push_back(job);
    }

/**
 * @brief Process a pilot job completion
 *
 * @param job: the pilot job
 */
    void BatchService::processPilotJobCompletion(PilotJob *job) {

        // Remove the job from the running job list
        BatchJob *batch_job = nullptr;
        for (auto const &rj : this->running_jobs) {
            if (rj->getWorkflowJob() == job) {
                batch_job = rj;
                break;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchService::processPilotJobCompletion():  Pilot job completion message recevied but no such pilot jobs found in queue"
            );
        }

        this->removeJobFromRunningList(batch_job);
        this->freeUpResources(batch_job->getResourcesAllocated());
        if (this->pilot_job_alarms[job->getName()] != nullptr) {
            this->pilot_job_alarms[job->getName()]->kill();
            this->pilot_job_alarms.erase(job->getName());
        }

        //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
        this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "SUCCESS", "COMPLETED_SUCCESSFULLY", "", "JOB_COMPLETED");
        this->appendJobInfoToCSVOutputFile(batch_job, "success");
#endif

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
    void BatchService::processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox) {

        std::string job_id;
        for (auto it = this->pending_jobs.begin(); it != this->pending_jobs.end(); it++) {
            if ((*it)->getWorkflowJob() == job) {
                job_id = std::to_string((*it)->getJobID());
                ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getMessagePayloadValueAsDouble(
                                BatchServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                try {
                    S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return;
                }
#ifdef ENABLE_BATSCHED
                // forward this notification to batsched
                this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "NOT_SUBMITTED", "", "JOB_COMPLETED");
                this->appendJobInfoToCSVOutputFile(*it, "TERMINATED");
#endif
                BatchJob *to_erase = *it;
                this->pending_jobs.erase(it);
                this->freeJobFromJobsList(to_erase);
                return;
            }
        }

        for (auto it2 = this->waiting_jobs.begin(); it2 != this->waiting_jobs.end(); it2++) {
            if ((*it2)->getWorkflowJob() == job) {
                job_id = std::to_string((*it2)->getJobID());
                ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getMessagePayloadValueAsDouble(
                                BatchServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                try {
                    S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return;
                }
#ifdef ENABLE_BATSCHED
                // forward this notification to batsched
                this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "NOT_SUBMITTED", "", "JOB_COMPLETED");
                this->appendJobInfoToCSVOutputFile(*it2, "TERMINATED");
#endif
                BatchJob *to_erase = *it2;
                this->waiting_jobs.erase(it2);
                this->freeJobFromJobsList(to_erase);
                return;
            }
        }

        for (auto it1 = this->running_jobs.begin(); it1 != this->running_jobs.end();) {
            if ((*it1)->getWorkflowJob() == job) {
                job_id = std::to_string((*it1)->getJobID());
                this->processPilotJobTimeout((PilotJob *) (*it1)->getWorkflowJob());
                // Update the cores count in the available resources
                std::map<std::string, std::tuple<unsigned long, double>> resources = (*it1)->getResourcesAllocated();
                for (auto r : resources) {
                    this->available_nodes_to_cores[r.first] += std::get<0>(r.second);
                }
                ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getMessagePayloadValueAsDouble(
                                BatchServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                try {
                    S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return;
                }
                //this is the list of raw pointers


#ifdef ENABLE_BATSCHED
                // forward this notification to batsched
                this->notifyJobEventsToBatSched(job_id, "TIMEOUT", "COMPLETED_FAILED", "", "JOB_COMPLETED");
                this->appendJobInfoToCSVOutputFile(*it1, "TERMINATED");

#endif
                //this is the list of unique pointers
                BatchJob *to_erase = *it1;
                this->running_jobs.erase(it1);
                this->freeJobFromJobsList(to_erase);
                return;
            } else {
                ++it1;
            }
        }

        // If we got here, we're in trouble
        WRENCH_INFO("Trying to terminate a pilot job that's neither pending nor running!");
        ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                job, this, false, std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
                this->getMessagePayloadValueAsDouble(
                        BatchServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
        try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return;
        }
    }

/**
 * @brief Process a standard job completion
 * @param executor: the standard job executor
 * @param job: the job
 *
 * @throw std::runtime_error
 */
    void BatchService::processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job) {
        bool executor_on_the_list = false;
        std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;
        for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
            if ((*it).get() == executor) {
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
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }

        // Look for the corresponding batch job
        BatchJob *batch_job = nullptr;
        for (auto const &rj : this->running_jobs) {
            if (rj->getWorkflowJob() == job) {
                batch_job = rj;
                break;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
        }

        // Free up resources (by finding the corresponding BatchJob)
        this->freeUpResources(batch_job->getResourcesAllocated());

        // Remove the job from the running job list
        this->removeJobFromRunningList(batch_job);


        WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());

        //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
        this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "SUCCESS", "COMPLETED_SUCCESSFULLY", "", "JOB_COMPLETED");
        this->appendJobInfoToCSVOutputFile(batch_job, "COMPLETED");
#endif

        // Send the callback to the originator
        try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServiceStandardJobDoneMessage(
                                             job, this,
                                             this->getMessagePayloadValueAsDouble(
                                                     BatchServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            return;
        }

        //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
        // this job is not used anymore anywhere)
        this->freeJobFromJobsList(batch_job);

        return;
    }

/**
 * @brief Process a work failure
 * @param worker_thread: the worker thread that did the work
 * @param work: the work
 * @param cause: the cause of the failure
 */
    void BatchService::processStandardJobFailure(StandardJobExecutor *executor,
                                                 StandardJob *job,
                                                 std::shared_ptr<FailureCause> cause) {

        bool executor_on_the_list = false;
        std::set<std::shared_ptr<StandardJobExecutor>>::iterator it;
        for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
            if ((*it).get() == executor) {
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
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }

        // Free up resources (by finding the corresponding BatchJob)
        BatchJob *batch_job = nullptr;
        for (auto const &rj : this->running_jobs) {
            if (rj->getWorkflowJob() == job) {
                batch_job = rj;
            }
        }

        if (batch_job == nullptr) {
            throw std::runtime_error(
                    "BatchService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
        }

        this->freeUpResources(batch_job->getResourcesAllocated());
        this->removeJobFromRunningList(batch_job);


        WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

        //first forward this notification to the batsched
#ifdef ENABLE_BATSCHED
        this->notifyJobEventsToBatSched(std::to_string(batch_job->getJobID()), "TIMEOUT", "COMPLETED_FAILED", "", "JOB_COMPLETED");
        this->appendJobInfoToCSVOutputFile(batch_job, "COMPLETED");
#endif

        this->sendStandardJobFailureNotification(job, std::to_string((batch_job->getJobID())), cause);
        //Free the job from the global (pending, running, waiting) job list, (doing this at the end of this method to make sure
        // this job is not used anymore anywhere)
        this->freeJobFromJobsList(batch_job);

    }

/**
 * @brief
 * @return
 */
    unsigned long BatchService::generateUniqueJobID() {
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
    BatchService::startJob(std::map<std::string, std::tuple<unsigned long, double>> resources,
                           WorkflowJob *workflow_job,
                           BatchJob *batch_job, unsigned long num_nodes_allocated,
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
                                {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,
                                         this->getPropertyValueAsString(
                                                 BatchServiceProperty::THREAD_STARTUP_OVERHEAD)},
                                 {StandardJobExecutorProperty::SIMULATE_COMPUTATION_AS_SLEEP,
                                         this->getPropertyValueAsString(
                                                 BatchServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP)},
                                 {StandardJobExecutorProperty::TASK_SELECTION_ALGORITHM,
                                         this->getPropertyValueAsString(
                                                 BatchServiceProperty::TASK_SELECTION_ALGORITHM)}
                                },
                                {}));
                executor->start(executor, true, false); // Daemonized, no auto-restart
                batch_job->setBeginTimeStamp(S4U_Simulation::getClock());
                batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + allocated_time);
                this->running_standard_job_executors.insert(executor);

//          this->running_jobs.insert(std::move(batch_job_ptr));
                this->timeslots.push_back(batch_job->getEndingTimeStamp());
                //remember the allocated resources for the job
                batch_job->setAllocatedResources(resources);

                SimulationMessage *msg =
                        new AlarmJobTimeOutMessage(batch_job, 0);

                std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
                                                                              batch_job->getEndingTimeStamp(), this->hostname,
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
                                                     {BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}},
                                                    {},
                                                    allocated_time,  job, "pilot_job", getScratch()
                        ));
                cs->simulation = this->simulation;
                job->setComputeService(cs);

                try {
                    cs->start(cs, true, false); // Daemonized, no auto-restart
                    batch_job->setBeginTimeStamp(S4U_Simulation::getClock());
                    double timeout_timestamp = (double)allocated_time;
                    batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + timeout_timestamp);
                } catch (std::runtime_error &e) {
                    throw;
                }

                // Put the job in the running queue
//          this->running_jobs.insert(std::move(batch_job_ptr));
                this->timeslots.push_back(batch_job->getEndingTimeStamp());

                //remember the allocated resources for the job
                batch_job->setAllocatedResources(resources);


                // Send the "Pilot job has started" callback
                // Note the getCallbackMailbox instead of the popCallbackMailbox, because
                // there will be another callback upon termination.
                try {
                    S4U_Mailbox::dputMessage(job->getCallbackMailbox(),
                                             new ComputeServicePilotJobStartedMessage(
                                                     job, this,
                                                     this->getMessagePayloadValueAsDouble(
                                                             BatchServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
                } catch (std::shared_ptr<NetworkError> &cause) {
                    throw WorkflowExecutionException(cause);
                }

                SimulationMessage *msg =
                        new AlarmJobTimeOutMessage(batch_job, 0);

                std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(this->simulation,
                                                                              batch_job->getEndingTimeStamp(), host_to_run_on,
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
    void BatchService::processGetResourceInformation(const std::string &answer_mailbox) {
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
                this->getMessagePayloadValueAsDouble(
                        ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
        try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return;
        }
    }

/**
 * @brief Start the process that will replay the background load
 *
 * @throw std::runtime_error
 */
    void BatchService::startBackgroundWorkloadProcess() {
        if (this->workload_trace.empty()) {
            throw std::runtime_error("BatchService::startBackgroundWorkloadProcess(): no workload trace file specified");
        }

        // Create the trace replayer process
        this->workload_trace_replayer = std::shared_ptr<WorkloadTraceFileReplayer>(
                new WorkloadTraceFileReplayer(S4U_Simulation::getHostName(), this,
                                              this->num_cores_per_node,
                                              this->getPropertyValueAsBoolean(BatchServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES),
                                              this->workload_trace)
        );
        try {
            this->workload_trace_replayer->simulation = this->simulation;
            this->workload_trace_replayer->start(this->workload_trace_replayer, true, false); // Daemonized, no auto-restart
        } catch (std::runtime_error &e) {
            throw;
        }
        return;
    }

/**
 * @brief Returns queue wait time estimates for the FCFS (non-batsched) algorithm
 * @param set_of_jobs: a set of job specifications (<id, num hosts, time>)
 *
 * @return a map of queue wait time predictions
 */
    std::map<std::string, double>
    BatchService::getStartTimeEstimatesForFCFS(
            std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) {

        // Assumes time origin is zero for simplicity!

        // Set the available time of each code to zero (i.e., now)
        // (invariant: for each host, core availabilities are sorted by
        //             non-decreasing available time)
        std::map<std::string, std::vector<double>> core_available_times;
        for (auto h : this->nodes_to_cores_map) {
            std::string hostname = h.first;
            unsigned long num_cores = h.second;
            std::vector<double> zeros;
            for (unsigned int i = 0; i < num_cores; i++) {
                zeros.push_back(0);
            }
            core_available_times.insert(std::make_pair(hostname, zeros));
        }


        // Update core availabilities for jobs that are currently running
        for (auto job : this->running_jobs) {
            double time_to_finish = std::max<double>(0, job->getBeginTimeStamp() +
                                                        job->getAllocatedTime() -
                                                        this->simulation->getCurrentSimulatedDate());
            for (auto resource : job->getResourcesAllocated()) {
                std::string hostname = resource.first;
                unsigned long num_cores = std::get<0>(resource.second);
                double ram = std::get<1>(resource.second);
                // Update available_times
                double new_available_time = *(core_available_times[hostname].begin() + num_cores - 1) + time_to_finish;
                for (unsigned int i = 0; i < num_cores; i++) {
                    *(core_available_times[hostname].begin() + i) = new_available_time;
                }
                // Sort them!
                std::sort(core_available_times[hostname].begin(), core_available_times[hostname].end());
            }
        }

#if 0
        std::cerr << "TIMELINES AFTER ACCOUNTING FOR RUNNING JOBS: \n";
      for (auto h : core_available_times) {
        std::cerr << "  " << h.first << ":\n";
        for (auto t : h.second) {
          std::cerr << "     core : " << t << "\n";
        }
      }
      std::cerr << "----------------------------------------\n";
#endif

        // Go through the pending jobs and update core availabilities
        for (auto job : this->pending_jobs) {
            double duration = job->getAllocatedTime();
            unsigned long num_hosts = job->getNumNodes();
            unsigned long num_cores_per_host = job->getAllocatedCoresPerNode();

#if 0
            std::cerr << "ACCOUNTING FOR RUNNING JOB WITH " << "duration:" <<
                  duration << " num_hosts=" << num_hosts << " num_cores_per_host=" << num_cores_per_host << "\n";
#endif

            // Compute the  earliest start times on all hosts
            std::vector<std::pair<std::string, double>> earliest_start_times;
            for (auto h : core_available_times) {
                double earliest_start_time = *(h.second.begin() + num_cores_per_host - 1);
                earliest_start_times.emplace_back(std::make_pair(h.first, earliest_start_time));
            }

            // Sort the hosts by earliest start times
            std::sort(earliest_start_times.begin(), earliest_start_times.end(),
                      [](std::pair<std::string, double> const &a, std::pair<std::string, double> const &b) {
                          return std::get<1>(a) < std::get<1>(b);
                      });

            // Compute the actual earliest start time
            double earliest_job_start_time = (*(earliest_start_times.begin() + num_hosts - 1)).second;

            // Update the core available times on each host used for the job
            for (unsigned int i = 0; i < num_hosts; i++) {
                std::string hostname = (*(earliest_start_times.begin() + i)).first;
                for (unsigned int j = 0; j < num_cores_per_host; j++) {
                    *(core_available_times[hostname].begin() + j) = earliest_job_start_time + duration;
                }
                std::sort(core_available_times[hostname].begin(), core_available_times[hostname].end());
            }

            // Go through all hosts and make sure that no core is available before earliest_job_start_time
            // since this is a simple FCFS algorithm with no "jumping ahead" of any kind
            for (auto h : this->nodes_to_cores_map) {
                std::string hostname = h.first;
                unsigned long num_cores = h.second;
                for (unsigned int i = 0; i < num_cores; i++) {
                    if (*(core_available_times[hostname].begin() + i) < earliest_job_start_time) {
                        *(core_available_times[hostname].begin() + i) = earliest_job_start_time;
                    }
                }
                std::sort(core_available_times[hostname].begin(), core_available_times[hostname].end());
            }

#if 0
            std::cerr << "AFTER ACCOUNTING FOR THIS JOB: \n";
        for (auto h : core_available_times) {
          std::cerr << "  " << h.first << ":\n";
          for (auto t : h.second) {
            std::cerr << "     core : " << t << "\n";
          }
        }
        std::cerr << "----------------------------------------\n";
#endif

        }

        // We now have the predicted available times for each cores given
        // everything that's running and pending. We can compute predictions.
        std::map<std::string, double> predictions;

        for (auto job : set_of_jobs) {
            std::string id = std::get<0>(job);
            unsigned int num_hosts = std::get<1>(job);
            unsigned int num_cores_per_host = std::get<2>(job);
            double duration = std::get<3>(job);

            double earliest_job_start_time;

            if ((num_hosts > core_available_times.size()) ||
                (num_cores_per_host > this->num_cores_per_node)) {

                earliest_job_start_time = -1.0;

            } else {

#if 0
                std::cerr << "COMPUTING PREDICTIONS for JOB: num_hosts=" << num_hosts <<
                    ", num_cores_per_hosts=" << num_cores_per_host << ", duration=" << duration << "\n";
#endif

                // Compute the earliest start times on all hosts
                std::vector<std::pair<std::string, double>> earliest_start_times;
                for (auto h : core_available_times) {
                    double earliest_start_time = *(h.second.begin() + num_cores_per_host - 1);
                    earliest_start_times.emplace_back(std::make_pair(h.first, earliest_start_time));
                }


                // Sort the hosts by putative start times
                std::sort(earliest_start_times.begin(), earliest_start_times.end(),
                          [](std::pair<std::string, double> const &a, std::pair<std::string, double> const &b) {
                              return std::get<1>(a) < std::get<1>(b);
                          });

                earliest_job_start_time = (*(earliest_start_times.begin() + num_hosts - 1)).second;
            }


            // Note that below we translate predictions back to actual start dates given the current time
            if (earliest_job_start_time > 0) {
                earliest_job_start_time = this->simulation->getCurrentSimulatedDate() + earliest_job_start_time;
            }
            predictions.insert(std::make_pair(id, earliest_job_start_time));

        }

        return predictions;

    }

/**
 * @brief Process a "terminate standard job message"
 *
 * @param job: the job to terminate
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 */
    void BatchService::processStandardJobTerminationRequest(StandardJob *job,
                                                            std::string answer_mailbox) {

        BatchJob* batch_job = nullptr;
        std::string job_id = "";
        // Is it running?
        bool is_running = false;
        for (auto const & j : this->running_jobs) {
            WorkflowJob *workflow_job = j->getWorkflowJob();
            if ((workflow_job->getType() == WorkflowJob::STANDARD) and ((StandardJob *)workflow_job == job)) {
                batch_job = j;
                job_id = std::to_string(j->getJobID());
                is_running = true;
            }
        }

        bool is_pending = false;
        std::deque<BatchJob *>::iterator batch_pending_it = this->pending_jobs.end();
        if (batch_job == nullptr) {
            // Is it pending?
            for (auto it1 = this->pending_jobs.begin(); it1 != this->pending_jobs.end(); it1++) {
                WorkflowJob *workflow_job = (*it1)->getWorkflowJob();
                if ((workflow_job->getType() == WorkflowJob::STANDARD) and ((StandardJob *)workflow_job == job)) {
                    batch_pending_it = it1;
                    job_id = std::to_string((*it1)->getJobID());
                    is_pending = true;
                }
            }
        }

        bool is_waiting = false;
        if (batch_job == nullptr && batch_pending_it == this->pending_jobs.end()) {
            // Is it waiting?
            for (auto const & j : this->waiting_jobs) {
                WorkflowJob *workflow_job = j->getWorkflowJob();
                if ((workflow_job->getType() == WorkflowJob::STANDARD) and ((StandardJob *)workflow_job == job)) {
                    batch_job = j;
                    job_id = std::to_string(j->getJobID());
                    is_waiting = true;
                }
            }
        }

        if (!is_pending && !is_running && !is_waiting) {
            // Send a failure reply
            ComputeServiceTerminateStandardJobAnswerMessage *answer_message =
                    new ComputeServiceTerminateStandardJobAnswerMessage(
                            job, this, false, std::shared_ptr<FailureCause>(
                                    new JobCannotBeTerminated(
                                            job)),
                            this->getMessagePayloadValueAsDouble(
                                    BatchServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
            try {
                S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
        }


#ifdef ENABLE_BATSCHED
        //      throw std::runtime_error("BatchService::processStandardJobTerminationRequest(): Not implemented for BATSCHED yet");
        notifyJobEventsToBatSched(job_id, "TIMEOUT", "COMPLETED_FAILED", "", "JOB_COMPLETED");
        this->appendJobInfoToCSVOutputFile(batch_job, "FAILED");
#endif
        // Is it running?
        if (is_running) {
            terminateRunningStandardJob(job);
            this->running_jobs.erase(batch_job);
            this->freeJobFromJobsList(batch_job);
        }
        if (is_pending) {
            BatchJob *to_free = *batch_pending_it;
            this->pending_jobs.erase(batch_pending_it);
            this->freeJobFromJobsList(to_free);
        }
        if (is_waiting) {
            this->waiting_jobs.erase(batch_job);
            this->freeJobFromJobsList(batch_job);
        }
        ComputeServiceTerminateStandardJobAnswerMessage *answer_message =
                new ComputeServiceTerminateStandardJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getMessagePayloadValueAsDouble(
                                BatchServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD));
        try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return;
        }

    }


/********************************************************************************************/
/** BATSCHED INTERFACE METHODS BELOW                                                        */
/********************************************************************************************/

#ifdef ENABLE_BATSCHED

/**
 *  @brief: Start a batsched process
 *           - exit code 1: unsupported algorithm
 *           - exit code 2: unsupported queuing option
 *           - exit code 3: execvp error
 */
    void BatchService::startBatsched() {

        // The "mod by 1500" below is totally ad-hoc, but not modding seemed
        // to lead to weird zmq "address already in use" errors...
        this->batsched_port = 28000 + (getpid() % 1500) +
                              S4U_Mailbox::generateUniqueSequenceNumber();
        this->pid = getpid();


        int top_pid = fork();

        if (top_pid == 0) { // Child process that will exec batsched

            std::string algorithm = this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM);
            bool is_supported = this->scheduling_algorithms.find(algorithm) != this->scheduling_algorithms.end();
            if (not is_supported) {
                exit(1);
            }

            std::string queue_ordering = this->getPropertyValueAsString(
                    BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM);
            bool is_queue_ordering_available =
                    this->queue_ordering_options.find(queue_ordering) != this->queue_ordering_options.end();
            if (not is_queue_ordering_available) {
                exit(2);
            }

            std::string rjms_delay = this->getPropertyValueAsString(BatchServiceProperty::BATCH_RJMS_DELAY);
            std::string socket_endpoint = "tcp://*:" + std::to_string(this->batsched_port);

            char **args = NULL;
            unsigned int num_args = 0;

            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("batsched"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("-v"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(algorithm.c_str()); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("-o"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(queue_ordering.c_str()); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("-s"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(socket_endpoint.c_str()); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup("--rjms_delay"); num_args++;
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = strdup(rjms_delay.c_str()); num_args++;

            if (this->getPropertyValueAsBoolean(BatchServiceProperty::BATSCHED_LOGGING_MUTED)) {
                (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] =
                        strdup("--verbosity=silent"); num_args++;
            }
            if (this->getPropertyValueAsBoolean(BatchServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION)) {
                (args = (char **) realloc(args, (num_args + 1) * sizeof(char *)))[num_args] =
                        strdup("--policy=contiguous"); num_args++;
            }
            (args = (char **)realloc(args, (num_args+1)*sizeof(char*)))[num_args] = nullptr;

            if (execvp(args[0], args) == -1) {
                exit(3);
            }


        } else if (top_pid > 0) {
            // parent process
            sleep(1); // Wait one second to let batsched the time to start
            // (this is pretty ugly)
            int exit_code = 0;
            int status = waitpid(top_pid, &exit_code, WNOHANG);
            if (status == 0) {
                exit_code = 0;
            } else {
                exit_code = WIFEXITED(exit_code);
            }
            switch (exit_code) {
                case 0: {
                    // Establish a tether so that if the main process dies, then batsched is brutally killed
                    int tether[2]; // this is a local variable, only defined in this scope
                    if (pipe(tether) != 0) {  // the pipe however is opened during the whole duration of both processes
                        kill(top_pid, SIGKILL); //kill the other child (that has fork-exec'd batsched)
                        throw std::runtime_error("startBatsched(): tether pipe creation failed!");
                    }
                    //now fork a process that sleeps until its parent is dead
                    int nested_pid = fork();

                    if (nested_pid > 0) {
                        //I am the parent, whose child has fork-exec'd batsched
                    } else if (nested_pid == 0) {
                        char foo;
                        close(tether[1]); // closing write end
                        read(tether[0], &foo, 1); // blocking read which returns when the parent dies
                        //check if the child that forked batsched is still running
                        if (getpgid(top_pid)) {
                            kill(top_pid, SIGKILL); //kill the other child that has fork-exec'd batsched
                        }
                        //my parent has died so, I will kill myself instead of exiting and becoming a zombie
                        kill(getpid(), SIGKILL);
                        //exit(is_sent); //if exit myself and become a zombie :D

                    }
                    return;
                }
                case 1:
                    throw std::invalid_argument(
                            "startBatsched(): Scheduling algorithm " +
                            this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM) +
                            " not supported by the batch service");
                case 2:
                    throw std::invalid_argument(
                            "startBatsched(): Queuing option " +
                            this->getPropertyValueAsString(BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM) +
                            "not supported by the batch service");
                case 3:
                    throw std::runtime_error(
                            "startBatsched(): Cannot start the batsched process");
                default:
                    throw std::runtime_error(
                            "startBatsched(): Unknown fatal error");
            }

        } else {
            // fork failed
            throw std::runtime_error(
                    "Error while fork-exec of batsched"
            );
        }
    }

/**
 * @brief: Stop the batsched process
 */
    void BatchService::stopBatsched() {

        // Stop Batsched
        zmq::context_t context(1);
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect("tcp://localhost:" + std::to_string(this->batsched_port));


        nlohmann::json simulation_ends_msg;
        simulation_ends_msg["now"] = S4U_Simulation::getClock();
        simulation_ends_msg["events"][0]["timestamp"] = S4U_Simulation::getClock();
        simulation_ends_msg["events"][0]["type"] = "SIMULATION_ENDS";
        simulation_ends_msg["events"][0]["data"] = {};
        std::string data_to_send = simulation_ends_msg.dump();

        zmq::message_t request(strlen(data_to_send.c_str()));
        memcpy(request.data(), data_to_send.c_str(), strlen(data_to_send.c_str()));
        socket.send(request);

        //  Get the reply.
        zmq::message_t reply;
        socket.recv(&reply);

        // Process the reply
        std::string reply_data;
        reply_data = std::string(static_cast<char *>(reply.data()), reply.size());

        nlohmann::json reply_decisions;
        nlohmann::json decision_events;
        reply_decisions = nlohmann::json::parse(reply_data);
        decision_events = reply_decisions["events"];
        if (decision_events.size() > 0) {
            throw std::runtime_error(
                    "BatchService::stopBatsched(): Upon termination BATSCHED returned a non-empty event set, which is unexpected");
        }

    }

    std::map<std::string, double>
    BatchService::getStartTimeEstimatesFromBatsched(
            std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs) {
        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();


        // IMPORTANT: THIS IGNORES THE NUMBER OF CORES (THIS IS A LIMITATION OF BATSCHED!)

        int idx = 0;
        batch_submission_data["events"] = nlohmann::json::array();
        for (auto job : set_of_jobs) {
            batch_submission_data["events"][idx]["timestamp"] = S4U_Simulation::getClock();
            batch_submission_data["events"][idx]["type"] = "QUERY";
            batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job_id"] = std::get<0>(job);
            batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job"]["id"] = std::get<0>(
                    job);
            batch_submission_data["events"][idx]["data"]["requests"]["estimate_waiting_time"]["job"]["res"] = std::get<1>(
                    job);
            batch_submission_data["events"][idx++]["data"]["requests"]["estimate_waiting_time"]["job"]["walltime"] = std::get<3>(
                    job);
        }

        std::string data = batch_submission_data.dump();

        std::string batchsched_query_mailbox = S4U_Mailbox::generateUniqueMailboxName("batchsched_query_mailbox");

        std::shared_ptr<BatschedNetworkListener> network_listener =
                std::shared_ptr<BatschedNetworkListener>(
                        new BatschedNetworkListener(this->hostname, this, batchsched_query_mailbox,
                                                    std::to_string(this->batsched_port),
                                                    data));
        network_listener->simulation = this->simulation;
        network_listener->start(network_listener, true, false); // Daemonized, no auto-restart
        network_listener = nullptr; // detached mode
//      this->network_listeners.insert(std::move(network_listener));


        std::map<std::string, double> job_estimated_start_times = {};
        for (auto job : set_of_jobs) {
            // Get the answer
            std::unique_ptr<SimulationMessage> message = nullptr;
            try {
                message = S4U_Mailbox::getMessage(batchsched_query_mailbox);
            } catch (std::shared_ptr<NetworkError> &cause) {
                throw WorkflowExecutionException(cause);
            }

            if (auto *msg = dynamic_cast<BatchQueryAnswerMessage *>(message.get())) {
                job_estimated_start_times[std::get<0>(job)] = msg->estimated_start_time;
            } else {
                throw std::runtime_error(
                        "BatchService::getQueueWaitingTimeEstimate(): Received an unexpected [" + message->getName() +
                        "] message.\nThis likely means that the scheduling algorithm that Batsched was configured to use (" +
                        this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM) +
                        ") does not support queue waiting time predictions!");
            }
        }
        return job_estimated_start_times;
    }


/**
* @brief Notify a job even to BATSCHED (BATSCHED ONLY)
* @param job_id the id of the job to be processed
* @param status the status of the job
* @param job_state current state of the job
* @param kill_reason the reason to be killed ("" if not being killed)
* @param event_type the type of event (JOB_COMPLETED/JOB_KILLED..)
*/
    void BatchService::notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                                 std::string kill_reason, std::string event_type) {

//      if (not this->isBatschedReady()) {
//        throw std::runtime_error("BatchService::notifyJobEventsToBatSched(): "
//                                         "BATSCHED is not ready, which should not happen");
//      }

        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();
        batch_submission_data["events"][0]["timestamp"] = S4U_Simulation::getClock();
        batch_submission_data["events"][0]["type"] = event_type;
        batch_submission_data["events"][0]["data"]["job_id"] = job_id;
        batch_submission_data["events"][0]["data"]["status"] = status;
        batch_submission_data["events"][0]["data"]["job_state"] = job_state;
        batch_submission_data["events"][0]["data"]["kill_reason"] = kill_reason;

        std::string data = batch_submission_data.dump();
        std::shared_ptr<BatschedNetworkListener> network_listener =
                std::shared_ptr<BatschedNetworkListener>(
                        new BatschedNetworkListener(this->hostname, this, this->mailbox_name,
                                                    std::to_string(this->batsched_port),
                                                    data));
        network_listener->simulation = this->simulation;
        network_listener->start(network_listener, true, false); // Daemonized, no auto-restart
        network_listener = nullptr; // detached mode
//      this->network_listeners.insert(network_listener);
    }

/**
 * @brief Start a network listener process (for BATSCHED only)
 */
    void BatchService::startBatschedNetworkListener() {

        nlohmann::json compute_resources_map;
        compute_resources_map["now"] = S4U_Simulation::getClock();
        compute_resources_map["events"][0]["timestamp"] = S4U_Simulation::getClock();
        compute_resources_map["events"][0]["type"] = "SIMULATION_BEGINS";
        compute_resources_map["events"][0]["data"]["nb_resources"] = this->nodes_to_cores_map.size();
        compute_resources_map["events"][0]["data"]["allow_time_sharing"] = false;
//    This was the "old" batsched up until commit 39a30d83
//      compute_resources_map["events"][0]["data"]["config"]["redis"]["enabled"] = false;
        compute_resources_map["events"][0]["data"]["config"]["redis-enabled"] = false;
        std::map<std::string, unsigned long>::iterator it;
        int count = 0;
        for (it = this->nodes_to_cores_map.begin(); it != this->nodes_to_cores_map.end(); it++) {
            compute_resources_map["events"][0]["data"]["resources_data"][count]["id"] = std::to_string(count);
            compute_resources_map["events"][0]["data"]["resources_data"][count]["name"] = it->first;
            compute_resources_map["events"][0]["data"]["resources_data"][count]["core"] = it->second;
            compute_resources_map["events"][0]["data"]["resources_data"][count++]["state"] = "idle";
        }
        std::string data = compute_resources_map.dump();


        try {
            std::shared_ptr<BatschedNetworkListener> network_listener =
                    std::shared_ptr<BatschedNetworkListener>(
                            new BatschedNetworkListener(this->hostname, this, this->mailbox_name,
                                                        std::to_string(this->batsched_port),
                                                        data));
            network_listener->simulation = this->simulation;
            network_listener->start(network_listener, true, false); // Daemonized, no auto-restart
            network_listener = nullptr; // detached mode
//        this->network_listeners.insert(std::move(network_listener));
        } catch (std::runtime_error &e) {
            throw;
        }

    }

    void BatchService::sendAllQueuedJobsToBatsched() {
        if (this->pending_jobs.empty()) {
            return;
        }

        // Send ALL Queued jobs to batsched, and move them all to the WAITING queue
        // The WAITING queue is: those jobs that I need to hear from Batsched about

        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();
        batch_submission_data["events"] = nlohmann::json::array();
        size_t i;
        std::deque<BatchJob *>::iterator it;
        for (i = 0, it = this->pending_jobs.begin(); i < this->pending_jobs.size(); i++, it++) {

            BatchJob *batch_job = *it;

            /* Get the nodes and cores per nodes asked for */
            unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();
            unsigned long num_nodes_asked_for = batch_job->getNumNodes();
            unsigned long allocated_time = batch_job->getAllocatedTime();

            batch_submission_data["events"][i]["timestamp"] = batch_job->getArrivalTimeStamp();
            batch_submission_data["events"][i]["type"] = "JOB_SUBMITTED";
            batch_submission_data["events"][i]["data"]["job_id"] = std::to_string(batch_job->getJobID());
            batch_submission_data["events"][i]["data"]["job"]["id"] = std::to_string(batch_job->getJobID());
            batch_submission_data["events"][i]["data"]["job"]["res"] = num_nodes_asked_for;
            batch_submission_data["events"][i]["data"]["job"]["core"] = cores_per_node_asked_for;
            batch_submission_data["events"][i]["data"]["job"]["walltime"] = allocated_time;

            this->pending_jobs.erase(it);
            this->waiting_jobs.insert(batch_job);

        }
        std::string data = batch_submission_data.dump();
        std::shared_ptr<BatschedNetworkListener> network_listener =
                std::shared_ptr<BatschedNetworkListener>(
                        new BatschedNetworkListener(this->hostname, this, this->mailbox_name,
                                                    std::to_string(this->batsched_port),
                                                    data));
        network_listener->simulation = this->simulation;
        network_listener->start(network_listener, true, false); // Daemonized, no auto-restart
        network_listener = nullptr; // detached mode
//      this->network_listeners.insert(std::move(network_listener));
    }

/**
 *
 * @param bat_sched_reply
 */
    void BatchService::processExecuteJobFromBatSched(std::string bat_sched_reply) {
        nlohmann::json execute_events = nlohmann::json::parse(bat_sched_reply);
        WorkflowJob *workflow_job = nullptr;
        BatchJob *batch_job = nullptr;
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
            //throw std::runtime_error("BatchService::processExecuteJobFromBatSched(): Job received from batsched that does not belong to the list of jobs batchservice has");
            WRENCH_WARN("BatchService::processExecuteJobFromBatSched(): Job received from batsched that does not belong to the list of known jobs... ignoring (Batsched seems to send this back even when a job has been actively terminated)");
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
        unsigned long time_in_seconds = batch_job->getAllocatedTime();
        unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();

        std::map<std::string, std::tuple<unsigned long, double>> resources = {};
        std::vector<std::string> hosts_assigned = {};
        std::map<std::string, unsigned long>::iterator it;

        for (auto node:node_resources) {
            this->available_nodes_to_cores[this->host_id_to_names[node]] -= cores_per_node_asked_for;
            resources.insert(std::make_pair(this->host_id_to_names[node],std::make_tuple( cores_per_node_asked_for,
                                                                                          0))); // TODO: Is setting RAM to 0 ok here?
        }

        startJob(resources, workflow_job, batch_job, num_nodes_allocated, time_in_seconds,
                 cores_per_node_asked_for);

    }


//    void BatchService::setBatschedReady(bool v) {
//      this->is_bat_sched_ready = v;
//    }
//
//    bool BatchService::isBatschedReady() {
//      return this->is_bat_sched_ready;
//    }


/**
 * @brief Appends a job to the CSV Output file
 *
 * @param batch_job: the batch job
 * @param status: "COMPLETED", "TERMINATED" (by user), "FAILED" (timeout)
 */
    void BatchService::appendJobInfoToCSVOutputFile(BatchJob *batch_job, std::string status) {
        std::string csv_file_path;
        static unsigned long job_id = 0;

        csv_file_path = this->getPropertyValueAsString(BatchServiceProperty::OUTPUT_CSV_JOB_LOG);
        if (csv_file_path.empty()) {
            return;
        }
        std::ofstream file;
        file.open(csv_file_path, std::ios_base::app);
        if (not file) {
            throw std::runtime_error("BatchService(): Unable to append to CSV output file " +
                                     csv_file_path + " (as specified by the BatchServiceProperty::OUTPUT_CSV_JOB_LOG property)");
        }
        std::string csv_line = "";

        // Allocated Processors
        csv_line += batch_job->csv_allocated_processors + ",";
        // Consumed Energy
        csv_line += "0,";
        // Execution Time
        double execution_time = batch_job->getEndingTimeStamp() - batch_job->getBeginTimeStamp();
        csv_line += std::to_string(execution_time) + ",";
        // Finish time
        double finish_time = batch_job->getEndingTimeStamp();
        csv_line += std::to_string(finish_time) + ",";
        // Job ID
        csv_line += std::to_string(job_id++) + ",";
        // MetaData
        csv_line += std::string("\"") + batch_job->csv_metadata + "\",";
        // Requested number of processors
        csv_line += std::to_string(batch_job->getNumNodes()) + ",";
        // Requested time
        csv_line += std::to_string(batch_job->getAllocatedTime()) + ",";
        // Starting time
        double starting_time = batch_job->getBeginTimeStamp();
        csv_line += std::to_string(starting_time) + ",";
        // Stretch
        double stretch = (batch_job->getEndingTimeStamp() - batch_job->getArrivalTimeStamp()) /
                         (batch_job->getEndingTimeStamp() - batch_job->getBeginTimeStamp());
        csv_line += std::to_string(stretch) + ",";
        // Submission time
        double submission_time = batch_job->getArrivalTimeStamp();
        csv_line += std::to_string(submission_time) + ",";
        // Success
        unsigned char success = 1;
        if (status == "COMPLETED") {
            success = 1;
        } else if (status == "FAILED") {
            success = 0;
        } else if (status == "TERMINATED") {
            success = 2;
        }
        csv_line += std::to_string(success) + ",";
        // Turnaround time
        double turnaround_time = (batch_job->getEndingTimeStamp() - batch_job->getArrivalTimeStamp());
        csv_line += std::to_string(turnaround_time) + ",";
        // Waiting time
        double waiting_time = (batch_job->getBeginTimeStamp() - batch_job->getArrivalTimeStamp());
        csv_line += std::to_string(waiting_time) + ",";
        // Workload name
        csv_line += "wrench";



        file << csv_line << std::endl;
        file.close();

    }

#endif  // ENABLE_BATSCHED

};
