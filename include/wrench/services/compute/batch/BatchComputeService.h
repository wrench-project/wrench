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
#include "wrench/services/compute/batch/BatchJob.h"
#include "wrench/services/compute/batch/BatschedNetworkListener.h"
#include "wrench/services/compute/batch/BatchComputeServiceProperty.h"
#include "wrench/services/compute/batch/BatchComputeServiceMessagePayload.h"
#include "wrench/services/helper_services/alarm/Alarm.h"
#include "wrench/job/CompoundJob.h"
#include "wrench/job/Job.h"
#include "wrench/services/compute/batch/batch_schedulers/BatchScheduler.h"

#include <deque>
#include <queue>
#include <set>
#include <tuple>

namespace wrench {

    class WorkloadTraceFileReplayer;
    class BareMetalComputeServiceOneShot;

    /**
     * @brief A batch-scheduled compute service that manages a set of compute hosts and
     *        controls access to their resource via a batch queue.
     *
     *        In the current implementation of
     *        this service, like for many of its real-world counterparts, memory_manager_service
     *        partitioning among jobs onq the same host is not handled.  When multiple jobs share hosts,
     *        which can happen when jobs require only a few cores per host and can thus
     *        be co-located on the same hosts in a non-exclusive fashion,
     *        each job simply runs as if it had access to the
     *        full RAM of each compute host it is scheduled on. The simulation of these
     *        memory_manager_service contended scenarios is thus, for now, not realistic as there is no simulation
     *        of the effects
     *        of memory_manager_service sharing (e.g., swapping).
     */
    class BatchComputeService : public ComputeService {

        /**
         * @brief A Batch Service
         */
    private:

        std::map<std::string, std::string> default_property_values = {
                {BatchComputeServiceProperty::TASK_STARTUP_OVERHEAD,                     "0"},
                {BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM,                    "FIRSTFIT"},
                {BatchComputeServiceProperty::TASK_SELECTION_ALGORITHM,                    "maximum_flops"},
#ifdef ENABLE_BATSCHED
                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "conservative_bf"},
//                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "easy_bf"},
//                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "easy_bf_fast"},

                {BatchComputeServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM,              "fcfs"},
#else
                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM,            "fcfs"},
#endif
                {BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY, "5"},
                {BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE,               ""},
                {BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE,     "false"},
                {BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE,     "false"},
                {BatchComputeServiceProperty::SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE,                          "-1"},
                {BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG,                          ""},
                {BatchComputeServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP,               "false"},
                {BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED,                      "true"},
                {BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION,              "false"}
        };

        std::map<std::string, double> default_messagepayload_values = {
                {BatchComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                 1024},
                {BatchComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD,1024},
                {BatchComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BatchComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,              1024},
                {BatchComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,           1024},
                {BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  1024},
                {BatchComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  1024},
                {BatchComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  1024},
                {BatchComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    1024},
                {BatchComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     1024},
                {BatchComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,         1024},
                {BatchComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,           1024},
                {BatchComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,           1024},
                {BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,  1024},
                {BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD,    1024},
                {BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD,     1024},
                {BatchComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD,              1024},
                {BatchComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD,            1024},
                {BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD,  1024},
                {BatchComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BatchComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD, 1024},
        };

    public:
        BatchComputeService(const std::string &hostname,
                            std::vector<std::string> compute_hosts,
                            std::string scratch_space_mount_point,
                            std::map<std::string, std::string> property_list = {},
                            std::map<std::string, double> messagepayload_list = {}
        );

        bool supportsStandardJobs() override { return true; };
        bool supportsCompoundJobs() override {return true; } ;
        bool supportsPilotJobs() override {return true; };

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/
        std::map<std::string,double> getStartTimeEstimates(std::set<std::tuple<std::string,unsigned long,unsigned long, double>> resources);

        std::vector<std::tuple<std::string, std::string, int, int, int, double, double>> getQueue();

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        ~BatchComputeService() override;
        // helper function
        static unsigned long parseUnsignedLongServiceSpecificArgument(std::string key, const std::map<std::string, std::string> &args);

        void validateServiceSpecificArguments(std::shared_ptr<CompoundJob> compound_job,
                                                                   const std::map<std::string, std::string> &service_specific_args) override;

        /***********************/
        /** \endcond          **/
        /***********************/

        void submitPilotJob(std::shared_ptr<PilotJob> job, const std::map<std::string, std::string> &service_specific_args) override {};

        void terminatePilotJob(std::shared_ptr<PilotJob> job) override {};

    private:
        friend class WorkloadTraceFileReplayer;
        friend class FCFSBatchScheduler;
        friend class CONSERVATIVEBFBatchScheduler;
        friend class CONSERVATIVEBFBatchSchedulerCoreLevel;

        friend class BatschedBatchScheduler;

        BatchComputeService(const std::string hostname,
                            std::vector<std::string> compute_hosts,
                            unsigned long cores_per_host,
                            double ram_per_host,
                            std::string scratch_space_mount_point,
                            std::map<std::string, std::string> property_list,
                            std::map<std::string, double> messagepayload_list,
                            std::string suffix
        );

        // helper function

        //submits a standard job
        void submitCompoundJob(std::shared_ptr<CompoundJob> job, const std::map<std::string, std::string> &batch_job_args) override;


        // helper function
        void terminateWorkflowJob(std::shared_ptr<Job> job);

        // terminate a standard job
        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override;

        std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>> workload_trace;
        std::shared_ptr<WorkloadTraceFileReplayer> workload_trace_replayer;

        bool clean_exit = false;

        //create alarms for compound jobs
        std::map<std::shared_ptr<CompoundJob>,std::shared_ptr<Alarm>> compound_job_alarms;

