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
#include "wrench/services/batch_service/BatchService.h"
#include <simulation/SimulationMessage.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "services/compute/ComputeServiceMessage.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/services/batch_service/BatchServiceMessage.h"

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
                               std::map<std::string, std::string> plist):
            BatchService(hostname, nodes_in_network, default_storage_service, supports_standard_jobs,
                         supports_pilot_jobs, nullptr,0,plist,"") {

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
                               PilotJob* parent_pilot_job,
                               unsigned long reduced_cores,
                               std::map<std::string, std::string> plist, std::string suffix) :
            ComputeService("batch_service" + suffix,
                           "batch_service" + suffix,
                           supports_standard_jobs,
                           supports_pilot_jobs,
                           default_storage_service) {

        // Set default properties
        for (auto p : this->default_property_values) {
            this->setProperty(p.first, p.second);
        }

        // Set specified properties
        for (auto p : plist) {
            this->setProperty(p.first, p.second);
        }


        //create a map for host to cores
        std::vector<std::string>::iterator it;
        for (it=nodes_in_network.begin();it!=nodes_in_network.end();it++){
            if(reduced_cores>0 && not this->supports_pilot_jobs){
                this->nodes_to_cores_map.insert({*it, reduced_cores});
                this->available_nodes_to_cores.insert({*it, reduced_cores});
            }else {
                this->nodes_to_cores_map.insert({*it, S4U_Simulation::getNumCores(*it)});
                this->available_nodes_to_cores.insert({*it, S4U_Simulation::getNumCores(*it)});
            }
        }

        this->total_num_of_nodes = nodes_in_network.size();
        this->parent_pilot_job = parent_pilot_job;
        this->hostname = hostname;

        this->generateUniqueJobId();

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
    unsigned long BatchService::submitStandardJob(StandardJob *job,std::map<std::string,unsigned long> batch_job_args) {

        if (this->state == Service::DOWN) {
            throw WorkflowExecutionException(new ServiceIsDown(this));
        }

        //check if there are enough hosts
        unsigned long nodes_asked_for = 0;
        std::map<std::string,unsigned long>::iterator it;
        it = batch_job_args.find("-N");
        if (it != batch_job_args.end()){
            if ((*it).second > this->total_num_of_nodes){
                throw std::runtime_error(
                        "BatchService::submitStandardJob(): There are not enough hosts");
            }
        }else{
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Batch Service requires -N(number of nodes) to be specified "
            );
        }
        nodes_asked_for = (*it).second;


        //check if there are enough cores per node
        unsigned long nodes_with_c_cores = 0;
        it = batch_job_args.find("-c");
        if (it != batch_job_args.end()){
            std::map<std::string,unsigned long>::iterator nodes_to_core_iterator;
            for(nodes_to_core_iterator=this->nodes_to_cores_map.begin();nodes_to_core_iterator!=this->nodes_to_cores_map.end();nodes_to_core_iterator++){
                if ((*it).second <= (*nodes_to_core_iterator).second){
                    nodes_with_c_cores++;
                }
            }
        }else{
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Batch Service requires -c(cores per node) to be specified "
            );
        }
        if (nodes_asked_for > nodes_with_c_cores){
            throw std::runtime_error(
                    "BatchService::submitStandardJob(): There are not enough hosts with those amount of cores per host");
        }
        unsigned long cores_asked_for = (*it).second;

        //get job time
        unsigned long time_asked_for = 0;
        it = batch_job_args.find("-t");
        if (it != batch_job_args.end()){
            time_asked_for = (*it).second;
        }else{
            throw std::invalid_argument(
                    "BatchService::submitStandardJob(): Batch Service requires -t to be specified "
            );
        }

        //Create a Batch Job
        unsigned long jobid = this->generateUniqueJobId();
        BatchJob* batch_job = new BatchJob(job,jobid,time_asked_for,nodes_asked_for,cores_asked_for,-1);

        //  send a "run a batch job" message to the daemon's mailbox
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_standard_job_mailbox");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new BatchServiceJobRequestMessage(answer_mailbox, batch_job,
                                                                                      this->getPropertyValueAsDouble(
                                                                                              BatchServiceProperty::SUBMIT_BATCH_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

        return jobid;

    }


    /**
     * @brief Synchronously submit a pilot job to the compute service
     *
     * @param job: a pilot job
     * @param batch_job_args: arugments to the batch job
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    unsigned long BatchService::submitPilotJob(PilotJob *job,std::map<std::string,unsigned long> batch_job_args) {

        if (this->state == Service::DOWN) {
            throw WorkflowExecutionException(new ServiceIsDown(this));
        }

        //check if there are enough hosts
        unsigned long nodes_asked_for = 0;
        std::map<std::string,unsigned long>::iterator it;
        it = batch_job_args.find("-N");
        if (it != batch_job_args.end()){
            if ((*it).second > this->total_num_of_nodes){
                throw std::runtime_error(
                        "BatchService::submitPilotJob(): There are not enough hosts");
            }
        }else{
            throw std::invalid_argument(
                    "BatchService::submitPilotJob(): Batch Service requires -N(number of nodes) to be specified "
            );
        }
        nodes_asked_for = (*it).second;


        //check if there are enough cores per node
        unsigned long nodes_with_c_cores = 0;
        it = batch_job_args.find("-c");
        if (it != batch_job_args.end()){
            std::map<std::string,unsigned long>::iterator nodes_to_core_iterator;
            for(nodes_to_core_iterator=this->nodes_to_cores_map.begin();nodes_to_core_iterator!=this->nodes_to_cores_map.end();nodes_to_core_iterator++){
                if ((*it).second <= (*nodes_to_core_iterator).second){
                    nodes_with_c_cores++;
                }
            }
        }else{
            throw std::invalid_argument(
                    "BatchService::submitPilotJob(): Batch Service requires -c(cores per node) to be specified "
            );
        }
        if (nodes_asked_for > nodes_with_c_cores){
            throw std::runtime_error(
                    "BatchService::submitPilotJob(): There are not enough hosts with those amount of cores per host");
        }
        unsigned long cores_asked_for = (*it).second;

        //get job time
        unsigned long time_asked_for = 0;
        it = batch_job_args.find("-t");
        if (it != batch_job_args.end()){
            time_asked_for = (*it).second;
        }else{
            throw std::invalid_argument(
                    "BatchService::submitPilotJob(): Batch Service requires -t to be specified "
            );
        }

        //Create a Batch Job
        unsigned long jobid = this->generateUniqueJobId();
        BatchJob* batch_job = new BatchJob(job,jobid,time_asked_for,nodes_asked_for,cores_asked_for,-1);

        //  send a "run a batch job" message to the daemon's mailbox
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("batch_pilot_job_mailbox");
        try {
            S4U_Mailbox::putMessage(this->mailbox_name,
                                    new BatchServiceJobRequestMessage(answer_mailbox, std::move(batch_job),
                                                                      this->getPropertyValueAsDouble(
                                                                              BatchServiceProperty::SUBMIT_BATCH_JOB_REQUEST_MESSAGE_PAYLOAD)));
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
        return jobid;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int BatchService::main() {

        TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_MAGENTA);

        WRENCH_INFO("Batch Service starting on host %s!", S4U_Simulation::getHostName().c_str());


        /** Main loop **/
        bool life = true;
        double next_timeout_timestamp = 0;
        next_timeout_timestamp = S4U_Simulation::getClock()+this->random_interval;
        while (life) {
            double job_timeout = next_timeout_timestamp-S4U_Simulation::getClock();
            if (job_timeout>0){
                life = processNextMessage(this->random_interval);
            }else{
                //check if some jobs have expired and should be killed
                if (this->running_jobs.size() > 0) {
                    std::set<BatchJob*>::iterator it;
                    for (it = this->running_jobs.begin(); it != this->running_jobs.end();) {
                        if ((*it)->getEndingTimeStamp() <= S4U_Simulation::getClock()) {
                            if((*it)->getWorkflowJob()->getType()==WorkflowJob::STANDARD) {
                                this->processStandardJobTimeout((StandardJob*)(*it)->getWorkflowJob());
                                this->udpateResources((*it)->getResourcesAllocated());
                                it = this->running_jobs.erase(it);
                            }else if((*it)->getWorkflowJob()->getType()==WorkflowJob::PILOT){
                                this->processPilotJobTimeout((PilotJob*)(*it)->getWorkflowJob());
                                this->udpateResources((*it)->getResourcesAllocated());
                                this->sendPilotJobCallBackMessage((PilotJob*)(*it)->getWorkflowJob());
                                it = this->running_jobs.erase(it);
                            }else{
                                throw std::runtime_error(
                                        "BatchService::main(): Received a JOB type other than Standard and Pilot jobs"
                                );
                            }
                        }else{
                            ++it;
                        }

                    }
                }
                next_timeout_timestamp = S4U_Simulation::getClock()+this->random_interval;
            }
            if(life) {
                while (this->dispatchNextPendingJob());
            }
        }

        WRENCH_INFO("Batch Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    void BatchService::sendPilotJobCallBackMessage(PilotJob* job){
        try {
            S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                     new ComputeServicePilotJobExpiredMessage(job, this,
                                                                              this->getPropertyValueAsDouble(
                                                                                      BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> cause) {
            return;
        }
    }

    void BatchService::udpateResources(std::set<std::pair<std::string,unsigned long>> resources){
        if(resources.empty()){
            return;
        }
        std::set<std::pair<std::string,unsigned long>>::iterator resource_it;
        for(resource_it=resources.begin();resource_it!=resources.end();resource_it++){
            this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
        }
    }


    void BatchService::processPilotJobTimeout(PilotJob* job){
        BatchService* cs = (BatchService*)job->getComputeService();
        if (cs == nullptr) {
            throw std::runtime_error("BatchService::terminate(): can't find compute service associated to pilot job");
        }
        cs->stop();
    }

    void BatchService::processStandardJobTimeout(StandardJob *job) {
        StandardJobExecutor *executor = nullptr;
        for (auto e : this->standard_job_executors) {
            if (e->getJob() == job) {
                executor = e;
            }
        }
        if (executor == nullptr) {
            throw std::runtime_error("BatchService::processStandardJobTimeout(): Cannot find standard job executor corresponding to job being timedout");
        }

        // Terminate the executor
        WRENCH_INFO("Terminating a standard job executor");
        executor->kill();

        this->standard_job_executors.erase(executor);
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
        for (auto e : this->standard_job_executors) {
            if (e->getJob() == job) {
                executor = e;
            }
        }
        if (executor == nullptr) {
            throw std::runtime_error("MulticoreComputeService::terminateRunningStandardJob(): Cannot find standard job executor corresponding to job being terminated");
        }

        // Terminate the executor
        WRENCH_INFO("Terminating a standard job executor");
        executor->kill();

        //TODO: Restore the allocated resources

        return;
    }

    std::set<std::pair<std::string,unsigned long>> BatchService::scheduleOnHosts(std::string host_selection_algorithm,
                                                                                 unsigned long num_nodes, unsigned long cores_per_node) {
        std::set<std::pair<std::string,unsigned long>> resources = {};
        std::vector<std::string> hosts_assigned = {};
        if (host_selection_algorithm=="FIRSTFIT"){
                std::map<std::string,unsigned long>::iterator it;
                unsigned long host_count = 0;
                for(it=this->available_nodes_to_cores.begin();it!=this->available_nodes_to_cores.end();it++){
                    if((*it).second>=cores_per_node){
                        //Remove that many cores from the available_nodes_to_core
                        (*it).second-=cores_per_node;
                        hosts_assigned.push_back((*it).first);
                        resources.insert(std::pair<std::string,unsigned long>((*it).first,cores_per_node));
                        if(++host_count>=num_nodes){
                            break;
                        }
                    }
                }
                if(resources.size()<num_nodes){
                    resources = {};
                    std::vector<std::string>::iterator it;
                    for(it=hosts_assigned.begin();it!=hosts_assigned.end();it++) {
                        available_nodes_to_cores[*it] += cores_per_node;
                    }
                }
        }else if (host_selection_algorithm=="BESTFIT"){
            while(resources.size()<num_nodes) {
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
                    unsigned long tentative_target_slack = num_available_cores - tentative_target_num_cores;

                    if ((target_host == "") ||
                        (tentative_target_num_cores > target_num_cores) ||
                        ((tentative_target_num_cores == target_num_cores) && (target_slack > tentative_target_slack))) {
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
                resources.insert(std::pair<std::string,unsigned long>(target_host,cores_per_node));
            }
        }else{
            throw std::invalid_argument(
                    "BatchService::scheduleOnHosts(): We don't support "+host_selection_algorithm +" as host selection algorithm"
            );
        }

        return resources;
    }

    BatchJob* BatchService::scheduleJob(std::string job_selection_algorithm) {
        if (job_selection_algorithm=="FCFS"){
                BatchJob* batch_job = this->pending_jobs.front();
                this->pending_jobs.pop_front();
                return batch_job;
        }
        return nullptr;
    }

    /**
     * @brief Dispatch one pending job, if possible
     * @return true if a job was dispatched, false otherwise
     */
    bool BatchService::dispatchNextPendingJob() {

        if (this->pending_jobs.size() == 0) {
            return false;
        }

        //Currently we have only FCFS scheduling
        BatchJob* batch_job = this->scheduleJob(this->getPropertyValueAsString(BatchServiceProperty::JOB_SELECTION_ALGORITHM));
        if(batch_job== nullptr){
            throw std::runtime_error(
                    "BatchService::dispatchNextPendingJob(): Got no such job in pending queue to dispatch"
            );
        }

        WorkflowJob *next_job = batch_job->getWorkflowJob();


        /* Get the nodes and cores per nodes asked for */
        unsigned long cores_per_node_asked_for = batch_job->getAllocatedCoresPerNode();
        unsigned long num_nodes_asked_for = batch_job->getNumNodes();

        //Try to schedule hosts based on FIRSTFIT OR BESTFIT
        std::set<std::pair<std::string,unsigned long>> resources = this->scheduleOnHosts(this->getPropertyValueAsString(BatchServiceProperty::HOST_SELECTION_ALGORITHM),
                                                                            num_nodes_asked_for,cores_per_node_asked_for);

        if(resources.empty()){
            return false;
        }


        switch (next_job->getType()) {
            case WorkflowJob::STANDARD: {
                WRENCH_INFO("Creating a StandardJobExecutor on %ld cores for a standard job on %ld nodes", cores_per_node_asked_for,num_nodes_asked_for);
                // Create a standard job executor
                unsigned long time_in_minutes = batch_job->getAllocatedTime();
                StandardJobExecutor *executor = new StandardJobExecutor(
                        this->simulation,
                        this->mailbox_name,
                        std::get<0>(*resources.begin()),
                        (StandardJob *)next_job,
                        resources,
                        this->default_storage_service,
                        {{StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD,
                                 this->getPropertyValueAsString(BatchServiceProperty::THREAD_STARTUP_OVERHEAD)}});

                this->standard_job_executors.insert(executor);
                batch_job->setEndingTimeStamp(S4U_Simulation::getClock()+time_in_minutes*60);
                this->running_jobs.insert(batch_job);
                this->timeslots.push_back(batch_job->getEndingTimeStamp());
                //remember the allocated resources for the job
                batch_job->setAllocatedResources(resources);
                return true;
            }
            break;

            case WorkflowJob::PILOT: {
                PilotJob *job = (PilotJob *) next_job;
                WRENCH_INFO("Allocating %ld nodes with %ld cores per node to a pilot job", num_nodes_asked_for, cores_per_node_asked_for);

                std::string host_to_run_on = resources.begin()->first;
                std::vector<std::string> nodes_for_pilot_job = {};
                for(auto it=resources.begin();it!=resources.end();it++){
                    nodes_for_pilot_job.push_back(it->first);
                }

                ComputeService *cs =
                        new wrench::BatchService(host_to_run_on,nodes_for_pilot_job,
                                                 this->default_storage_service,true,false,job,cores_per_node_asked_for,
                                                 {},"_pilot");
                cs->setSimulation(this->simulation);

                // Create and launch a compute service for the pilot job
                job->setComputeService(cs);



                //set the ending timestamp of the batchjob (pilotjob)
                unsigned long time_in_minutes = batch_job->getAllocatedTime();
                double timeout_timestamp = std::min(job->getDuration(),time_in_minutes*60*1.0);
                batch_job->setEndingTimeStamp(S4U_Simulation::getClock()+timeout_timestamp);


                // Put the job in the running queue
                this->running_jobs.insert(batch_job);
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

                return true;
            }
            break;
        }
    }

    /**
    * @brief Declare all current jobs as failed (likely because the daemon is being terminated
    * or has timed out (because it's in fact a pilot job))
    */
    void BatchService::failCurrentStandardJobs(std::shared_ptr<FailureCause> cause) {
        for (auto workflow_job : this->running_jobs) {
            if (workflow_job->getWorkflowJob()->getType() == WorkflowJob::STANDARD) {
                StandardJob *job = (StandardJob*) workflow_job->getWorkflowJob();
                this->failRunningStandardJob(job, cause);
            }
        }

        while (not this->pending_jobs.empty()) {
            WorkflowJob *workflow_job = this->pending_jobs.front()->getWorkflowJob();
            this->pending_jobs.pop_back();
            if (workflow_job->getType() == WorkflowJob::STANDARD) {
                StandardJob *job = (StandardJob *) workflow_job;
                this->failPendingStandardJob(job, cause);
            }
        }
    }


    /**
     * @brief Notify upper level job submitters
     */
    void BatchService::notifyJobSubmitters(PilotJob* job) {

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
     * @brief Terminate the daemon, dealing with pending/running jobs
     */
    void BatchService::terminate(bool notify_pilot_job_submitters) {
        this->setStateToDown();

        WRENCH_INFO("Failing current standard jobs");
        this->failCurrentStandardJobs(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));

        if(this->supports_pilot_jobs){
            for(auto workflow_job:this->running_jobs){
                if(workflow_job->getWorkflowJob()->getType()==WorkflowJob::PILOT){
                    PilotJob* p_job = (PilotJob*) (workflow_job->getWorkflowJob());
                    BatchService* cs = (BatchService*)p_job->getComputeService();
                    if (cs == nullptr) {
                        throw std::runtime_error("BatchService::terminate(): can't find compute service associated to pilot job");
                    }
                    cs->stop();
                }
            }
        }

        if (notify_pilot_job_submitters && this->parent_pilot_job) {

            WRENCH_INFO("Letting the level above know that the pilot job has ended on mailbox %s",
                        this->parent_pilot_job->getCallbackMailbox().c_str());
            try {
                S4U_Mailbox::putMessage(this->parent_pilot_job->popCallbackMailbox(),
                                        new ComputeServicePilotJobExpiredMessage(this->parent_pilot_job, this,
                                                                                 this->getPropertyValueAsDouble(
                                                                                         BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> cause) {
                return;
            }
        }
    }


    bool BatchService::processNextMessage(double timeout) {

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name,timeout);
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
            this->terminate(false);
            // This is Synchronous;
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                                BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));

            } catch (std::shared_ptr<NetworkError> cause) {
                return false;
            }
            return false;

        }else if (BatchServiceJobRequestMessage *msg = dynamic_cast<BatchServiceJobRequestMessage *>(message.get())) {
            WRENCH_INFO("Asked to run a batch job using batchservice with jobid %ld",msg->job->getJobID());
            if (msg->job->getWorkflowJob()->getType()==WorkflowJob::STANDARD) {
                if (not this->supports_standard_jobs) {
                    try {
                        S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                                 new ComputeServiceSubmitStandardJobAnswerMessage(
                                                         (StandardJob *) msg->job->getWorkflowJob(), this,
                                                         false,
                                                         std::shared_ptr<FailureCause>(
                                                                 new JobTypeNotSupported(msg->job->getWorkflowJob(),
                                                                                         this)),
                                                         this->getPropertyValueAsDouble(
                                                                 BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
                    } catch (std::shared_ptr<NetworkError> cause) {
                        return true;
                    }
                    return true;
                }
            }else if(msg->job->getWorkflowJob()->getType()==WorkflowJob::PILOT){
                if (not this->supports_pilot_jobs) {
                    try {
                        S4U_Mailbox::dputMessage(msg->answer_mailbox,
                                                 new ComputeServiceSubmitPilotJobAnswerMessage(
                                                         (PilotJob *) msg->job->getWorkflowJob(), this,
                                                         false,
                                                         std::shared_ptr<FailureCause>(
                                                                 new JobTypeNotSupported(msg->job->getWorkflowJob(),
                                                                                         this)),
                                                         this->getPropertyValueAsDouble(
                                                                 BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
                    } catch (std::shared_ptr<NetworkError> cause) {
                        return true;
                    }
                    return true;
                }
            }

            this->pending_jobs.push_back(msg->job);

            if (msg->job->getWorkflowJob()->getType()==WorkflowJob::STANDARD) {

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
            }else if(msg->job->getWorkflowJob()->getType()==WorkflowJob::PILOT){
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

        }  else {
            throw std::runtime_error(
                    "BatchService::waitForNextMessage(): Unknown message type: " + std::to_string(message->payload));
        }
    }


    /**
     * @brief Process a pilot job completion
     *
     * @param job: the pilot job
     */
    void BatchService::processPilotJobCompletion(PilotJob *job) {

        // Remove the job from the running job list
        bool job_on_the_list = false;
        for(auto it=this->running_jobs.begin();it!=this->running_jobs.end();it++){
            if((*it)->getWorkflowJob()==job){
                job_on_the_list = true;
                this->running_jobs.erase(it);
                // Update the cores count in the available resources
                std::set<std::pair<std::string,unsigned long>> resources = (*it)->getResourcesAllocated();
                std::set<std::pair<std::string,unsigned long>>::iterator resource_it;
                for(resource_it=resources.begin();resource_it!=resources.end();resource_it++){
                    this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
                }
                break;
            }
        }
        if(!job_on_the_list){
            throw std::runtime_error("BatchService::processPilotJobCompletion():  Pilot job completion message recevied but no such pilot jobs found in queue"
            );
        }

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
     * @brief Process a standard job completion
     * @param executor: the standard job executor
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void BatchService::processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job) {
        // Remove the executor from the executor list
        WRENCH_INFO("====> %ld", this->standard_job_executors.size());
        if (this->standard_job_executors.find(executor) == this->standard_job_executors.end()) {
            throw std::runtime_error("BatchService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }
        this->standard_job_executors.erase(executor);

        // Remove the job from the running job list
        bool job_on_the_list = false;

        for(auto it=this->running_jobs.begin();it!=this->running_jobs.end();it++){
            if((*it)->getWorkflowJob()==job){
                job_on_the_list = true;
                this->running_jobs.erase(it);
                // Update the cores count in the available resources
                std::set<std::pair<std::string,unsigned long>> resources = (*it)->getResourcesAllocated();
                std::set<std::pair<std::string,unsigned long>>::iterator resource_it;
                for(resource_it=resources.begin();resource_it!=resources.end();resource_it++){
                    this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
                }
                break;
            }
        }

        if(!job_on_the_list){
            throw std::runtime_error("BatchService::processStandardJobCompletion(): Received a standard job completion, but the job is not in the running job list");
        }


        WRENCH_INFO("A standard job executor has completed job %s", job->getName().c_str());

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

        // Remove the executor from the executor list
        if (this->standard_job_executors.find(executor) == this->standard_job_executors.end()) {
            throw std::runtime_error("MulticoreComputeService::processStandardJobCompletion(): Received a standard job completion, but the executor is not in the executor list");
        }
        this->standard_job_executors.erase(executor);

        // Remove the job from the running job list
        bool job_on_the_list = false;
        std::set<BatchJob*>::iterator it;
        for(it=this->running_jobs.begin();it!=this->running_jobs.end();it++){
            if((*it)->getWorkflowJob()==job){
                job_on_the_list = true;
                this->running_jobs.erase(*it);
                // Update the cores count in the available resources
                std::set<std::pair<std::string,unsigned long>> resources = (*it)->getResourcesAllocated();
                std::set<std::pair<std::string,unsigned long>>::iterator resource_it;
                for(resource_it=resources.begin();resource_it!=resources.end();resource_it++){
                    this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
                }
                break;
            }
        }
        if(!job_on_the_list){
            throw std::runtime_error("BatchService::processStandardJobFailure(): Received a standard job completion, but the job is not in the running job list");
        }

        WRENCH_INFO("A standard job executor has failed to perform job %s", job->getName().c_str());

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

    unsigned long BatchService::generateUniqueJobId(){
        static unsigned long jobid = 1;
        return jobid++;
    }


}
