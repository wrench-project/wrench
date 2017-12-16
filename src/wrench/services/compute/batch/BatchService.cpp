/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/compute/batch/BatchService.h"
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include <wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h>
#include <wrench/util/PointerUtil.h>
#include <wrench/services/compute/batch/BatchNetworkListener.h>
#include <zmq.hpp>
#include "wrench/services/helpers/Alarm.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "services/compute/ComputeServiceMessage.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/services/compute/batch/BatchServiceMessage.h"
#include "wrench/services/compute/batch/BatchRequestReplyProcess.h"
#include <sys/types.h>
#include <signal.h>
#include <json.hpp>
#include <boost/algorithm/string.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service, "Log category for Batch Service");

namespace wrench {
    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param nodes_in_network: the hosts running in the network
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param plist: a property list ({} means "use all defaults")
     */
    BatchService::BatchService(std::string hostname,
                               std::vector<std::string> nodes_in_network,
                               StorageService *default_storage_service,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               std::map<std::string, std::string> plist) :
            BatchService(hostname, nodes_in_network, default_storage_service, supports_standard_jobs,
                         supports_pilot_jobs,0,plist, "") {

    }


    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param nodes_in_network: the hosts running in the network
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param plist: a property list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     */
    BatchService::BatchService(std::string hostname,
                               std::vector<std::string> nodes_in_network,
                               StorageService *default_storage_service,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               unsigned long reduced_cores,
                               std::map<std::string, std::string> plist, std::string suffix) :
            ComputeService("batch" + suffix,
                           "batch" + suffix,
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           default_storage_service) {
        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(plist));

        //create a map for host to cores
        int i = 0;
        std::vector<std::string>::iterator it;
        for (it = nodes_in_network.begin(); it != nodes_in_network.end(); it++) {
            if (reduced_cores > 0 && not this->supports_pilot_jobs) {
                this->nodes_to_cores_map.insert({*it, reduced_cores});
                this->available_nodes_to_cores.insert({*it, reduced_cores});
            } else {
                this->nodes_to_cores_map.insert({*it, S4U_Simulation::getNumCores(*it)});
                this->available_nodes_to_cores.insert({*it, S4U_Simulation::getNumCores(*it)});
            }
            this->host_id_to_names[i++] = *it;
        }

        this->total_num_of_nodes = nodes_in_network.size();
        this->hostname = hostname;

        this->generateUniqueJobId();

        this->run_batsched();

