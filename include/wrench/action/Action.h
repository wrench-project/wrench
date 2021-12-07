/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ACTION_H
#define WRENCH_ACTION_H

#include <memory>
#include <string>
#include <set>
#include <stack>

namespace wrench {

    class CompoundJob;
    class FailureCause;
    class ActionExecutor;

    /**
     * @brief An abstract class that implements the concept of an action
     */
    class Action : public std::enable_shared_from_this<Action> {

    public:

        /** @brief Action states */
        enum State {
            /** @brief Not ready (not ready, because depends on other actions) */
            NOT_READY,
            /** @brief Ready (ready to execute) */
            READY,
            /** @brief Started (is being executed) */
            STARTED,
            /** @brief Completed (successfully completed) */
            COMPLETED,
            /** @brief Killed (due to user actions, service being terminated, etc.) */
            KILLED,
            /** @brief Failed (has failed) */
            FAILED
        };

        const std::string &getName() const;
        std::shared_ptr<CompoundJob> getJob() const;
        Action::State getState() const;
        std::string getStateAsString() const;
        static std::string stateToString(Action::State state);

        double getStartDate() const;
        double getEndDate() const;
        std::shared_ptr<FailureCause> getFailureCause() const;

        std::set<std::shared_ptr<Action>> &getChildren();

        std::set<std::shared_ptr<Action>> &getParents();

        virtual unsigned long getMinNumCores() const;
        virtual unsigned long getMaxNumCores() const;
        virtual double getMinRAMFootprint() const;

        virtual bool usesScratch() const;

        void setPriority(double priority);
        double getPriority() const;

        void setSimulateComputationAsSleep(bool simulate_computation_as_sleep);
        void setThreadCreationOverhead(double overhead_in_seconds);

        static std::string getActionTypeAsString(const std::shared_ptr<Action>& action);

        /**
         * @brief A data structure that keeps track of an action's execution(s)
         */
        struct ActionExecution {
            /** @brief start date **/
            double start_date = -1.0;
            /** @brief end date **/
            double end_date = -1.0;
            /** @brief final state **/
            Action::State state;
            /** @brief execution host (could be a virtual host)**/
            std::string execution_host;
            /** @brief physical execution host **/
            std::string physical_execution_host;
            /** @brief Number of allocated cores **/
            unsigned long num_cores_allocated = 0;
            /** @brief RAM allocated cores **/
            double ram_allocated = 0;
            /** @brief Failure cause (if applicable) **/
            std::shared_ptr<FailureCause> failure_cause;
        };

        std::stack<Action::ActionExecution> &getExecutionHistory();

        /**
         * @brief Get the shared pointer for this object
         * @return a shared pointer to the object
         */
        std::shared_ptr<Action> getSharedPtr() { return this->shared_from_this(); }

    protected:

        friend class CompoundJob;
        friend class ActionExecutor;
        friend class ActionExecutionService;
        friend class BareMetalComputeService; // this is a bit unfortunate (to call setFailureCause - perhaps go through CompoundJob?)
        friend class BatchComputeService; // this is a bit unfortunate (to call setFailureCause - perhaps go through CompoundJob?)

        void newExecution(Action::State state);

        void setStartDate(double date);
        void setEndDate(double date);
        void setState(Action::State new_state);
        void setExecutionHost(std::string host);
        void setNumCoresAllocated(unsigned long num_cores);
        void setRAMAllocated(double ram);
        void setFailureCause(std::shared_ptr<FailureCause> failure_cause);

        virtual ~Action() = default;
        Action(const std::string& name, const std::string& prefix, std::shared_ptr<CompoundJob> job);

        /**
        * @brief Method to execute the task
        * @param action_executor: the executor that executes this action
        */
        virtual void execute(std::shared_ptr<ActionExecutor> action_executor) = 0;
        /**
         * @brief Method called when the task terminates
         * @param action_executor:  the executor that executes this action
         */
        virtual void terminate(std::shared_ptr<ActionExecutor> action_executor) = 0;

        void updateState();

        /** @brief The thread creation overhead in seconds */
        double thread_creation_overhead;
        /** @brief Whether to simulate the computation as sleep */
        bool simulate_computation_as_sleep;

    private:


        std::set<std::shared_ptr<Action>> parents;
        std::set<std::shared_ptr<Action>> children;

        double priority;

        std::string name;
        std::shared_ptr<CompoundJob> job;

        std::map<std::string, std::string> service_specific_arguments;

        static unsigned long getNewUniqueNumber();

        std::stack<ActionExecution> execution_history;

    };
}

#endif //WRENCH_ACTION_H
