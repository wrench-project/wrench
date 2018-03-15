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

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "wrench/services/compute/batch/BatchJob.h"
#include "wrench/services/compute/batch/BatchNetworkListener.h"
#include "wrench/services/compute/batch/BatchServiceProperty.h"
#include "wrench/services/helpers/Alarm.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include <deque>
#include <queue>
#include <set>
#include <tuple>

namespace wrench {

    class BatchService : public ComputeService {

        /**
         * @brief A Batch Service
         */
    private:

        std::map<std::string, std::string> default_property_values =
                {{BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,                 "1024"},
                 {BatchServiceProperty::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD,"1024"},
                 {BatchServiceProperty::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, "1024"},
                 {BatchServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,              "1024"},
                 {BatchServiceProperty::THREAD_STARTUP_OVERHEAD,                     "0"},
                 {BatchServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {BatchServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {BatchServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                 {BatchServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                 {BatchServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,         "1024"},
                 {BatchServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,           "1024"},
                 {BatchServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {BatchServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {BatchServiceProperty::BATCH_FAKE_JOB_REPLY_MESSAGE_PAYLOAD,        "1024"},
                 {BatchServiceProperty::HOST_SELECTION_ALGORITHM,                    "FIRSTFIT"},
                 {BatchServiceProperty::JOB_SELECTION_ALGORITHM,                     "FCFS"},
                 {BatchServiceProperty::SCHEDULER_REPLY_MESSAGE_PAYLOAD,             "1024"},
                 {BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "easy_bf"},
                 {BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM,              "fcfs"},
                 {BatchServiceProperty::BATCH_FAKE_SUBMISSION,                       "false"}
                };

    public:
        BatchService(std::string &hostname,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                     std::vector<std::string> compute_hosts,
                     StorageService *default_storage_service,
                     std::map<std::string, std::string> plist = {});

        //cancels the job
        void cancelJob(unsigned long jobid);

        //returns jobid,started time, running time
        std::vector<std::tuple<unsigned long, double, double>> getJobsInQueue();

        std::map<std::string,double> getQueueWaitingTimeEstimate(std::set<std::tuple<std::string,unsigned int,double>>);

        ~BatchService();


    private:
        BatchService(std::string hostname,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                     std::vector<std::string> compute_hosts,
                     StorageService *default_storage_service,
                     unsigned long cores_per_host,
                     std::map<std::string, std::string> plist,
                     std::string suffix);

#ifdef ENABLE_BATSCHED
        unsigned int self_port;
        unsigned int batsched_port;
#endif

        //Configuration to create randomness in measurement period initially
        unsigned long random_interval = 10;

        //create alarms for standard jobs
        std::map<std::string,std::shared_ptr<Alarm>> standard_job_alarms;

        //alarms for pilot jobs (only one pilot job alarm)
        std::map<std::string,std::shared_ptr<Alarm>> pilot_job_alarms;

        //vector of network listeners
        std::vector<std::shared_ptr<BatchNetworkListener>> network_listeners;

        /* Resources information in Batchservice */
        unsigned long total_num_of_nodes;
        std::map<std::string, unsigned long> nodes_to_cores_map;
        std::vector<double> timeslots;
        std::map<std::string, unsigned long> available_nodes_to_cores;
        std::map<unsigned long, std::string> host_id_to_names;
        /*End Resources information in Batchservice */

        // Vector of standard job executors
        std::set<std::shared_ptr<StandardJobExecutor>> running_standard_job_executors;

        // Vector of standard job executors
        std::set<std::shared_ptr<StandardJobExecutor>> finished_standard_job_executors;

        //Queue of pending batch jobs
        std::deque<std::unique_ptr<BatchJob>> pending_jobs;
        //A set of running batch jobs
        std::set<std::unique_ptr<BatchJob>> running_jobs;
        // A set of waiting jobs that have been submitted to batsched, but not scheduled
        std::set<std::unique_ptr<BatchJob>> waiting_jobs;

        //Batch Service request reply process
        std::unique_ptr<BatchNetworkListener> request_reply_process;

        //Batch scheduling supported algorithms
        std::set<std::string> scheduling_algorithms = {"easy_bf", "conservative_bf", "easy_bf_plot_liquid_load_horizon",
                                                       "energy_bf", "energy_bf_dicho", "energy_bf_idle_sleeper",
                                                       "energy_bf_monitoring",
                                                       "energy_bf_monitoring_inertial", "energy_bf_subpart_sleeper",
                                                       "filler", "killer", "killer2", "rejecter", "sleeper",
                                                       "submitter", "waiting_time_estimator"
        };

        //Batch queue ordering options
        std::set<std::string> queue_ordering_options = {"fcfs", "lcfs", "desc_bounded_slowdown", "desc_slowdown",
                                                        "asc_size", "desc_size", "asc_walltime", "desc_walltime"

        };


        pid_t pid;

        //Is sched ready?
        bool is_bat_sched_ready;

        //timestamp received from batscheduler
        double batsched_timestamp;

        //fork the batsched_process
        void run_batsched();

        unsigned long generateUniqueJobId();

        std::string foundRunningJobOnTheList(WorkflowJob *job);

        std::string convertAvailableResourcesToJsonString(std::map<std::string, unsigned long>);

        std::string convertResourcesToJsonString(std::set<std::tuple<std::string, unsigned long, double>>);

        //submits a standard job
        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> &batch_job_args) override;

        //submits a standard job
        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &batch_job_args) override;

        // terminate a standard job
        void terminateStandardJob(StandardJob *job) override;

        // terminate a pilot job
        void terminatePilotJob(PilotJob *job) override;

        int main() override;

        bool processNextMessage();

        bool dispatchNextPendingJob();

        void processGetResourceInformation(const std::string &answer_mailbox);

        void processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job);

        void processStandardJobFailure(StandardJobExecutor *executor,
                                       StandardJob *job,
                                       std::shared_ptr<FailureCause> cause);

        void failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);

        void failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);

        void terminateRunningStandardJob(StandardJob *job);

        std::set<std::tuple<std::string, unsigned long, double>> scheduleOnHosts(std::string host_selection_algorithm,
                                                                                 unsigned long, unsigned long, double);

        BatchJob *scheduleJob(std::string);

        //Terminate the batch service (this is usually for pilot jobs when they act as a batch service)
        void cleanup() override;

        //Fail the standard jobs inside the pilot jobs
        void failCurrentStandardJobs(std::shared_ptr<FailureCause> cause);

        //Process the pilot job completion
        void processPilotJobCompletion(PilotJob *job);

        //Process standardjob timeout
        void processStandardJobTimeout(StandardJob *job);

        //process pilot job termination request
        void processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox);

        //Process standardjob timeout
        void processPilotJobTimeout(PilotJob *job);

        //notify upper level job submitters (about pilot job termination)
        void notifyJobSubmitters(PilotJob *job);

        //update the resources
        void updateResources(std::set<std::tuple<std::string, unsigned long, double>> resources);

        void updateResources(StandardJob *job);

        //send call back to the pilot job submitters
        void sendPilotJobCallBackMessage(PilotJob *job);

        //send call back to the standard job submitters
        void sendStandardJobCallBackMessage(StandardJob *job);

        //send all the jobs in the queue to the batscheduler
        bool scheduleAllQueuedJobs();

        // process a job submission
        void processJobSubmission(BatchJob *job, std::string answer_mailbox);

        //process execute events from batsched
        void processExecuteJobFromBatSched(std::string bat_sched_reply);

        //process execution of job
        void processExecution(std::set<std::tuple<std::string, unsigned long, double>>, WorkflowJob *,
                              BatchJob *, unsigned long, unsigned long, unsigned long);

        //notify batsched about job completion/failure/killed events
        void notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                       std::string kill_reason);


    };
}


#endif //WRENCH_BATCH_SERVICE_H
