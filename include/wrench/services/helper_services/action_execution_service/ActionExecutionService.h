/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ACTION_SCHEDULER_H
#define WRENCH_ACTION_SCHEDULER_H


#include <queue>

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.h"
#include "wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.h"


namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class Simulation;
    class FailureCause;
    class Alarm;
    class Action;
    class ActionExecutor;


    /**
     * @brief An action execution service that:
     *   - Accepts only ready actions
     *   - Run actions FCFS w.r.t to memory constraints without backfilling
     *   - Will oversubscribe cores in whatever way
     *   - Attempts some load balancing
     *   - Tries to execute actions with as many cores as possible
     */
    class ActionExecutionService : public Service {

    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {ActionExecutionServiceProperty::THREAD_CREATION_OVERHEAD, "0"},
                {ActionExecutionServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP, "false"},
                {ActionExecutionServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN, "false"},
                {ActionExecutionServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "true"},
        };

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024}};

    public:
        // Public Constructor
        ActionExecutionService(const std::string &hostname,
                               const std::map<std::string, std::tuple<unsigned long, double>> &compute_resources,
                               std::shared_ptr<Service> parent_service,
                               WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        bool actionCanRun(const std::shared_ptr<Action> &action);

        std::shared_ptr<Service> getParentService() const;

        void setParentService(std::shared_ptr<Service> parent);

        void submitAction(const std::shared_ptr<Action> &action);

        void terminateAction(std::shared_ptr<Action> action, ComputeService::TerminationCause termination_cause);

        bool IsThereAtLeastOneHostWithAvailableResources(unsigned long num_cores, double ram);

        std::map<std::string, std::tuple<unsigned long, double>> &getComputeResources();

        std::map<std::string, double> getResourceInformation(const std::string &key);

        ~ActionExecutionService();

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        friend class Simulation;

        void validateProperties();


        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;

        // Core availabilities (for each hosts, how many cores and how many bytes of RAM are currently available on it)
        std::unordered_map<std::string, double> ram_availabilities;
        std::unordered_map<std::string, unsigned long> running_thread_counts;

        std::shared_ptr<Service> parent_service = nullptr;

        std::unordered_map<std::shared_ptr<StandardJob>, std::set<std::shared_ptr<DataFile>>> files_in_scratch;

        // Set of running jobs
        std::set<std::shared_ptr<Action>> running_actions;

        // Action execution specs
        std::unordered_map<std::shared_ptr<Action>, std::tuple<std::string, unsigned long>> action_run_specs;

        std::set<std::shared_ptr<Action>> all_actions;
        std::deque<std::shared_ptr<Action>> ready_actions;

        // Set of running ActionExecutors
        std::unordered_map<std::shared_ptr<Action>, std::shared_ptr<ActionExecutor>> action_executors;

        int main() override;

        // Helper functions to make main() a bit more palatable

        void terminate(bool send_failure_notifications, ComputeService::TerminationCause termination_cause);

        void failCurrentActions();

        void processActionExecutorCompletion(const std::shared_ptr<ActionExecutor> &executor);

        void processActionExecutorFailure(const std::shared_ptr<ActionExecutor> &executor);

        void processActionExecutorCrash(const std::shared_ptr<ActionExecutor> &executor);

        void processActionTerminationRequest(const std::shared_ptr<Action> &action, simgrid::s4u::Mailbox *answer_mailbox, ComputeService::TerminationCause termination_cause);

        bool processNextMessage();

        void dispatchReadyActions();

        //        void someHostIsBackOn(simgrid::s4u::Host const &h);
        //        bool host_back_on = false;


        /** @brief Reasons why a standard job could be terminated */
        enum JobTerminationCause {
            /** @brief The WMS intentionally requested, via a JobManager, that a running job is to be terminated */
            TERMINATED,

            /** @brief The compute service was directed to stop, and any running StandardJob will fail */
            COMPUTE_SERVICE_KILLED
        };

        void terminateRunningAction(std::shared_ptr<Action> action, bool killed_due_to_job_cancellation);

        void killAction(const std::shared_ptr<Action> &action, const std::shared_ptr<FailureCause> &cause);


        void processSubmitAction(simgrid::s4u::Mailbox *answer_mailbox, const std::shared_ptr<Action> &action);

        std::tuple<std::string, unsigned long> pickAllocation(const std::shared_ptr<Action> &action,
                                                              const std::string &required_host, unsigned long required_num_cores,
                                                              std::set<std::string> &hosts_to_avoid);


        bool isThereAtLeastOneHostWithResources(unsigned long num_cores, double ram);

        void cleanup(bool has_terminated_cleanly, int return_value) override;

        bool areAllComputeResourcesDownWithNoActionExecutorRunning();


        int exit_code = 0;

        std::shared_ptr<HostStateChangeDetector> host_state_change_monitor;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_ACTION_SCHEDULER_H
