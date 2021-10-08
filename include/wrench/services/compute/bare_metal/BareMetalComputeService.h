/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BAREMETALCOMPUTESERVICE_H
#define WRENCH_BAREMETALCOMPUTESERVICE_H


#include <queue>

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "BareMetalComputeServiceProperty.h"
#include "BareMetalComputeServiceMessagePayload.h"
#include "wrench/services/compute/workunit_executor/Workunit.h"
#include "wrench/services/helpers/HostStateChangeDetector.h"



namespace wrench {

    class Simulation;

    class StorageService;

    class FailureCause;

    class Alarm;


    /**
     * @brief A compute service that manages a set of multi-core compute hosts and
     *        provides access to their resources.
     *
     *        One can think of this as a simple service that allows the user to
     *        run tasks and to specify for each task on which host it should run
     *        and with how many cores. If no host is specified, the service will pick
     *        the least loaded host. If no number of cores is specified, the service
     *        will use as many cores as possible.  The service
     *        will make sure that the RAM capacity of a host is not exceeded by possibly
     *        delaying task executions until enough RAM is available.
     */
    class BareMetalComputeService : public ComputeService {

        friend class CloudComputeService;
        friend class BatchComputeService;

    private:

        std::map<std::string, std::string> default_property_values = {
                {BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS,                         "true"},
                {BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,                            "false"},
                {BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD,                          "0.0"},
                {BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN,      "false"},
        };

        std::map<std::string, double> default_messagepayload_values = {
                {BareMetalComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                    1024},
                {BareMetalComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,                 1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,    1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,     1024},
                {BareMetalComputeServiceMessagePayload::JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD,         1024},
                {BareMetalComputeServiceMessagePayload::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD,               1024},
                {BareMetalComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,              1024},
                {BareMetalComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,            1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,       1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,        1024},
                {BareMetalComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,              1024},
                {BareMetalComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,              1024},
                {BareMetalComputeServiceMessagePayload::PILOT_JOB_FAILED_MESSAGE_PAYLOAD,               1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     1024},
                {BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD,     1024},
                {BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,      1024},
                {BareMetalComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD, 1024},
        };

    public:

        // Public Constructor
        BareMetalComputeService(const std::string &hostname,
                                const std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                std::string scratch_space_mount_point,
                                std::map<std::string, std::string> property_list = {},
                                std::map<std::string, double> messagepayload_list = {}
        );

        // Public Constructor
        BareMetalComputeService(const std::string &hostname,
                                const std::vector<std::string> compute_hosts,
                                std::string scratch_space_mount_point,
                                std::map<std::string, std::string> property_list = {},
                                std::map<std::string, double> messagepayload_list = {}
        );

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void submitStandardJob(std::shared_ptr<StandardJob> job, const std::map<std::string, std::string> &service_specific_args) override;

        void submitPilotJob(std::shared_ptr<PilotJob> job, const std::map<std::string, std::string> &service_specific_args) override;

        void terminateStandardJob(std::shared_ptr<StandardJob> job) override;

        void terminatePilotJob(std::shared_ptr<PilotJob> job) override;

        ~BareMetalComputeService();

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class Simulation;

        void validateProperties();

        // Low-level Constructor
        BareMetalComputeService(const std::string &hostname,
                                std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                std::map<std::string, std::string> property_list,
                                std::map<std::string, double> messagepayload_list,
                                double ttl,
                                std::shared_ptr<PilotJob> pj, std::string suffix,
                                std::shared_ptr<StorageService> scratch_space); // reference to upper level scratch space

        // Private Constructor
        BareMetalComputeService(const std::string &hostname,
                                std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                std::map<std::string, std::string> property_list,
                                std::map<std::string, double> messagepayload_list,
                                std::shared_ptr<StorageService> scratch_space);

        // Low-level constructor helper method
        void initiateInstance(const std::string &hostname,
                              std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                              std::map<std::string, std::string> property_list,
                              std::map<std::string, double> messagepayload_list,
                              double ttl,
                              std::shared_ptr<PilotJob> pj);


        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;

        // Core availabilities (for each hosts, how many cores and how many bytes of RAM are currently available on it)
        std::map<std::string, double> ram_availabilities;
        std::map<std::string, unsigned long> running_thread_counts;

        unsigned long total_num_cores;