        // Start the daemon on the same host
        try {
            this->start(hostname);
        } catch (std::invalid_argument e) {
            throw e;
        }
    }


    /**
     * @brief Synchronously submit a standard job to the batch service
     *
     * @param job: a standard job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     *
     */
    void BatchService::submitStandardJob(StandardJob *job, std::map<std::string, std::string> &batch_job_args) {

        if (this->state == Service::DOWN) {
            throw WorkflowExecutionException(new ServiceIsDown(this));
        }

        //check if there are enough hosts
        unsigned long nodes_asked_for = 0;
        std::map<std::string, std::string>::iterator it;
        it = batch_job_args.find("-N");
        if (it != batch_job_args.end()) {
            if (sscanf((*it).second.c_str(), "%lu", &nodes_asked_for) != 1) {
                throw std::invalid_argument(
                        "BatchService::submitStandardJob(): Invalid -N value '" + (*it).second + "'");
            }
            if (nodes_asked_for > this->total_num_of_nodes) {
                throw std::runtime_error(
                        "BatchService::submitStandardJob(): There are not enough hosts");
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Batch Service requires -N(number of nodes) to be specified "
            );
        }

        //check if there are enough cores per node
        unsigned long nodes_with_c_cores = 0;
        it = batch_job_args.find("-c");
        if (it != batch_job_args.end()) {
            std::map<std::string, unsigned long>::iterator nodes_to_core_iterator;
            for (nodes_to_core_iterator = this->nodes_to_cores_map.begin();
                 nodes_to_core_iterator != this->nodes_to_cores_map.end(); nodes_to_core_iterator++) {
                unsigned long num;
                if (sscanf((*it).second.c_str(), "%lu", &num) != 1) {
                    throw std::invalid_argument(
                            "BatchService::submitStandardJob(): Invalid -c value '" + (*it).second +
                            "'");
                }
                if (num <= (*nodes_to_core_iterator).second) {
                    nodes_with_c_cores++;
                }
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Batch Service requires -c(cores per node) to be specified "
            );
        }
        if (nodes_asked_for > nodes_with_c_cores) {
            throw std::runtime_error(
                    "BatchService::submitStandardJob(): There are not enough hosts with those amount of cores per host");
        }
        unsigned long cores_asked_for;
        if (sscanf((*it).second.c_str(), "%lu", &cores_asked_for) != 1) {
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Invalid -c value '" + (*it).second + "'");
        }


        //get job time
        unsigned long time_asked_for = 0;
        it = batch_job_args.find("-t");
        if (it != batch_job_args.end()) {
            if (sscanf((*it).second.c_str(), "%lu", &time_asked_for) != 1) {
                throw std::invalid_argument(
                        "BatchService::submitStandardJob(): Invalid -t value '" + (*it).second + "'");
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Batch Service requires -t to be specified "
            );
        }

        //Create a Batch Job
        unsigned long jobid = this->generateUniqueJobId();
        BatchJob* batch_job = new BatchJob(job, jobid, time_asked_for,
                                                                                     nodes_asked_for, cores_asked_for, -1, S4U_Simulation::getClock());

        //  send a "run a batch job" message to the daemon's mailbox
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_standard_job_mailbox");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new BatchServiceJobRequestMessage(answer_mailbox, batch_job,
                                                                      this->getPropertyValueAsDouble(
                                                                              BatchServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        if (ComputeServiceSubmitStandardJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }

        } else {
            throw std::runtime_error(
                    "BatchService::submitStandardJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }

        return;

    }


    /**
     * @brief Synchronously submit a pilot job to the compute service
     *
     * @param job: a pilot job
     * @param batch_job_args: arguments to the batch job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void BatchService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &batch_job_args) {

        if (this->state == Service::DOWN) {
            throw WorkflowExecutionException(new ServiceIsDown(this));
        }

        //check if there are enough hosts
        unsigned long nodes_asked_for = 0;
        std::map<std::string, std::string>::iterator it;
        it = batch_job_args.find("-N");
        if (it != batch_job_args.end()) {
            if (sscanf((*it).second.c_str(), "%lu", &nodes_asked_for) != 1) {
                throw std::runtime_error(
                        "BatchService::submitPilotJob(): Invalid -N value '" + (*it).second + "'");
            }
            if (nodes_asked_for > this->total_num_of_nodes) {
                throw std::runtime_error(
                        "BatchService::submitPilotJob(): There are not enough hosts");
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::submitPilotJob(): Batch Service requires -N(number of nodes) to be specified "
            );
        }

        //check if there are enough cores per node
        unsigned long nodes_with_c_cores = 0;
        unsigned long cores_asked_for = 0;
        it = batch_job_args.find("-c");
        if (it != batch_job_args.end()) {
            std::map<std::string, unsigned long>::iterator nodes_to_core_iterator;
            if (sscanf((*it).second.c_str(), "%lu", &cores_asked_for) != 1) {
                throw std::invalid_argument(
                        "BatchService::submitPilotJob(): Invalid -c value '" + (*it).second + "'");
            }
            for (nodes_to_core_iterator = this->nodes_to_cores_map.begin();
                 nodes_to_core_iterator != this->nodes_to_cores_map.end(); nodes_to_core_iterator++) {
                if (cores_asked_for <= (*nodes_to_core_iterator).second) {
                    nodes_with_c_cores++;
                }
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::submitPilotJob(): Batch Service requires -c(cores per node) to be specified "
            );
        }
        if (nodes_asked_for > nodes_with_c_cores) {
            throw std::runtime_error(
                    "BatchService::submitPilotJob(): There are not enough hosts with those amount of cores per host");
        }

        //get job time
        unsigned long time_asked_for = 0;
        it = batch_job_args.find("-t");
        if (it != batch_job_args.end()) {
            if (sscanf((*it).second.c_str(), "%lu", &time_asked_for) != 1) {
                throw std::invalid_argument(
                        "BatchService::submitPilotJob(): Invalid -t argiment '" + (*it).second + "'");
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::submitPilotJob(): Batch Service requires -t to be specified "
            );
        }

        //Create a Batch Job
        unsigned long jobid = this->generateUniqueJobId();
        BatchJob* batch_job = new BatchJob(job, jobid, time_asked_for,
                                                                                     nodes_asked_for, cores_asked_for, -1, S4U_Simulation::getClock());

        //  send a "run a batch job" message to the daemon's mailbox
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_pilot_job_mailbox");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new BatchServiceJobRequestMessage(answer_mailbox, batch_job,
                                                                      this->getPropertyValueAsDouble(
                                                                              BatchServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        if (ComputeServiceSubmitPilotJobAnswerMessage *msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }

        } else {
            throw std::runtime_error(
                    "BatchService::submitPilotJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
        return;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BatchService::main() {

        TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

        WRENCH_INFO("Batch Service starting on host %s!", S4U_Simulation::getHostName().c_str());
        nlohmann::json compute_resources_map;
        compute_resources_map["now"] = S4U_Simulation::getClock();
        compute_resources_map["events"][0]["timestamp"] = S4U_Simulation::getClock();
        compute_resources_map["events"][0]["type"] = "SIMULATION_BEGINS";
        compute_resources_map["events"][0]["data"]["nb_resources"] = this->nodes_to_cores_map.size();
        compute_resources_map["events"][0]["data"]["allow_time_sharing"] = false;
        compute_resources_map["events"][0]["data"]["config"]["redis"]["enabled"] = false;
        std::map<std::string, unsigned long>::iterator it;
        int count = 0;
        for (it = this->nodes_to_cores_map.begin(); it != this->nodes_to_cores_map.end(); it++) {
            compute_resources_map["events"][0]["data"]["resources_data"][count]["id"] = std::to_string(count);
            compute_resources_map["events"][0]["data"]["resources_data"][count]["name"] = it->first;
            compute_resources_map["events"][0]["data"]["resources_data"][count]["core"] = it->second;
            compute_resources_map["events"][0]["data"]["resources_data"][count++]["state"] = "idle";
        }
        std::string data = compute_resources_map.dump();


        std::unique_ptr<BatchNetworkListener> network_listener =
                std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name,
                                         "14000", "28000",
                                         BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                         data));

        network_listeners.push_back(std::move(network_listener));

        /** Main loop **/
        bool life = true;
        while (life) {
            life = processNextMessage();
            if (this->running_jobs.size() > 0) {
                std::set<std::unique_ptr<BatchJob>>::iterator it;
                for (it = this->running_jobs.begin(); it != this->running_jobs.end();) {
                    if ((*it)->getEndingTimeStamp() <= S4U_Simulation::getClock()) {
                        if ((*it)->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
                            this->processStandardJobTimeout(
                                    (StandardJob *) (*it)->getWorkflowJob());
                            this->updateResources((*it)->getResourcesAllocated());
                            it = this->running_jobs.erase(it);
                        } else if ((*it)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                        } else {
                            throw std::runtime_error(
                                    "BatchService::main(): Received a JOB type other than Standard and Pilot jobs"
                            );
                        }
                    } else {
                        ++it;
                    }

                }
            }
            if (life && is_bat_sched_ready) {
                while (this->scheduleAllQueuedJobs());
            }
        }

        WRENCH_INFO("Batch Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    void BatchService::sendPilotJobCallBackMessage(PilotJob *job) {
        try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServicePilotJobExpiredMessage(job, this,
                                                                              this->getPropertyValueAsDouble(
                                                                                      BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }
    }

    void BatchService::sendStandardJobCallBackMessage(StandardJob *job) {
        WRENCH_INFO("A standard job executor has failed because of timeout %s", job->getName().c_str());
        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServiceStandardJobFailedMessage(job, this,
                                                                               std::shared_ptr<FailureCause>(
                                                                                       new ServiceIsDown(this)),
                                                                               this->getPropertyValueAsDouble(
                                                                                       BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }
    }

    void BatchService::updateResources(std::set<std::pair<std::string, unsigned long>> resources) {
        if (resources.empty()) {
            return;
        }
        std::set<std::pair<std::string, unsigned long>>::iterator resource_it;
        for (resource_it = resources.begin(); resource_it != resources.end(); resource_it++) {
            this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
        }
    }

    void BatchService::updateResources(StandardJob *job) {
        // Remove the job from the running job list
        bool job_on_the_list = false;

        std::set<std::unique_ptr<BatchJob>>::iterator it;
        for (it = this->running_jobs.begin(); it != this->running_jobs.end(); it++) {
            if ((*it)->getWorkflowJob() == job) {
                job_on_the_list = true;
                // Update the cores count in the available resources
                std::set<std::pair<std::string, unsigned long>> resources = (*it)->getResourcesAllocated();
                std::set<std::pair<std::string, unsigned long>>::iterator resource_it;
                for (resource_it = resources.begin(); resource_it != resources.end(); resource_it++) {
                    this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
                }
                this->running_jobs.erase(it);
                break;
            }
        }

        if (!job_on_the_list) {
            throw std::runtime_error(
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
        }

        return;
    }

    std::string BatchService::foundRunningJobOnTheList(WorkflowJob* job) {
        std::set<std::unique_ptr<BatchJob>>::iterator it;
        std::string job_id = "";
        for (it = this->running_jobs.begin(); it != this->running_jobs.end(); it++) {
            if ((*it)->getWorkflowJob() == job) {
                // Update the cores count in the available resources
                std::set<std::pair<std::string, unsigned long>> resources = (*it)->getResourcesAllocated();
                job_id = std::to_string((*it)->getJobID());
                this->updateResources(resources);
                this->running_jobs.erase(it);
                break;
            }
        }
        return job_id;
    }


    void BatchService::processPilotJobTimeout(PilotJob *job) {
        BatchService *cs = (BatchService *) job->getComputeService();
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

    void BatchService::processStandardJobTimeout(StandardJob *job) {
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;

        bool terminated = false;
        for (it = this->running_standard_job_executors.begin(); it != this->running_standard_job_executors.end(); it++) {
            if (((*it).get())->getJob() == job) {
                ((*it).get())->kill();
                PointerUtil::moveUniquePtrFromSetToSet(it, &(this->running_standard_job_executors),
                                                       &(this->finished_standard_job_executors));
                terminated = true;
                break;
            }
        }
        if (not terminated) {
            throw std::runtime_error(
                    "BatchService::processStandardJobTimeout(): Cannot find standard job executor corresponding to job being timedout");
        }
    }

    /**
     * @brief fail a running standard job
     * @param job: the job
     * @param cause: the failure cause
     */
    void BatchService::failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {

        WRENCH_INFO("Failing running job %s", job->getName().c_str());

        terminateRunningStandardJob(job);

        // Send back a job failed message (Not that it can be a partial fail)
        WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServiceStandardJobFailedMessage(job, this, cause,
                                                                               this->getPropertyValueAsDouble(
                                                                                       BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }
    }

    /**
    * @brief terminate a running standard job
    * @param job: the job
    */
    void BatchService::terminateRunningStandardJob(StandardJob *job) {

        StandardJobExecutor *executor = nullptr;
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;
        for (it = this->running_standard_job_executors.begin();
             it != this->running_standard_job_executors.end(); it++) {
            if (((*it).get())->getJob() == job) {
                executor = (it->get());
            }
        }
        if (executor == nullptr) {
            throw std::runtime_error(
                    "MultihostMulticoreComputeService::terminateRunningStandardJob(): Cannot find standard job executor corresponding to job being terminated");
        }

        // Terminate the executor
        WRENCH_INFO("Terminating a standard job executor");
        executor->kill();

        //TODO: Restore the allocated resources

        return;
    }

    std::set<std::pair<std::string, unsigned long>> BatchService::scheduleOnHosts(std::string host_selection_algorithm,
                                                                                  unsigned long num_nodes,
                                                                                  unsigned long cores_per_node) {
        std::set<std::pair<std::string, unsigned long>> resources = {};
        std::vector<std::string> hosts_assigned = {};
        if (host_selection_algorithm == "FIRSTFIT") {
            std::map<std::string, unsigned long>::iterator it;
            unsigned long host_count = 0;
            for (it = this->available_nodes_to_cores.begin();
                 it != this->available_nodes_to_cores.end(); it++) {
                if ((*it).second >= cores_per_node) {
                    //Remove that many cores from the available_nodes_to_core
                    (*it).second -= cores_per_node;
                    hosts_assigned.push_back((*it).first);
                    resources.insert(std::make_pair((*it).first, cores_per_node));
                    if (++host_count >= num_nodes) {
                        break;
                    }
                }
            }
            if (resources.size() < num_nodes) {
                resources = {};
                std::vector<std::string>::iterator it;
                for (it = hosts_assigned.begin(); it != hosts_assigned.end(); it++) {
                    available_nodes_to_cores[*it] += cores_per_node;
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
                    unsigned long tentative_target_num_cores = MIN(num_available_cores, cores_per_node);
                    unsigned long tentative_target_slack =
                            num_available_cores - tentative_target_num_cores;

                    if ((target_host == "") ||
                        (tentative_target_num_cores > target_num_cores) ||
                        ((tentative_target_num_cores == target_num_cores) &&
                         (target_slack > tentative_target_slack))) {
                        target_host = hostname;
                        target_num_cores = tentative_target_num_cores;
                        target_slack = tentative_target_slack;
                    }
                }
                if (target_host == "") {
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
                resources.insert(std::make_pair(target_host, cores_per_node));
            }
        } else {
            throw std::invalid_argument(
                    "BatchService::scheduleOnHosts(): We don't support " + host_selection_algorithm +
                    " as host selection algorithm"
            );
        }

        return resources;
    }

    std::unique_ptr<BatchJob> BatchService::scheduleJob(std::string job_selection_algorithm) {
        if (job_selection_algorithm == "FCFS") {
            std::unique_ptr<BatchJob> batch_job_ptr = std::move(this->pending_jobs.front());
            this->pending_jobs.pop_front();
            return batch_job_ptr;
        }
        return nullptr;
    }

    bool BatchService::scheduleAllQueuedJobs(){
        if (this->pending_jobs.size() == 0) {
            return false;
        }


        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();
        batch_submission_data["events"] = nlohmann::json::array();
        int i = 0;
        std::deque<std::unique_ptr<BatchJob>>::iterator it;
        for (it=this->pending_jobs.begin();i<this->pending_jobs.size();it++) {

            BatchJob *batch_job = it->get();

            /* Get the nodes and cores per nodes asked for */
            unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();
            unsigned long num_nodes_asked_for = batch_job->getNumNodes();
            unsigned long time_in_minutes = batch_job->getAllocatedTime();

            batch_submission_data["events"][i]["timestamp"] = batch_job->getAppearedTimeStamp();
            batch_submission_data["events"][i]["type"] = "JOB_SUBMITTED";
            batch_submission_data["events"][i]["data"]["job_id"] = std::to_string(batch_job->getJobID());
            batch_submission_data["events"][i]["data"]["job"]["id"] = std::to_string(batch_job->getJobID());
            batch_submission_data["events"][i]["data"]["job"]["res"] = num_nodes_asked_for;
            batch_submission_data["events"][i]["data"]["job"]["core"] = cores_per_node_asked_for;
            batch_submission_data["events"][i++]["data"]["job"]["walltime"] = time_in_minutes * 60;
            PointerUtil::moveUniquePtrFromDequeToSet(it, &(this->pending_jobs),
                                                     &(this->waiting_jobs));

        }
        std::string data = batch_submission_data.dump();
        std::unique_ptr<BatchNetworkListener> network_listener =
                std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name,
                                         "14000", "28000",
                                         BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                         data));
        network_listeners.push_back(std::move(network_listener));
        this->is_bat_sched_ready = false;
        return false;
    }

    /**
    * @brief Declare all current jobs as failed (likely because the daemon is being terminated
    * or has timed out (because it's in fact a pilot job))
    */
    void BatchService::failCurrentStandardJobs(std::shared_ptr<FailureCause> cause) {


        std::set<std::unique_ptr<BatchJob>>::iterator it;
        for (it = this->running_jobs.begin(); it != this->running_jobs.end(); it++) {
            WorkflowJob *workflow_job = it->get()->getWorkflowJob();
            if (workflow_job->getType() == WorkflowJob::STANDARD) {
                StandardJob *job = (StandardJob *) workflow_job;
                this->failRunningStandardJob(job, cause);
            }
        }

        std::deque<std::unique_ptr<BatchJob>>::iterator it1;
        for (it1 = this->pending_jobs.begin(); it1 != this->pending_jobs.end(); it1++) {
            WorkflowJob *workflow_job = it1->get()->getWorkflowJob();
            if (workflow_job->getType() == WorkflowJob::STANDARD) {
                StandardJob *job = (StandardJob *) workflow_job;
                this->failPendingStandardJob(job, cause);
            }
            this->pending_jobs.erase(it1);
        }

        std::set<std::unique_ptr<BatchJob>>::iterator it2;
        for (it2 = this->waiting_jobs.begin(); it2 != this->waiting_jobs.end(); it2++) {
            WorkflowJob *workflow_job = it2->get()->getWorkflowJob();
            if (workflow_job->getType() == WorkflowJob::STANDARD) {
                StandardJob *job = (StandardJob *) workflow_job;
                this->failPendingStandardJob(job, cause);
            }
            this->waiting_jobs.erase(it2);
        }
    }


    /**
     * @brief Notify upper level job submitters
     */
    void BatchService::notifyJobSubmitters(PilotJob *job) {

        WRENCH_INFO("Letting the level above know that the pilot job has ended on mailbox %s",
                    job->getCallbackMailbox().c_str());
        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServicePilotJobExpiredMessage(job, this,
                                                                             this->getPropertyValueAsDouble(
                                                                                     BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
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

        if (this->state == Service::DOWN) {
            throw WorkflowExecutionException(new ServiceIsDown(this));
        }

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("terminate_pilot_job");

        // Send a "terminate a pilot job" message to the daemon's mailbox
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new ComputeServiceTerminatePilotJobRequestMessage(answer_mailbox, job,
                                                                                      this->getPropertyValueAsDouble(
                                                                                              BatchServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> cause) {
            throw WorkflowExecutionException(cause);
        }

        if (ComputeServiceTerminatePilotJobAnswerMessage *msg = dynamic_cast<ComputeServiceTerminatePilotJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }

        } else {
            throw std::runtime_error(
                    "MultihostMulticoreComputeService::terminatePilotJob(): Received an unexpected [" +
                    message->getName() +
                    "] message!");
        }
    }


    /**
     * @brief Terminate the daemon, dealing with pending/running jobs
     */
    void BatchService::terminate() {
        this->setStateToDown();

        WRENCH_INFO("Failing current standard jobs");
        this->failCurrentStandardJobs(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));

        //remove standard job alarms
        std::vector<std::unique_ptr<Alarm>>::iterator it;
        for(it=this->standard_job_alarms.begin();it!=this->standard_job_alarms.end();it++){
            if((*it)->isUp()){
                it->reset();
            }
        }

        //remove standard job alarms
        for(it=this->pilot_job_alarms.begin();it!=this->pilot_job_alarms.end();it++){
            if((*it)->isUp()){
                it->reset();
            }
        }

        nlohmann::json simulation_ends_msg;
        simulation_ends_msg["now"] = S4U_Simulation::getClock();
        simulation_ends_msg["events"][0]["timestamp"] = S4U_Simulation::getClock();
        simulation_ends_msg["events"][0]["type"] = "SIMULATION_ENDS";
        simulation_ends_msg["events"][0]["data"] = {};
        std::string data = simulation_ends_msg.dump();

        std::unique_ptr<BatchNetworkListener> network_listener =
                std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name,
                                         "14000", "28000",
                                         BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                         data));

        network_listeners.push_back(std::move(network_listener));


        if (this->supports_pilot_jobs) {
            std::set<std::unique_ptr<BatchJob>>::iterator it;
            for (it = this->running_jobs.begin(); it != this->running_jobs.end(); it++) {
                if ((*it)->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                    PilotJob *p_job = (PilotJob *) ((*it)->getWorkflowJob());
                    BatchService *cs = (BatchService *) p_job->getComputeService();
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
            }
        }
    }


    bool BatchService::processNextMessage() {

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> cause) {
            return true;
        } catch (std::shared_ptr<NetworkTimeout> cause) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
            return false;
        }

        WRENCH_INFO("Got a [%s] message", message->getName().c_str());


        if (ServiceStopDaemonMessage *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->terminate();
            // This is Synchronous;
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                                BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

            } catch (std::shared_ptr<NetworkError> cause) {
                return false;
            }
            return false;

        } else if (BatchSchedReadyMessage *msg = dynamic_cast<BatchSchedReadyMessage *>(message.get())) {
            is_bat_sched_ready = true;
            return true;

        } else if (BatchExecuteJobFromBatSchedMessage *msg = dynamic_cast<BatchExecuteJobFromBatSchedMessage *>(message.get())) {
            processExecuteJobFromBatSched(msg->batsched_decision_reply);
            return true;

        } else if (BatchServiceJobRequestMessage *msg = dynamic_cast<BatchServiceJobRequestMessage *>(message.get())) {
            WRENCH_INFO("Asked to run a batch job using batchservice with jobid %ld", msg->job->getJobID());
            if (msg->job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
                if (not this->supports_standard_jobs) {
                    try {
                        S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                                 new ComputeServiceSubmitStandardJobAnswerMessage(
                                                         (StandardJob *) msg->job->getWorkflowJob(),
                                                         this,
                                                         false,
                                                         std::shared_ptr<FailureCause>(
                                                                 new JobTypeNotSupported(
                                                                         msg->job->getWorkflowJob(),
                                                                         this)),
                                                         this->getPropertyValueAsDouble(
                                                                 BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
                    } catch (std::shared_ptr<NetworkError> cause) {
                        return true;
                    }
                    return true;
                }
            } else if (msg->job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                if (not this->supports_pilot_jobs) {
                    try {
                        S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                                 new ComputeServiceSubmitPilotJobAnswerMessage(
                                                         (PilotJob *) msg->job->getWorkflowJob(),
                                                         this,
                                                         false,
                                                         std::shared_ptr<FailureCause>(
                                                                 new JobTypeNotSupported(
                                                                         msg->job->getWorkflowJob(),
                                                                         this)),
                                                         this->getPropertyValueAsDouble(
                                                                 BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
                    } catch (std::shared_ptr<NetworkError> cause) {
                        return true;
                    }
                    return true;
                }
            }

            if (msg->job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {

                try {
                    S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                             new ComputeServiceSubmitStandardJobAnswerMessage(
                                                     (StandardJob *) msg->job->getWorkflowJob(), this,
                                                     true,
                                                     nullptr,
                                                     this->getPropertyValueAsDouble(
                                                             BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
                } catch (std::shared_ptr<NetworkError> cause) {
                    return true;
                }
            } else if (msg->job->getWorkflowJob()->getType() == WorkflowJob::PILOT) {
                try {
                    S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                             new ComputeServiceSubmitPilotJobAnswerMessage(
                                                     (PilotJob *) msg->job->getWorkflowJob(), this,
                                                     true,
                                                     nullptr,
                                                     this->getPropertyValueAsDouble(
                                                             BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
                } catch (std::shared_ptr<NetworkError> cause) {
                    return true;
                }
            }
            this->pending_jobs.push_back(std::unique_ptr<BatchJob>(msg->job));
            return true;

        } else if (StandardJobExecutorDoneMessage *msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
            processStandardJobCompletion(msg->executor, msg->job);
            return true;

        } else if (StandardJobExecutorFailedMessage *msg = dynamic_cast<StandardJobExecutorFailedMessage *>(message.get())) {
            processStandardJobFailure(msg->executor, msg->job, msg->cause);
            return true;

        } else if (ComputeServicePilotJobExpiredMessage *msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
            processPilotJobCompletion(msg->job);
            return true;
        } else if (auto *msg = dynamic_cast<ComputeServiceTerminatePilotJobRequestMessage *>(message.get())) {
            processPilotJobTerminationRequest(msg->job, msg->answer_mailbox);
            return true;

        } else if (AlarmJobTimeOutMessage *msg = dynamic_cast<AlarmJobTimeOutMessage *>(message.get())) {
            if (msg->job->getType() == WorkflowJob::STANDARD) {
                this->processStandardJobTimeout((StandardJob *) (msg->job));
                this->updateResources((StandardJob *) msg->job);
                this->sendStandardJobCallBackMessage((StandardJob *) msg->job);
                return true;
            } else if (msg->job->getType() == WorkflowJob::PILOT) {
                PilotJob *job = (PilotJob *) msg->job;
                ComputeService *cs = job->getComputeService();
                try {
                    cs->stop();
                } catch (wrench::WorkflowExecutionException &e) {
                    throw std::runtime_error(
                            "BatchService::processNextMessage(): Not able to terminate the pilot job"
                    );
                }
                this->processPilotJobCompletion(job);
                return true;
            } else {
                throw std::runtime_error(
                        "BatchService::processNextMessage(): Alarm about unknown job type"
                );
            }
        } else {
            throw std::runtime_error(
                    "BatchService::processNextMessage(): Unknown message type: " +
                    std::to_string(message->payload));
            return false;
        }

    }


    /**
     * @brief Process a pilot job completion
     *
     * @param job: the pilot job
     */
    void BatchService::processPilotJobCompletion(PilotJob *job) {

        std::string job_id = "";
        // Remove the job from the running job list
        bool job_on_the_list = false;
        std::set<std::unique_ptr<BatchJob>>::iterator it;
        for (it = this->running_jobs.begin(); it != this->running_jobs.end();) {
            if ((*it)->getWorkflowJob() == job) {
                job_on_the_list = true;
                job_id = std::to_string((*it)->getJobID());
                // Update the cores count in the available resources
                std::set<std::pair<std::string, unsigned long>> resources = (*it)->getResourcesAllocated();
                std::set<std::pair<std::string, unsigned long>>::iterator resource_it;
                for (resource_it = resources.begin(); resource_it != resources.end(); resource_it++) {
                    this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
                }
                this->running_jobs.erase(it);
                break;
            } else {
                ++it;
            }
        }

        if (!job_on_the_list) {
            throw std::runtime_error(
                    "BatchService::processPilotJobCompletion():  Pilot job completion message recevied but no such pilot jobs found in queue"
            );
        }

        //first forward this notification to the batsched
        this->notifyJobEventsToBatSched(job_id,"SUCCESS","COMPLETED_SUCCESSFULLY","");

        // Forward the notification
        try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServicePilotJobExpiredMessage(job, this,
                                                                              this->getPropertyValueAsDouble(
                                                                                      BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
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


        std::deque<std::unique_ptr<BatchJob>>::iterator it;
        std::string job_id = "";
        for (it = this->pending_jobs.begin(); it != this->pending_jobs.end(); it++) {
            if (it->get()->getWorkflowJob() == job) {
                job_id = std::to_string(it->get()->getJobID());
                this->pending_jobs.erase(it);
                ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getPropertyValueAsDouble(
                                BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                try {
                    S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return;
                }
                //first forward this notification to the batsched
                this->notifyJobEventsToBatSched(job_id,"TIMEOUT","NOT_SUBMITTED","");
                return;
            }
        }

        std::set<std::unique_ptr<BatchJob>>::iterator it2;
        for (it2 = this->waiting_jobs.begin(); it2 != this->waiting_jobs.end(); it2++) {
            if (it2->get()->getWorkflowJob() == job) {
                job_id = std::to_string(it2->get()->getJobID());
                this->waiting_jobs.erase(it2);
                ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getPropertyValueAsDouble(
                                BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                try {
                    S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return;
                }
                //first forward this notification to the batsched
                this->notifyJobEventsToBatSched(job_id,"TIMEOUT","NOT_SUBMITTED","");
                return;
            }
        }

        std::set<std::unique_ptr<BatchJob>>::iterator it1;
        for (it1 = this->running_jobs.begin(); it1 != this->running_jobs.end();) {
            if ((*it1)->getWorkflowJob() == job) {
                job_id = std::to_string(it1->get()->getJobID());
                this->processPilotJobTimeout((PilotJob *) (*it1)->getWorkflowJob());
                // Update the cores count in the available resources
                std::set<std::pair<std::string, unsigned long>> resources = (*it1)->getResourcesAllocated();
                std::set<std::pair<std::string, unsigned long>>::iterator resource_it;
                for (resource_it = resources.begin(); resource_it != resources.end(); resource_it++) {
                    this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
                }
                ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getPropertyValueAsDouble(
                                BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
                try {
                    S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
                } catch (std::shared_ptr<NetworkError> &cause) {
                    return;
                }
                this->running_jobs.erase(it1);
                //first forward this notification to the batsched
                this->notifyJobEventsToBatSched(job_id,"TIMEOUT","COMPLETED_FAILED","");
                return;
            } else {
                ++it1;
            }
        }


        // If we got here, we're in trouble
        WRENCH_INFO("Trying to terminate a pilot job that's neither pending nor running!");
        ComputeServiceTerminatePilotJobAnswerMessage *answer_message = new ComputeServiceTerminatePilotJobAnswerMessage(
                job, this, false, std::shared_ptr<FailureCause>(new JobCannotBeTerminated(job)),
                this->getPropertyValueAsDouble(
                        BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD));
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
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;
        for(it=this->running_standard_job_executors.begin();it!=this->running_standard_job_executors.end();it++){
            if((*it).get()==executor){
                PointerUtil::moveUniquePtrFromSetToSet(it, &(this->running_standard_job_executors), &(this->finished_standard_job_executors));
                executor_on_the_list = true;
                break;
            }
        }

        if(not executor_on_the_list){
            throw std::runtime_error(
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }

        // Remove the job from the running job list
        std::string job_id = this->foundRunningJobOnTheList((WorkflowJob*)job);

        if (job_id=="") {
            throw std::runtime_error(
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
        }

        WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());

        //first forward this notification to the batsched
        this->notifyJobEventsToBatSched(job_id,"SUCCESS","COMPLETED_SUCCESSFULLY","");

        // Send the callback to the originator
        try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServiceStandardJobDoneMessage(job, this,
                                                                              this->getPropertyValueAsDouble(
                                                                                      BatchServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }

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
        std::set<std::unique_ptr<StandardJobExecutor>>::iterator it;
        for(it=this->running_standard_job_executors.begin();it!=this->running_standard_job_executors.end();it++){
            if((*it).get()==executor){
                PointerUtil::moveUniquePtrFromSetToSet(it, &(this->running_standard_job_executors), &(this->finished_standard_job_executors));
                executor_on_the_list = true;
                break;
            }
        }

        if(not executor_on_the_list){
            throw std::runtime_error(
                    "BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }

        // Remove the job from the running job list
        std::string job_id = this->foundRunningJobOnTheList((WorkflowJob*)job);

        if (job_id=="") {
            throw std::runtime_error(
                    "BatchService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
        }

        WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

        //first forward this notification to the batsched
        this->notifyJobEventsToBatSched(job_id,"TIMEOUT","COMPLETED_FAILED","");

        // Fail the job
        this->failPendingStandardJob(job, cause);

    }


    /**
     * @brief fail a pending standard job
     * @param job: the job
     * @param cause: the failure cause
     */
    void BatchService::failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause) {
        // Send back a job failed message
        WRENCH_INFO("Sending job failure notification to '%s'", job->getCallbackMailbox().c_str());
        // NOTE: This is synchronous so that the process doesn't fall off the end
        try {
            S4U_Mailbox::putMessage(job->popCallbackMailbox(),
                                    new ComputeServiceStandardJobFailedMessage(job, this, cause,
                                                                               this->getPropertyValueAsDouble(
                                                                                       BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }
    }

    unsigned long BatchService::generateUniqueJobId() {
        static unsigned long jobid = 1;
        return jobid++;
    }

    void BatchService::run_batsched() {
        this->pid = fork();
        if (this->pid == 0) {
            // run the batsched here
            std::string algorithm = this->getPropertyValueAsString(BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM);
            bool is_supported = this->scheduling_algorithms.find(algorithm) != this->scheduling_algorithms.end();
            if (not is_supported) {
                throw std::runtime_error(
                        "The algorithm " + algorithm + " is not supported by the batch service"
                );
            }

            const char *args[] = {"batsched", "-v", algorithm.c_str(), NULL};
            // Now execute it
            execvp(args[0], (char **) args);
        } else if (this->pid > 0) {
            // parent process
            return;
        } else {
            // fork failed
            throw std::runtime_error(
                    "Batscheduler process was not able to be forked"
            );
        }
    }


    void BatchService::processExecuteJobFromBatSched(std::string bat_sched_reply) {
        nlohmann::json execute_events = nlohmann::json::parse(bat_sched_reply);
        WorkflowJob* workflow_job = nullptr;
        BatchJob* batch_job = nullptr;
        std::set<std::unique_ptr<BatchJob>>::iterator it1;
        for(it1=this->waiting_jobs.begin();it1!=this->waiting_jobs.end();it1++){
            if(std::to_string(it1->get()->getJobID())==execute_events["job_id"]){
                batch_job = it1->get();
                workflow_job = batch_job->getWorkflowJob();
                PointerUtil::moveUniquePtrFromSetToSet(it1, &(this->waiting_jobs),
                                                       &(this->running_jobs));
                break;
            }
        }
        if(workflow_job== nullptr){
            throw std::runtime_error(
                    "BatchService::processExecuteJobFromBatSched(): Job received from batsched that does not belong to the list of jobs batchservice has"
            );
        }

        /* Get the nodes and cores per nodes asked for */
        std::string nodes_allocated_by_batsched = execute_events["alloc"];
        std::vector<std::string> allocations;
        boost::split(allocations,nodes_allocated_by_batsched,boost::is_any_of(" "));
        std::vector<int> node_resources;
        for(auto alloc:allocations){
            std::vector<std::string> each_allocations;
            boost::split(each_allocations,alloc,boost::is_any_of("-"));
            if(each_allocations.size()<2){
                std::string start_node = each_allocations[0];
                std::string::size_type sz;
                int start = std::stoi(start_node, &sz);
                node_resources.push_back(start);
            }else {
                std::string start_node = each_allocations[0];
                std::string end_node = each_allocations[1];
                std::string::size_type sz;
                unsigned long start = std::stoi(start_node, &sz);
                unsigned long end = std::stoi(end_node, &sz);
                for (int i = start; i < end; i++) {
                    node_resources.push_back(i);
                }
            }
        }
        unsigned long num_nodes_allocated = node_resources.size();
        unsigned long time_in_minutes = batch_job->getAllocatedTime();
        unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();

        std::set<std::pair<std::string, unsigned long>> resources = {};
        std::vector<std::string> hosts_assigned = {};
        std::map<std::string, unsigned long>::iterator it;

        for(auto node:node_resources){
            this->available_nodes_to_cores[this->host_id_to_names[node]] -= cores_per_node_asked_for;
            resources.insert(std::make_pair(this->host_id_to_names[node], cores_per_node_asked_for));
        }



        switch (workflow_job->getType()) {
            case WorkflowJob::STANDARD: {
                auto job = (StandardJob*) workflow_job;
                WRENCH_INFO("Creating a StandardJobExecutor on %ld cores for a standard job on %ld nodes",
                            cores_per_node_asked_for, num_nodes_allocated);
                // Create a standard job executor
                std::cout<<"Executor arguments "<<this->mailbox_name<<" "<<std::get<0>(*resources.begin())<<"\n";
                StandardJobExecutor *executor = new StandardJobExecutor(
                        this->simulation,
                        this->mailbox_name,
                        std::get<0>(*resources.begin()),
                        (StandardJob *) workflow_job,
                        resources,
                        this->default_storage_service,
                        {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,
                                 this->getPropertyValueAsString(
                                         BatchServiceProperty::THREAD_STARTUP_OVERHEAD)}});
                this->running_standard_job_executors.insert(std::unique_ptr<StandardJobExecutor>(executor));
                batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + time_in_minutes * 60);
//          this->running_jobs.insert(std::move(batch_job_ptr));
                this->timeslots.push_back(batch_job->getEndingTimeStamp());
                //remember the allocated resources for the job
                batch_job->setAllocatedResources(resources);

                SimulationMessage* msg =
                        new AlarmJobTimeOutMessage(job, 0);

                std::unique_ptr<Alarm> alarm_ptr = std::unique_ptr<Alarm>(new Alarm(batch_job->getEndingTimeStamp(), this->hostname, this->mailbox_name, msg,
                                                                                    "batch_standard"));

                standard_job_alarms.push_back(
                        std::move(alarm_ptr));


                return;
            }
                break;

            case WorkflowJob::PILOT: {
                PilotJob *job = (PilotJob *) workflow_job;
                WRENCH_INFO("Allocating %ld nodes with %ld cores per node to a pilot job",
                            num_nodes_allocated, cores_per_node_asked_for);

                std::string host_to_run_on = resources.begin()->first;
                std::vector<std::string> nodes_for_pilot_job = {};
                for (auto it = resources.begin(); it != resources.end(); it++) {
                    nodes_for_pilot_job.push_back(it->first);
                }

                //set the ending timestamp of the batchjob (pilotjob)

                double timeout_timestamp = std::min(job->getDuration(), time_in_minutes * 60 * 1.0);
                batch_job->setEndingTimeStamp(S4U_Simulation::getClock() + timeout_timestamp);

                ComputeService *cs =
                        new MultihostMulticoreComputeService(host_to_run_on,
                                                             true, false,
                                                             resources,
                                                             this->default_storage_service
                        );
                cs->setSimulation(this->simulation);

                // Create and launch a compute service for the pilot job
                job->setComputeService(cs);


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
                                             new ComputeServicePilotJobStartedMessage(job, this,
                                                                                      this->getPropertyValueAsDouble(
                                                                                              BatchServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
                } catch (std::shared_ptr<NetworkError> cause) {
                    throw WorkflowExecutionException(cause);
                }

                SimulationMessage* msg =
                        new AlarmJobTimeOutMessage(job, 0);
                std::unique_ptr<Alarm> alarm_ptr = std::unique_ptr<Alarm>(new Alarm(batch_job->getEndingTimeStamp(), host_to_run_on, this->mailbox_name, msg,
                                                                                    "batch_pilot"));

                this->pilot_job_alarms.push_back(
                        std::move(alarm_ptr));

                // Push my own mailbox onto the pilot job!
//          job->pushCallbackMailbox(this->mailbox_name);

                return;
            }
                break;
        }

    }

    void BatchService::notifyJobEventsToBatSched(std::string job_id,std::string status, std::string job_state, std::string kill_reason) {
        nlohmann::json batch_submission_data;
        batch_submission_data["now"] = S4U_Simulation::getClock();
        batch_submission_data["events"][0]["timestamp"] = S4U_Simulation::getClock();
        batch_submission_data["events"][0]["type"] = "JOB_COMPLETED";
        batch_submission_data["events"][0]["data"]["job_id"] = job_id;
        batch_submission_data["events"][0]["data"]["status"] = status;
        batch_submission_data["events"][0]["data"]["job_state"] = job_state;
        batch_submission_data["events"][0]["data"]["kill_reason"] = kill_reason;

        std::string data = batch_submission_data.dump();
        std::unique_ptr<BatchNetworkListener> network_listener =
            std::unique_ptr<BatchNetworkListener>(new BatchNetworkListener(this->hostname, this->mailbox_name,
                                         "14000", "28000",
                                         BatchNetworkListener::NETWORK_LISTENER_TYPE::SENDER_RECEIVER,
                                         data));

        network_listeners.push_back(std::move(network_listener));
    }



}
