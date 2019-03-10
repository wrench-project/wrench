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
#include "wrench/services/compute/batch/BatschedNetworkListener.h"
#include "wrench/services/compute/batch/BatchServiceProperty.h"
#include "wrench/services/compute/batch/BatchServiceMessagePayload.h"
#include "wrench/services/helpers/Alarm.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include <deque>
#include <queue>
#include <set>
#include <tuple>

namespace wrench {

    class WorkloadTraceFileReplayer; // forward

    /**
     * @brief A batch-scheduled compute service that manages a set of compute hosts and
     *        controls access to their resource via a batch queue.
     *
     *        In the current implementation of
     *        this service, like for many of its real-world counterparts, memory
     *        partitioning among jobs on the same host is not handled.  When multiple jobs share hosts,
     *        which can happen when jobs require only a few cores per host and can thus
     *        be co-located on the same hosts in a non-exclusive fashion,
     *        each job simply runs as if it had access to the
     *        full RAM of each compute host it is scheduled on. The simulation of these
     *        memory contended scenarios is thus, for now, not realistic as there is no simulation
     *        of the effects
     *        of memory sharing (e.g., swapping).
     */
    class BatchService : public ComputeService {

        /**
         * @brief A Batch Service
         */
    private:

        std::map<std::string, std::string> default_property_values = {
                {BatchServiceProperty::SUPPORTS_PILOT_JOBS,                         "true"},
                {BatchServiceProperty::SUPPORTS_STANDARD_JOBS,                      "true"},
                {BatchServiceProperty::THREAD_STARTUP_OVERHEAD,                     "0"},
                {BatchServiceProperty::HOST_SELECTION_ALGORITHM,                    "FIRSTFIT"},
                {BatchServiceProperty::TASK_SELECTION_ALGORITHM,                    "maximum_flops"},
        #ifdef ENABLE_BATSCHED
                {BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "easy_bf"},
                {BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM,              "fcfs"},
        #else
                {BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM,            "FCFS"},
        #endif
                {BatchServiceProperty::BATCH_RJMS_DELAY,                            "0"},
                {BatchServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE,               ""},
                {BatchServiceProperty::OUTPUT_CSV_JOB_LOG,                          ""},
                {BatchServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP,           "false"},
                {BatchServiceProperty::BATSCHED_LOGGING_MUTED,                  "true"},
                {BatchServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION,          "false"}
        };