//        //alarms for pilot jobs (only one pilot job alarm)
//        std::map<std::string,std::shared_ptr<Alarm>> pilot_job_alarms;

        /* Resources information in batch */
        unsigned long total_num_of_nodes;
        unsigned long num_cores_per_node;
        std::map<std::string, unsigned long> nodes_to_cores_map;
        std::vector<double> timeslots;
        std::map<std::string, unsigned long> available_nodes_to_cores;
        std::map<unsigned long, std::string> host_id_to_names;
        std::vector<std::string> compute_hosts;
        /* End Resources information in batch */

        // Vector of one-shot bare-metal compute services
//        std::set<std::shared_ptr<StandardJobExecutor>> running_standard_job_executors;
        std::set<std::shared_ptr<BareMetalComputeServiceOneShot>> running_bare_metal_one_shot_compute_services;

        // Vector of one-shot bare-metal compute services (which is cleared periodically)
        std::set<std::shared_ptr<BareMetalComputeServiceOneShot>> finished_bare_metal_one_shot_compute_services;

        // Master List of batch jobs
        std::set<std::shared_ptr<BatchJob>>  all_jobs;

        //A set of running batch jobs
        std::set<std::shared_ptr<BatchJob>> running_jobs;

        // The batch queue
        std::deque<std::shared_ptr<BatchJob>> batch_queue;

        // A set of "waiting" batch jobs, i.e., jobs that are waiting to be sent to
        //  the  scheduler (useful for batsched only)
        std::set<std::shared_ptr<BatchJob>> waiting_jobs;

        // Scheduler
        std::unique_ptr<BatchScheduler> scheduler;


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
        std::set<std::string> scheduling_algorithms = {"fcfs", "conservative_bf", "conservative_bf_core_level"
        };

        //Batch queue ordering options
        std::set<std::string> queue_ordering_options = {
        };

#endif

        unsigned long generateUniqueJobID();

        void removeJobFromRunningList(std::shared_ptr<BatchJob> job);

        void removeJobFromBatchQueue(std::shared_ptr<BatchJob> job);

        void removeBatchJobFromJobsList(std::shared_ptr<BatchJob> job);

        int main() override;

        bool processNextMessage();

        void startBackgroundWorkloadProcess();

        void processGetResourceInformation(const std::string &answer_mailbox);

        void processCompoundJobCompletion(std::shared_ptr<BareMetalComputeServiceOneShot> executor, std::shared_ptr<CompoundJob> job);

        void processCompoundJobFailure(std::shared_ptr<BareMetalComputeServiceOneShot> executor,
                                       std::shared_ptr<CompoundJob> job,
                                       std::shared_ptr<FailureCause> cause);

        void terminateRunningCompoundJob(std::shared_ptr<CompoundJob> job);


        //Terminate the batch service (this is usually for pilot jobs when they act as a batch service)
        void cleanup(bool has_returned_from_main, int return_value) override;

        // Terminate currently running pilot jobs
        void terminateRunningPilotJobs();

        //Fail the standard jobs
        void failCurrentCompoundJobs();

        //Process the pilot job completion
        void processPilotJobCompletion(std::shared_ptr<PilotJob> job) {}; // TODO: TO REMOVE

        //Process standard job timeout
        void processCompoundJobTimeout(std::shared_ptr<CompoundJob> job);

        //process standard job termination request
        void processCompoundJobTerminationRequest(std::shared_ptr<CompoundJob> job, std::string answer_mailbox);

        //process pilot job termination request
        void processPilotJobTerminationRequest(std::shared_ptr<PilotJob> job, std::string answer_mailbox) {}; // TODO: TO REMOVE

        // process a batch bach_job tiemout event
        void processAlarmJobTimeout(std::shared_ptr<BatchJob>bach_job);

        //Process pilot job timeout
        void processPilotJobTimeout(std::shared_ptr<PilotJob> job);

        //free up resources
        void freeUpResources(std::map<std::string, std::tuple<unsigned long, double>> resources);

        //send call back to the pilot job submitters
        void sendPilotJobExpirationNotification(std::shared_ptr<PilotJob> job);

        //send call back to the standard job submitters
        void sendCompoundJobFailureNotification(std::shared_ptr<CompoundJob> job, std::string job_id, std::shared_ptr<FailureCause> cause);

        // process a job submission
        void processJobSubmission(std::shared_ptr<BatchJob>job, std::string answer_mailbox);

        //start a job
        void startJob(std::map<std::string, std::tuple<unsigned long, double>>, std::shared_ptr<CompoundJob> ,
                      std::shared_ptr<BatchJob>, unsigned long, unsigned long, unsigned long);


        void processExecuteJobFromBatSched(std::string bat_sched_reply);

        void processIsThereAtLeastOneHostWithAvailableResources(const std::string &answer_mailbox, unsigned long num_cores, double ram);

    };
}


#endif //WRENCH_BATCH_SERVICE_H