        double ttl;
        bool has_ttl;
        double death_date;
        std::shared_ptr<Alarm> death_alarm = nullptr;

        std::shared_ptr<PilotJob> containing_pilot_job; // In case this service is in fact a pilot job

        std::map<std::shared_ptr<StandardJob> , std::set<WorkflowFile*>> files_in_scratch;

        // Set of running jobs
        std::set<std::shared_ptr<StandardJob> > running_jobs;

        // Job task execution specs
        std::map<std::shared_ptr<StandardJob> , std::map<WorkflowTask *, std::tuple<std::string, unsigned long>>> job_run_specs;

        // Map of all Workunits
        std::map<std::shared_ptr<StandardJob> , std::set<std::shared_ptr<Workunit>>> all_workunits;

        std::deque<std::shared_ptr<Workunit>> ready_workunits;
//        std::map<std::shared_ptr<StandardJob> , std::set<Workunit *>> running_workunits;
        std::map<std::shared_ptr<StandardJob> , std::set<std::shared_ptr<Workunit>>> completed_workunits;

        // Set of running WorkunitExecutors
        std::map<std::shared_ptr<StandardJob> , std::set<std::shared_ptr<WorkunitExecutor>>> workunit_executors;

        // Add the scratch files of one standardjob to the list of all the scratch files of all the standard jobs inside the pilot job
        void storeFilesStoredInScratch(std::set<WorkflowFile*> scratch_files);

        // Cleanup the scratch if I am a pilot job
        void cleanUpScratch();

        int main() override;

        // Helper functions to make main() a bit more palatable

        void terminate(bool notify_pilot_job_submitters);

        void failCurrentStandardJobs();

        void processWorkunitExecutorCompletion(std::shared_ptr<WorkunitExecutor> workunit_executor, std::shared_ptr<Workunit> workunit);

        void processWorkunitExecutorFailure(std::shared_ptr<WorkunitExecutor> workunit_executor, std::shared_ptr<Workunit> workunit, std::shared_ptr<FailureCause> cause);

        void processWorkunitExecutorCrash(std::shared_ptr<WorkunitExecutor> workunit_executor);

        void forgetWorkunitExecutor(std::shared_ptr<WorkunitExecutor> workunit_executor);

        void processStandardJobTerminationRequest(std::shared_ptr<StandardJob> job, const std::string &answer_mailbox);

        bool processNextMessage();

        void dispatchReadyWorkunits();

//        void someHostIsBackOn(simgrid::s4u::Host const &h);
//        bool host_back_on = false;


        /** @brief Reasons why a standard job could be terminated */
        enum JobTerminationCause {
            /** @brief The WMS intentionally requested, via a JobManager, that a running job is to be terminated */
            TERMINATED,

            /** @brief The compute service was directed to stop, and any running StandardJob will fail */
            COMPUTE_SERVICE_KILLED
        };

        void terminateRunningStandardJob(std::shared_ptr<StandardJob> job, JobTerminationCause termination_cause);

        void failRunningStandardJob(std::shared_ptr<StandardJob> job, std::shared_ptr<FailureCause> cause);

        void processGetResourceInformation(const std::string &answer_mailbox);

        void processSubmitPilotJob(const std::string &answer_mailbox, std::shared_ptr<PilotJob> job, std::map<std::string, std::string> service_specific_args);

        void processSubmitStandardJob(const std::string &answer_mailbox, std::shared_ptr<StandardJob> job,
                                      std::map<std::string, std::string> &service_specific_arguments);

        void processIsThereAtLeastOneHostWithAvailableResources(
                const std::string &answer_mailbox, unsigned long num_cores, double ram);

        std::tuple<std::string, unsigned long> pickAllocation(WorkflowTask *task,
                                                              std::string required_host, unsigned long required_num_cores, double required_ram,
                                                              std::set<std::string> &hosts_to_avoid);

        bool jobCanRun(std::shared_ptr<StandardJob> job, std::map<std::string, std::string> &service_specific_arguments);

        bool isThereAtLeastOneHostWithResources(unsigned long num_cores, double ram);

        void cleanup(bool has_terminated_cleanly, int return_value) override;

        bool areAllComputeResourcesDownWithNoWUERunning();


        int exit_code = 0;

        std::shared_ptr<HostStateChangeDetector> host_state_change_monitor;

    };
};


#endif //WRENCH_BAREMETALCOMPUTESERVICE_H