        std::map<std::string, std::string> default_messagepayload_values = {
                {BatchServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                 "1024"},
                {BatchServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD,"1024"},
                {BatchServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, "1024"},
                {BatchServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,              "1024"},
                {BatchServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,           "1024"},
                {BatchServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                {BatchServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                {BatchServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  "1024"},
                {BatchServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                {BatchServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                {BatchServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                {BatchServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,         "1024"},
                {BatchServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,           "1024"},
                {BatchServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,           "1024"},
                {BatchServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                {BatchServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
        };

    public:
        BatchService(std::string &hostname,
                     std::vector<std::string> compute_hosts,
                     double scratch_space_size,
                     std::map<std::string, std::string> property_list = {},
                     std::map<std::string, std::string> messagepayload_list = {}
        );

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/
        std::map<std::string,double> getStartTimeEstimates(std::set<std::tuple<std::string,unsigned int,unsigned int, double>>);

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        ~BatchService() override;
        /***********************/
        /** \endcond          **/
        /***********************/


    private:
        friend class WorkloadTraceFileReplayer;

        BatchService(std::string hostname,
                     std::vector<std::string> compute_hosts,
                     unsigned long cores_per_host,
                     double ram_per_host,
                     double scratch_space_size,
                     std::map<std::string, std::string> property_list,
                     std::map<std::string, std::string> messagepayload_list,
                     std::string suffix
        );

        // helper function
        static unsigned long parseUnsignedLongServiceSpecificArgument(std::string key, std::map<std::string, std::string> &args);

        // helper function
        void submitWorkflowJob(WorkflowJob *job, std::map<std::string, std::string> &batch_job_args);

        //submits a standard job
        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> &batch_job_args) override;

        //submits a standard job
        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &batch_job_args) override;

        // helper function
        void terminateWorkflowJob(WorkflowJob *job);

        // terminate a standard job
        void terminateStandardJob(StandardJob *job) override;

        // terminate a pilot job
        void terminatePilotJob(PilotJob *job) override;

        std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> workload_trace;
        std::shared_ptr<WorkloadTraceFileReplayer> workload_trace_replayer;

        bool clean_exit = false;

        //Configuration to create randomness in measurement period initially
        unsigned long random_interval = 10;

        //create alarms for standard jobs
        std::map<std::string,std::shared_ptr<Alarm>> standard_job_alarms;

        //alarms for pilot jobs (only one pilot job alarm)
        std::map<std::string,std::shared_ptr<Alarm>> pilot_job_alarms;


        /* Resources information in Batchservice */
        unsigned long total_num_of_nodes;
        unsigned long num_cores_per_node;
        std::map<std::string, unsigned long> nodes_to_cores_map;
        std::vector<double> timeslots;
        std::map<std::string, unsigned long> available_nodes_to_cores;
        std::map<unsigned long, std::string> host_id_to_names;
        std::vector<std::string> compute_hosts;
        /*End Resources information in Batchservice */

        // Vector of standard job executors
        std::set<std::shared_ptr<StandardJobExecutor>> running_standard_job_executors;

        // Vector of standard job executors (which is cleared periodically)
        std::set<std::shared_ptr<StandardJobExecutor>> finished_standard_job_executors;

        // Master List of batch jobs
        std::set<std::unique_ptr<BatchJob>>  all_jobs;

        //Queue of pending batch jobs
        std::deque<BatchJob *> pending_jobs;

        //A set of running batch jobs
        std::set<BatchJob *> running_jobs;

        // A set of waiting jobs that have been submitted to batsched, but not scheduled
        std::set<BatchJob *> waiting_jobs;


#ifdef ENABLE_BATSCHED

        std::set<std::string> scheduling_algorithms = {"conservative_bf", "crasher", "easy_bf", "easy_bf_fast",
                                                       "easy_bf_plot_liquid_load_horizon",
                                                       "energy_bf", "energy_bf_dicho", "energy_bf_idle_sleeper",
                                                       "energy_bf_monitoring",
                                                       "energy_bf_monitoring_inertial", "energy_bf_subpart_sleeper",
                                                       "energy_watcher", "fcfs_fast", "fast_conservative_bf",
                                                       "filler", "killer", "killer2", "random", "rejecter",
                                                       "sequencer", "sleeper", "submitter", "waiting_time_estimator"
        };

        std::set<std::string> queue_ordering_options = {"fcfs", "lcfs", "desc_bounded_slowdown", "desc_slowdown",
                                                        "asc_size", "desc_size", "asc_walltime", "desc_walltime"

        };
#else
        std::set<std::string> scheduling_algorithms = {"FCFS"
        };

        //Batch queue ordering options
        std::set<std::string> queue_ordering_options = {
        };

#endif


        unsigned long generateUniqueJobID();

        void removeJobFromRunningList(BatchJob *job);

        void freeJobFromJobsList(BatchJob* job);

        int main() override;

        bool processNextMessage();

        void startBackgroundWorkloadProcess();

        void processGetResourceInformation(const std::string &answer_mailbox);

        void processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job);

        void processStandardJobFailure(StandardJobExecutor *executor,
                                       StandardJob *job,
                                       std::shared_ptr<FailureCause> cause);

        void terminateRunningStandardJob(StandardJob *job);

        std::map<std::string, std::tuple<unsigned long, double>> scheduleOnHosts(std::string host_selection_algorithm,
                                                                                 unsigned long, unsigned long, double);

        BatchJob *pickJobForScheduling(std::string);

        //Terminate the batch service (this is usually for pilot jobs when they act as a batch service)
        void cleanup() override;

        // Terminate currently running pilot jobs
        void terminateRunningPilotJobs();

        //Fail the standard jobs
        void failCurrentStandardJobs();

        //Process the pilot job completion
        void processPilotJobCompletion(PilotJob *job);

        //Process standardjob timeout
        void processStandardJobTimeout(StandardJob *job);

        //process standard job termination request
        void processStandardJobTerminationRequest(StandardJob *job, std::string answer_mailbox);

        //process pilot job termination request
        void processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox);

        //Process standardjob timeout
        void processPilotJobTimeout(PilotJob *job);

        //free up resources
        void freeUpResources(std::map<std::string, std::tuple<unsigned long, double>> resources);

        //send call back to the pilot job submitters
        void sendPilotJobExpirationNotification(PilotJob *job);

        //send call back to the standard job submitters
        void sendStandardJobFailureNotification(StandardJob *job, std::string job_id, std::shared_ptr<FailureCause> cause);

        // Try to schedule a job
        bool scheduleOneQueuedJob();

        // process a job submission
        void processJobSubmission(BatchJob *job, std::string answer_mailbox);


        //start a job
        void startJob(std::map<std::string, std::tuple<unsigned long, double>>, WorkflowJob *,
                      BatchJob *, unsigned long, double, unsigned long);



        //vector of network listeners (only useful when ENABLE_BATSCHED == on)

        std::map<std::string,double> getStartTimeEstimatesForFCFS(std::set<std::tuple<std::string,unsigned int,unsigned int, double>>);


        /** BATSCHED-related fields **/
        std::set<std::shared_ptr<BatschedNetworkListener>> network_listeners;
        pid_t pid;
        unsigned long batsched_port;


#ifdef ENABLE_BATSCHED
        friend class BatschedNetworkListener;

        void startBatsched();
        void stopBatsched();

        std::map<std::string,double> getStartTimeEstimatesFromBatsched(std::set<std::tuple<std::string,unsigned int,unsigned int,double>>);

        void startBatschedNetworkListener();

        void notifyJobEventsToBatSched(std::string job_id, std::string status, std::string job_state,
                                       std::string kill_reason, std::string even_type);

        void appendJobInfoToCSVOutputFile(BatchJob *batch_job, std::string status);

        void sendAllQueuedJobsToBatsched();

        //process execute events from batsched
        void processExecuteJobFromBatSched(std::string bat_sched_reply);

#endif // ENABLE_BATSCHED


    };
}


#endif //WRENCH_BATCH_SERVICE_H
