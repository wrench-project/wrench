/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTIHOSTMULTICORECOMPUTESERVICE_H
#define WRENCH_MULTIHOSTMULTICORECOMPUTESERVICE_H


#include <queue>

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "MultihostMulticoreComputeServiceProperty.h"
#include "MultihostMulticoreComputeServiceMessagePayload.h"

namespace wrench {

    class Simulation;

    class StorageService;

    class FailureCause;

    class Alarm;


    /**
     * @brief A compute service that manages a set of multi-core compute hosts and
     *        controls access to their resources using standard scheduling strategies.
     */
    class MultihostMulticoreComputeService : public ComputeService {

        friend class VirtualizedClusterService;
        friend class BatchService;

    private:

        std::map<std::string, std::string> default_property_values = {
                {MultihostMulticoreComputeServiceProperty::SUPPORTS_STANDARD_JOBS,                         "true"},
                {MultihostMulticoreComputeServiceProperty::SUPPORTS_PILOT_JOBS,                            "true"},
                {MultihostMulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD,                        "0.0"},
                {MultihostMulticoreComputeServiceProperty::JOB_SELECTION_POLICY,                           "FCFS"},
                {MultihostMulticoreComputeServiceProperty::RESOURCE_ALLOCATION_POLICY,                     "aggressive"},
                {MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM,      "maximum"},
                {MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_TASK_SELECTION_ALGORITHM,       "maximum_flops"},
                {MultihostMulticoreComputeServiceProperty::TASK_SCHEDULING_HOST_SELECTION_ALGORITHM,       "best_fit"},
        };

        std::map<std::string, std::string> default_messagepayload_values = {
                {MultihostMulticoreComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                    "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,                 "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD,         "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD,               "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,              "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,            "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,       "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,        "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,              "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,              "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::PILOT_JOB_FAILED_MESSAGE_PAYLOAD,               "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD,     "1024"},
                {MultihostMulticoreComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,      "1024"},
        };

    public:

        // Public Constructor
        MultihostMulticoreComputeService(const std::string &hostname,
                                         const std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
                                         double scratch_space_size,
                                         std::map<std::string, std::string> property_list = {},
                                         std::map<std::string, std::string> messagepayload_list = {}
        );

        // Public Constructor
        MultihostMulticoreComputeService(const std::string &hostname,
                                         const std::set<std::string> compute_hosts,
                                         double scratch_space_size,
                                         std::map<std::string, std::string> property_list = {},
                                         std::map<std::string, std::string> messagepayload_list = {}
        );


        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_args) override;

        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_args) override;

        void terminateStandardJob(StandardJob *job) override;

        void terminatePilotJob(PilotJob *job) override;

        ~MultihostMulticoreComputeService();

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class Simulation;

        // Low-level Constructor
        MultihostMulticoreComputeService(const std::string &hostname,
                                         std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
                                         std::map<std::string, std::string> property_list,
                                         std::map<std::string, std::string> messagepayload_list,
                                         double ttl,
                                         PilotJob *pj, std::string suffix,
                                         StorageService* scratch_space); // reference to upper level scratch space

        // Private Constructor
        MultihostMulticoreComputeService(const std::string &hostname,
                                         std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
                                         std::map<std::string, std::string> property_list,
                                         std::map<std::string, std::string> messagepayload_list,
                                         StorageService* scratch_space);

        // Low-level constructor helper method
        void initiateInstance(const std::string &hostname,
                              std::set<std::tuple<std::string, unsigned long, double>> compute_resources,
                              std::map<std::string, std::string> property_list,
                              std::map<std::string, std::string> messagepayload_list,
                              double ttl,
                              PilotJob *pj);

        std::set<std::tuple<std::string, unsigned long, double>> compute_resources;

        // Core availabilities (for each hosts, how many cores and how many bytes of RAM are currently available on it)
        std::map<std::string, std::pair<unsigned long, double>> core_and_ram_availabilities;
        unsigned long total_num_cores;

        double ttl;
        bool has_ttl;
        double death_date;
        std::shared_ptr<Alarm> death_alarm = nullptr;

        PilotJob *containing_pilot_job; // In case this service is in fact a pilot job
        std::set<WorkflowFile*> files_in_scratch; // Files stored in scratch, here only used by a pilot job, because for standard jobs, they will be deleted inside standardjobs

        // Set of current standard job executors
        std::set<std::shared_ptr<StandardJobExecutor>> standard_job_executors;
        // Set of completed standard job executors
        std::set<std::shared_ptr<StandardJobExecutor>> completed_job_executors;

        // Set of running jobs
        std::set<WorkflowJob *> running_jobs;

        // Queue of pending jobs (standard or pilot) that haven't begun executing
        std::deque<WorkflowJob *> pending_jobs;

        // Add the scratch files of one standardjob to the list of all the scratch files of all the standard jobs inside the pilot job
        void storeFilesStoredInScratch(std::set<WorkflowFile*> scratch_files);

        // Cleanup the scratch if I am a pilot job
        void cleanUpScratch();

        int main() override;

        // Helper functions to make main() a bit more palatable

        void terminate(bool notify_pilot_job_submitters);

        void terminateAllPilotJobs();

        void failCurrentStandardJobs(std::shared_ptr<FailureCause> cause);

        void processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job);

        void processStandardJobFailure(StandardJobExecutor *executor,
                                       StandardJob *job,
                                       std::shared_ptr<FailureCause> cause);

        void processPilotJobCompletion(PilotJob *job);

        void processStandardJobTerminationRequest(StandardJob *job, const std::string &answer_mailbox);

        void processPilotJobTerminationRequest(PilotJob *job, const std::string &answer_mailbox);

        bool processNextMessage();

        bool dispatchNextPendingJob();

        bool dispatchStandardJob(StandardJob *job);

        bool dispatchPilotJob(PilotJob *job);

        std::set<std::tuple<std::string, unsigned long, double>> computeResourceAllocation(StandardJob *job);

        std::set<std::tuple<std::string, unsigned long, double>> computeResourceAllocationAggressive(StandardJob *job);


//        void createWorkForNewlyDispatchedJob(StandardJob *job);

        void terminateRunningStandardJob(StandardJob *job);

        void terminateRunningPilotJob(PilotJob *job);

        void failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);

        void failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);

        void processGetResourceInformation(const std::string &answer_mailbox);

        void processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                      std::map<std::string, std::string> &service_specific_arguments);

        void processSubmitPilotJob(const std::string &answer_mailbox,  PilotJob *job);
    };
};


#endif //WRENCH_MULTIHOSTMULTICORECOMPUTESERVICE_H
