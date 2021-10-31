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
#include "wrench/services/helper_services/standard_job_executor/StandardJobExecutor.h"
#include "wrench/services/helper_services/work_unit_executor/Workunit.h"
#include "wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.h"
#include "wrench/services/helper_services/action_scheduler/ActionSchedulerProperty.h"


namespace wrench {

    class Simulation;
    class FailureCause;
    class Alarm;
    class Action;
    class ActionExecutor;


    /**
     * @brief TODO
     */
    class ActionScheduler : public Service {

    private:

        std::map<std::string, std::string> default_property_values = {
                {ActionSchedulerProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN,      "false"},
                {ActionSchedulerProperty::RE_READY_ACTION_AFTER_ACTION_EXECUTOR_CRASH,     "false"},
        };

        std::map<std::string, double> default_messagepayload_values = {
        };

    public:

        // Public Constructor
        ActionScheduler(const std::string &hostname,
                        const std::map<std::string, std::tuple<unsigned long, double>> compute_resources,
                        std::shared_ptr<Service> parent_service,
                        double ttl,
                        std::map<std::string, std::string> property_list = {},
                        std::map<std::string, double> messagepayload_list = {}
        );

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        void submitAction(const std::shared_ptr<Action> &action);

        void terminateAction(std::shared_ptr<Action> action);

        ~ActionScheduler();

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class Simulation;

        void validateProperties();


        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;

        // Core availabilities (for each hosts, how many cores and how many bytes of RAM are currently available on it)
        std::map<std::string, double> ram_availabilities;
        std::map<std::string, unsigned long> running_thread_counts;

        unsigned long total_num_cores;

        double ttl;
        bool has_ttl;
        double death_date;
        std::shared_ptr<Alarm> death_alarm = nullptr;

        std::shared_ptr<Service> parent_service;

        std::map<std::shared_ptr<StandardJob> , std::set<WorkflowFile*>> files_in_scratch;

        // Set of running jobs
        std::set<std::shared_ptr<Action> > running_actions;

        // Action execution specs
        std::map<std::shared_ptr<Action> , std::tuple<std::string, unsigned long>> action_run_specs;

        std::set<std::shared_ptr<Action>> all_actions;
        std::deque<std::shared_ptr<Action>> ready_actions;

        // Set of running ActionExecutors
        std::map<std::shared_ptr<Action> , std::shared_ptr<ActionExecutor>> action_executors;

        int main() override;

        // Helper functions to make main() a bit more palatable

        void terminate();

        void failCurrentActions();

        void processActionExecutorCompletion(std::shared_ptr<ActionExecutor> executor);

        void processActionExecutorFailure(std::shared_ptr<ActionExecutor> executor);

        void processActionExecutorCrash(std::shared_ptr<ActionExecutor> executor);

        void processActionTerminationRequest(std::shared_ptr<Action> action, const std::string &answer_mailbox);

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

        void terminateRunningAction(std::shared_ptr<Action> action, bool killed_due_to_job_cancelation);

        void failRunningAction(std::shared_ptr<Action> action, std::shared_ptr<FailureCause> cause);

        std::map<std::string, std::map<std::string, double>> getResourceInformation();

        void processSubmitAction(const std::string &answer_mailbox, std::shared_ptr<Action> action);

        std::tuple<std::string, unsigned long> pickAllocation(std::shared_ptr<Action> action,
                                                              std::string required_host, unsigned long required_num_cores,
                                                              std::set<std::string> &hosts_to_avoid);

        bool actionCanRun(std::shared_ptr<Action> action);

        bool isThereAtLeastOneHostWithResources(unsigned long num_cores, double ram);

        void cleanup(bool has_terminated_cleanly, int return_value) override;

        bool areAllComputeResourcesDownWithNoActionExecutorRunning();


        bool IsThereAtLeastOneHostWithAvailableResources(unsigned long num_cores, double ram);

        int exit_code = 0;

        std::shared_ptr<HostStateChangeDetector> host_state_change_monitor;

    };
};


#endif //WRENCH_ACTION_SCHEDULER_H
