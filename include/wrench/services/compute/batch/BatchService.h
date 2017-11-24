/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_BATCH_SERVICE_H
#define WRENCH_BATCH_SERVICE_H

#include "wrench/services/Service.h"
#include <queue>
#include <deque>
#include "wrench/workflow/job/StandardJob.h"
#include "BatchServiceProperty.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include <tuple>
#include "BatchJob.h"
#include "BatchScheduler.h"
#include <set>
#include <wrench/services/helpers/Alarm.h>

namespace wrench {


    /**
     * @brief A batch-scheduled ComputeService
     */
    class BatchService: public ComputeService {




    private:
        std::map<std::string, std::string> default_property_values =
                {{BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,          "1024"},
                 {BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,       "1024"},
                 {BatchServiceProperty::THREAD_STARTUP_OVERHEAD,              "0"},
                 {BatchServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,    "1024"},
                 {BatchServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                 {BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,    "1024"},
                 {BatchServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,        "1024"},
                 {BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,        "1024"},
                 {BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,  "1024"},
                 {BatchServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,    "1024"},
                 {BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::HOST_SELECTION_ALGORITHM,           "FIRSTFIT"},
                 {BatchServiceProperty::JOB_SELECTION_ALGORITHM,           "FCFS"}
                };

    public:

        /* Public constructor */
        BatchService(std::string hostname,
        std::vector<std::string> compute_nodes,
                     StorageService *default_storage_service,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                std::map<std::string, std::string> plist = {});

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

       ~BatchService();

        //cancels the job
//        void cancelJob(unsigned long jobid);
        //returns jobid,started time, running time
        std::vector<std::tuple<unsigned long,double,double>> getJobsInQueue();

        /***********************/
        /** \endcond           */
        /***********************/


    private:
        BatchService(std::string hostname,
        std::vector<std::string> nodes_in_network,
        StorageService *default_storage_service,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                     unsigned long reduced_cores,
        std::map<std::string, std::string> plist,
        std::string suffix);

        //Configuration to create randomness in measurement period initially
//        unsigned long random_interval = 10;

        //create alarms for standardjobs
        std::vector<std::unique_ptr<Alarm>> standard_job_alarms;

        //alarms for pilot jobs (only one pilot job alarm)
        std::vector<std::unique_ptr<Alarm>> pilot_job_alarms;

//        std::vector<std::shared_ptr<SimulationMessage>> sent_alrm_msgs;

        /* Resources information in Batchservice */
        unsigned long total_num_of_nodes;
        std::map<std::string,unsigned long> nodes_to_cores_map;
//        std::vector<double> timeslots;
        std::map<std::string,unsigned long> available_nodes_to_cores;
        /*End Resources information in Batchservice */

        // Vector of standard job executors
        std::set<std::unique_ptr<StandardJobExecutor>> running_standard_job_executors;

        // Vector of standard job executors
        std::set<std::unique_ptr<StandardJobExecutor>> finished_standard_job_executors;

        //Queue of pending batch jobs
        std::deque<std::unique_ptr<BatchJob>> pending_jobs;
        //A set of running batch jobs
        std::set<std::unique_ptr<BatchJob>> running_jobs;

        unsigned long generateUniqueJobId();

        bool foundRunningJobOnTheList(WorkflowJob* job);

        //submits the standard job
        //overriden function of parent Compute Service
        void submitStandardJob(StandardJob *job,std::map<std::string, std::string> &batch_job_args) override;

        //submits the standard job
        //overriden function of parent Compute Service
        void submitPilotJob(PilotJob *job,std::map<std::string, std::string> &batch_job_args) override;

        int main() override;
        bool processNextMessage();
        bool dispatchNextPendingJob();
        void processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job);

        void processStandardJobFailure(StandardJobExecutor *executor,
                                       StandardJob *job,
                                       std::shared_ptr<FailureCause> cause);

        void failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);
        void failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);
        void terminateRunningStandardJob(StandardJob *job);
        void terminatePilotJob(PilotJob* job) override ;

        std::set<std::pair<std::string,unsigned long>> scheduleOnHosts(std::string host_selection_algorithm,
                                                                       unsigned long, unsigned long);

        std::unique_ptr<BatchJob> scheduleJob(std::string);

        //Terminate the batch service (this is usually for pilot jobs when they act as a batch service)
        void terminate(bool);

        //Fail the standard jobs inside the pilot jobs
        void failCurrentStandardJobs(std::shared_ptr<FailureCause> cause);

        //Process the pilot job completion
        void processPilotJobCompletion(PilotJob* job);

        //Process standardjob timeout
        void processStandardJobTimeout(StandardJob* job);

        //process pilot job termination request
        void processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox);

        //Process standardjob timeout
        void processPilotJobTimeout(PilotJob* job);

        //update the resources
        void updateResources(std::set<std::pair<std::string,unsigned long>> resources);
        void updateResources(StandardJob* job);

        //send call back to the pilot job submitters
        void sendPilotJobCallBackMessage(PilotJob* job);

        //send call back to the standard job submitters
        void sendStandardJobCallBackMessage(StandardJob*job);

    };
}


#endif //WRENCH_BATCH_SERVICE_H
