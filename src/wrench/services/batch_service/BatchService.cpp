//
// Created by suraj on 8/29/17.
//

#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/batch_service/BatchService.h"
#include <simulation/SimulationMessage.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <services/ServiceMessage.h>
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "services/compute/ComputeServiceMessage.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include <algorithm>
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
     * @param plist: a property list ({} means "use all defaults")
     */
    BatchService::BatchService(std::string hostname, std::vector<std::string> nodes_in_network,
                               StorageService *default_storage_service,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               std::map<std::string, std::string> plist):
            BatchService(hostname, nodes_in_network, default_storage_service, supports_standard_jobs,
                         supports_pilot_jobs, nullptr,plist,"") {

    }

    /**
     * @brief Constructor
     * @param hostname: the hostname on which to start the service
     * @param nodes_in_network: the hosts running in the network
     * @param plist: a property list ({} means "use all defaults")
     * @param suffix: suffix to append to the service name and mailbox
     */
    BatchService::BatchService(std::string hostname, std::vector<std::string> nodes_in_network,
                               StorageService *default_storage_service,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               PilotJob* parent_pilot_job,
                               std::map<std::string, std::string> plist, std::string suffix) :
            ComputeService("batch_service" + suffix, "batch_service" + suffix,default_storage_service) {

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
            this->nodes_to_cores_map.insert({*it,S4U_Simulation::getNumCores(*it)});
            this->available_nodes_to_cores.insert({*it,S4U_Simulation::getNumCores(*it)});
        }

        this->total_num_of_nodes = nodes_in_network.size();
        this->supports_standard_jobs = supports_standard_jobs;
        this->supports_pilot_jobs = supports_pilot_jobs;

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
        if(this->timeslots.size()>0) {
            next_timeout_timestamp = *std::min_element(this->timeslots.begin(), this->timeslots.end());
        }else{
            next_timeout_timestamp = S4U_Simulation::getClock()+this->random_interval;
        }
        while (life) {
            double job_timeout = next_timeout_timestamp-S4U_Simulation::getClock();
            if (job_timeout>0){
                life = processNextMessage(job_timeout);
            }else{
                //check if some jobs have expired and should be killed
                if (this->running_jobs.size() > 0) {
                    std::cout<<"Running pilot job timestamp "<<job_timeout<<"\n";
                    std::set<BatchJob*>::iterator it;
                    for (it = this->running_jobs.begin(); it != this->running_jobs.end(); it++) {
                        if ((*it)->getEndingTimeStamp() <= S4U_Simulation::getClock());
                        {
                            if((*it)->getWorkflowJob()->getType()==WorkflowJob::STANDARD) {
                                this->failRunningStandardJob((StandardJob *) (*it)->getWorkflowJob(),
                                                             std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
                            }else if((*it)->getWorkflowJob()->getType()==WorkflowJob::PILOT){
                                PilotJob* p_job = (PilotJob*) (*it);
                                BatchService* cs = (BatchService*)p_job->getComputeService();
                                cs->terminate(true);
                            }else{
                                throw std::runtime_error(
                                        "BatchService::main(): Received a JOB type other than Standard and Pilot jobs"
                                );
                            }
                        }
                    }
                }
            }
            if(life) {
                while (this->dispatchNextPendingJob());
            }
        }

        WRENCH_INFO("Batch Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
        return 0;
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
        if (host_selection_algorithm=="FIRSTFIT"){
                std::vector<std::string> hosts_assigned = {};
                std::map<std::string,unsigned long>::iterator it;
                for(it=this->available_nodes_to_cores.begin();it!=this->available_nodes_to_cores.end();it++){
                    if((*it).second>=cores_per_node){
                        //Remove that many cores from the available_nodes_to_core
                        (*it).second-=cores_per_node;
                        hosts_assigned.push_back((*it).first);
                        resources.insert({(*it).first,cores_per_node});
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

        }
        return resources;
    }

    BatchJob* BatchService::scheduleJob(std::string job_selection_algorithm) {
        BatchJob* batch_job = nullptr;
        if (job_selection_algorithm=="FCFS"){
                batch_job = this->pending_jobs.front();
                this->pending_jobs.pop_front();
        }
        return batch_job;
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

                ComputeService *cs =
                        new wrench::BatchService(hostname,simulation->getHostnameList(),
                                                 this->default_storage_service,true,false,job,
                                                 {},"_pilot");
                cs->setSimulation(this->simulation);

                // Create and launch a compute service for the pilot job
                job->setComputeService(cs);



                //set the ending timestamp of the batchjob (pilotjob)
                unsigned long time_in_minutes = batch_job->getAllocatedTime();
                batch_job->setEndingTimeStamp(S4U_Simulation::getClock()+time_in_minutes*60);


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

                // Push my own mailbox onto the pilot job!
                job->pushCallbackMailbox(this->mailbox_name);
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
                this->failRunningStandardJob(job, std::move(cause));
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
                    cs->failCurrentStandardJobs(std::shared_ptr<FailureCause>(new ServiceIsDown(this)));
                    cs->stop();
                    // Remove the job from the running list
                    this->running_jobs.erase(workflow_job);
                    // Update the available resources
                    std::set<std::pair<std::string,unsigned long>> resources = (workflow_job)->getResourcesAllocated();
                    std::set<std::pair<std::string,unsigned long>>::iterator resource_it;
                    for(resource_it=resources.begin();resource_it!=resources.end();resource_it++){
                        this->available_nodes_to_cores[(*resource_it).first] += (*resource_it).second;
                    }

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
            // This is Synchronous
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

        }else if (StandardJobExecutorDoneMessage *msg = dynamic_cast<StandardJobExecutorDoneMessage *>(message.get())) {
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
