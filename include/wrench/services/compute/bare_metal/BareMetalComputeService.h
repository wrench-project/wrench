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
#include "BareMetalComputeServiceProperty.h"
#include "BareMetalComputeServiceMessagePayload.h"
#include "wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.h"


namespace wrench {

    class Simulation;
    class StorageService;
    class FailureCause;
    class Alarm;
    class Action;
    class ActionExecutionService;


    /**
     * @brief A compute service that manages a set of multi-core compute hosts and
     *        provides access to their resources.
     *
     *        One can think of this as a simple service that allows the user to
     *        run jobs and to specify for each job on which host it should run
     *        and with how many cores. If no host is specified, the service will pick
     *        the least loaded host. If no number of cores is specified, the service
     *        will use as many cores as possible.  The service
     *        will make sure that the RAM capacity of a host is not exceeded by possibly
     *        delaying job executions until enough RAM is available. Note that if the submitted
     *        jobs require a total number of cores larger than available, say, on a particular
     *        host, then these jobs will simply time-share the cores. In other words, this
     *        service does not provide space-sharing of hosts/cores
     *        (unlike, for instance, a wrench::BatchComputeService).
     *
     */
    class BareMetalComputeService : public ComputeService {
        friend class CloudComputeService;
        friend class BatchComputeService;

    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {BareMetalComputeServiceProperty::THREAD_STARTUP_OVERHEAD, "0"},
                {BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "true"},
                {BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN, "false"},
        };

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {BareMetalComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::PILOT_JOB_FAILED_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::TERMINATE_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD, 1024},
                {BareMetalComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD, 1024},
        };

    public:
        // Public Constructor
        BareMetalComputeService(const std::string &hostname,
                                const std::map<std::string, std::tuple<unsigned long, double>> &compute_resources,
                                const std::string &scratch_space_mount_point,
                                WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        // Public Constructor
        BareMetalComputeService(const std::string &hostname,
                                const std::vector<std::string> &compute_hosts,
                                const std::string &scratch_space_mount_point,
                                WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void submitCompoundJob(std::shared_ptr<CompoundJob> job, const std::map<std::string, std::string> &service_specific_args) override;

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override;

        ~BareMetalComputeService() override;


    protected:
        friend class JobManager;

        void validateServiceSpecificArguments(const std::shared_ptr<CompoundJob> &job,
                                              std::map<std::string, std::string> &service_specific_args) override;


        BareMetalComputeService(const std::string &hostname,
                                std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                                WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
                                std::shared_ptr<PilotJob> pj, const std::string &suffix,
                                std::shared_ptr<StorageService> scratch_space);// reference to upper level scratch space

        BareMetalComputeService(const std::string &hostname,
                                const std::map<std::string, std::tuple<unsigned long, double>> &compute_resources,
                                WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
                                std::shared_ptr<StorageService> scratch_space);

        void validateProperties();


        // Low-level constructor helper method
        void initiateInstance(const std::string &hostname,
                              std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                              WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                              WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
                              std::shared_ptr<PilotJob> pj);


    protected:
        //        std::shared_ptr<Alarm> death_alarm = nullptr;
        //        std::shared_ptr<PilotJob> containing_pilot_job;// In case this service is in fact a pilot job


        /**
         * @brief Sets of files stored in scratch space
         */
        std::unordered_map<std::shared_ptr<StandardJob>, std::set<std::shared_ptr<DataFile>>> files_in_scratch;

        /**
         * @brief Set of jobs currently handled by the service
         */
        std::set<std::shared_ptr<CompoundJob>> current_jobs;

        /**
         * @brief Set of non-ready actions
         */
        std::set<std::shared_ptr<Action>> not_ready_actions;
        /**
         * @brief Set of ready actions
         */
        std::vector<std::shared_ptr<Action>> ready_actions;
        /**
         * @brief Set of dispatched actions
         */
        std::set<std::shared_ptr<Action>> dispatched_actions;
        /**
         * @brief Map of numbers of per-compound-job dispatched actions
         */
        std::unordered_map<std::shared_ptr<CompoundJob>, int> num_dispatched_actions_for_cjob;

        // Add the scratch files of one standardjob to the list of all the scratch files of all the standard jobs inside the pilot job
        //        void storeFilesStoredInScratch(std::set<std::shared_ptr<DataFile>> scratch_files);

        // Cleanup the scratch if I am a pilot job
        void cleanUpScratch();

        int main() override;

        // Helper functions to make main() a bit more palatable

        void terminate(bool send_failure_notifications, ComputeService::TerminationCause termination_cause);

        void processActionDone(const std::shared_ptr<Action> &action);

        void processCompoundJobTerminationRequest(const std::shared_ptr<CompoundJob> &job, simgrid::s4u::Mailbox *answer_mailbox);

        bool processNextMessage();

        void dispatchReadyActions();


        void terminateCurrentCompoundJob(const std::shared_ptr<CompoundJob> &job, ComputeService::TerminationCause termination_cause);

        void processGetResourceInformation(simgrid::s4u::Mailbox *answer_mailbox, const std::string &key);

        void processSubmitCompoundJob(simgrid::s4u::Mailbox *answer_mailbox, const std::shared_ptr<CompoundJob> &job,
                                      std::map<std::string, std::string> &service_specific_arguments);

        void processIsThereAtLeastOneHostWithAvailableResources(
                simgrid::s4u::Mailbox *answer_mailbox, unsigned long num_cores, double ram);

        void cleanup(bool has_terminated_cleanly, int return_value) override;

        static std::tuple<std::string, unsigned long> parseResourceSpec(const std::string &spec);

        /**
         * @brief This service's exit code
         */
        int exit_code = 0;

        /**
         * @brief The HostStateChangeDetector that is (optionally) started by this service
         */
        std::shared_ptr<HostStateChangeDetector> host_state_change_monitor;

        /**
         * @brief The ActionExecutionService that is started by this service
         */
        std::shared_ptr<ActionExecutionService> action_execution_service;

        /***********************/
        /** \endcond           */
        /***********************/
    };
}// namespace wrench


#endif//WRENCH_BAREMETALCOMPUTESERVICE_H
