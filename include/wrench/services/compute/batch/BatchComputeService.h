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
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"

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
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {BatchComputeServiceProperty::THREAD_STARTUP_OVERHEAD, "0"},
                {BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM, "FIRSTFIT"},
                {BatchComputeServiceProperty::TASK_SELECTION_ALGORITHM, "maximum_flops"},
#ifdef ENABLE_BATSCHED
                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                //                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "easy_bf"},
                //                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM,                  "easy_bf_fast"},

                {BatchComputeServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM, "fcfs"},
#else
                {BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"},
                {BatchComputeServiceProperty::BACKFILLING_DEPTH, "infinity"},
#endif
                {BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY, "5"},
                {BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, ""},
                {BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE, "false"},
                {BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE, "false"},
                {BatchComputeServiceProperty::SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE, "-1"},
                {BatchComputeServiceProperty::OUTPUT_CSV_JOB_LOG, ""},
                {BatchComputeServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP, "false"},
                {BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"},
                {BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION, "false"},
                {BatchComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE, "0"}};

        WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE default_messagepayload_values = {
                {BatchComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {BatchComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
        };

    public:
        BatchComputeService(const std::string &hostname,
                            std::vector<std::string> compute_hosts,
                            const std::string& scratch_space_mount_point,
                            WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                            WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;
        bool supportsFunctions() override;

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/
        std::map<std::string, double> getStartTimeEstimates(std::set<std::tuple<std::string, unsigned long, unsigned long, sg_size_t>> set_of_jobs);

        std::vector<std::tuple<std::string, std::string, int, int, int, double, double>> getQueue();

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        ~BatchComputeService() override;
        // helper function
        static unsigned long parseUnsignedLongServiceSpecificArgument(const std::string &key, const std::map<std::string, std::string> &args);

        void validateServiceSpecificArguments(const std::shared_ptr<CompoundJob> &cjob,
                                              std::map<std::string, std::string> &service_specific_args) override;

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        friend class WorkloadTraceFileReplayer;
        friend class HomegrownBatchScheduler;
        friend class FCFSBatchScheduler;
        friend class EasyBackfillingBatchScheduler;
        friend class ConservativeBackfillingBatchScheduler;
        friend class ConservativeBackfillingBatchSchedulerCoreLevel;

        friend class BatschedBatchScheduler;

        BatchComputeService(const std::string &hostname,
                            std::vector<std::string> compute_hosts,
                            unsigned long cores_per_host,
                            sg_size_t ram_per_host,
                            std::string scratch_space_mount_point,
                            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                            WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list,
                            const std::string &suffix);

        //submits a standard job
        void submitCompoundJob(std::shared_ptr<CompoundJob> job, const std::map<std::string, std::string> &batch_job_args) override;

        // terminate a standard job
        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override;

        std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>> workload_trace;
        std::shared_ptr<WorkloadTraceFileReplayer> workload_trace_replayer;

        bool clean_exit = false;

        //create alarms for compound jobs
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<Alarm>> compound_job_alarms;

        /* Resources information in batch */
        unsigned long total_num_of_nodes;
        unsigned long num_cores_per_node;
        std::map<simgrid::s4u::Host *, unsigned long> nodes_to_cores_map;
        std::vector<double> timeslots;
        std::map<simgrid::s4u::Host *, unsigned long> available_nodes_to_cores;
        std::map<unsigned long, simgrid::s4u::Host *> host_id_to_names;
        std::vector<simgrid::s4u::Host *> compute_hosts;

        /* End Resources information in batch */

        // Vector of one-shot bare-metal compute services
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<BareMetalComputeServiceOneShot>> running_bare_metal_one_shot_compute_services;

        // Master List of batch jobs
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<BatchJob>> all_jobs;

        //A set of running batch jobs
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<BatchJob>> running_jobs;

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
                                                       "sequencer", "sleeper", "submitter", "waiting_time_estimator"};

        std::set<std::string> queue_ordering_options = {"fcfs", "lcfs", "desc_bounded_slowdown", "desc_slowdown",
                                                        "asc_size", "desc_size", "asc_walltime", "desc_walltime"};
#else
        std::set<std::string> scheduling_algorithms = {"fcfs", "conservative_bf", "conservative_bf_core_level", "easy_bf_depth0", "easy_bf_depth1"};

        //Batch queue ordering options
        std::set<std::string> queue_ordering_options = {};

#endif

        static unsigned long generateUniqueJobID();

        void removeJobFromRunningList(const std::shared_ptr<BatchJob> &job);

        void removeJobFromBatchQueue(const std::shared_ptr<BatchJob> &job);

        void removeBatchJobFromJobsList(const std::shared_ptr<BatchJob> &job);

        int main() override;

        bool processNextMessage();

        void startBackgroundWorkloadProcess();

        std::map<std::string, double> constructResourceInformation(const std::string &key) override;

        void processGetResourceInformation(S4U_CommPort *answer_commport, const std::string &key);

        void processCompoundJobCompletion(const std::shared_ptr<BareMetalComputeServiceOneShot> &executor, const std::shared_ptr<CompoundJob> &cjob);

        void processCompoundJobFailure(const std::shared_ptr<BareMetalComputeServiceOneShot> &executor,
                                       const std::shared_ptr<CompoundJob> &cjob,
                                       const std::shared_ptr<FailureCause> &cause);

        void terminateRunningCompoundJob(const std::shared_ptr<CompoundJob> &cjob, ComputeService::TerminationCause termination_cause);

        //Terminate the batch service (this is usually for pilot jobs when they act as a batch service)
        void cleanup(bool has_returned_from_main, int return_value) override;

        // Terminate
        void terminate(bool send_failure_notifications, ComputeService::TerminationCause termination_cause);

        //Process standard job timeout
        void processCompoundJobTimeout(const std::shared_ptr<CompoundJob> &job);

        //process standard job termination request
        void processCompoundJobTerminationRequest(const std::shared_ptr<CompoundJob> &job, S4U_CommPort *answer_commport);

        // process a batch bach_job timeout event
        void processAlarmJobTimeout(const std::shared_ptr<BatchJob> &batch_job);

        //free up resources
        void freeUpResources(const std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> &resources);

        //send call back to the pilot job submitters
        void sendPilotJobExpirationNotification(const std::shared_ptr<PilotJob> &job);

        //send call back to the standard job submitters
        void sendCompoundJobFailureNotification(const std::shared_ptr<CompoundJob> &job, const std::string &job_id, const std::shared_ptr<FailureCause> &cause);

        // process a job submission
        void processJobSubmission(const std::shared_ptr<BatchJob> &job, S4U_CommPort *answer_commport);

        //start a job
        void startJob(const std::map<simgrid::s4u::Host *, std::tuple<unsigned long, sg_size_t>> &, const std::shared_ptr<CompoundJob> &,
                      const std::shared_ptr<BatchJob> &, unsigned long, unsigned long, unsigned long);

        // process a resource request
        void processIsThereAtLeastOneHostWithAvailableResources(S4U_CommPort *answer_commport, unsigned long num_cores, sg_size_t ram);

#ifdef ENABLE_BATSCHED
        void processExecuteJobFromBatSched(const std::string &bat_sched_reply);
#endif
    };
}// namespace wrench


#endif//WRENCH_BATCH_SERVICE_H
