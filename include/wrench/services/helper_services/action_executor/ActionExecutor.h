/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ACTION_EXECUTOR_H
#define WRENCH_ACTION_EXECUTOR_H

#include "wrench/execution_controller/ExecutionController.h"
#include "wrench/action/Action.h"

namespace wrench {
    class Simulation;
    class Action;
    class ActionExecutionService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A service that performs an Action
     */
    class ActionExecutor : public ExecutionController {
    public:
        unsigned long getNumCoresAllocated() const;
        sg_size_t getMemoryAllocated() const;
        double getThreadCreationOverhead() const;
        std::shared_ptr<Action> getAction();

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        ActionExecutor(
            const std::string& hostname,
            unsigned long num_cores,
            sg_size_t ram_footprint,
            double thread_creation_overhead,
            bool simulate_computation_as_sleep,
            S4U_CommPort* callback_commport,
            SimulationMessage* custom_callback_msg,
            const std::shared_ptr<Action>& action,
            const std::shared_ptr<ActionExecutionService>& action_execution_service);

        int main() override;
        void kill(bool job_termination);
        void cleanup(bool has_returned_from_main, int return_value) override;
        std::shared_ptr<ActionExecutionService> getActionExecutionService() const;
        bool getSimulateComputationAsSleep() const;
        void setActionTimeout(const double timeout);

    protected:
        void execute_action();
        void execute_action_with_timeout();

        std::shared_ptr<Action> action;
        std::shared_ptr<ActionExecutionService> action_execution_service;
        S4U_CommPort* callback_commport;
        SimulationMessage* custom_callback_message;
        bool killed_on_purpose;

        bool simulation_compute_as_sleep;
        double thread_creation_overhead;
        double action_timeout = 0;

        unsigned long num_cores;
        sg_size_t ram_footprint;


        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /* Helper class */
    class ActionExecutorActorWithTimeout : public ActionExecutor {
    public:
        ActionExecutorActorWithTimeout(const std::string& hostname,
                                       unsigned long num_cores,
                                       sg_size_t ram_footprint,
                                       double thread_creation_overhead,
                                       bool simulate_computation_as_sleep,
                                       S4U_CommPort* callback_commport,
                                       SimulationMessage* custom_callback_msg,
                                       const std::shared_ptr<Action>& action,
                                       const std::shared_ptr<ActionExecutionService>& action_execution_service,
                                       double timeout,
                                       std::shared_ptr<FailureCause> *failure_cause) : ActionExecutor(hostname,
                                                                        num_cores,
                                                                        ram_footprint,
                                                                        thread_creation_overhead,
                                                                        simulate_computation_as_sleep,
                                                                        callback_commport,
                                                                        custom_callback_msg,
                                                                        action,
                                                                        action_execution_service) {
            this->timeout = timeout;
            this->failure_cause = failure_cause;
        }

        int main() override;

    private:
        double timeout;
        std::shared_ptr<FailureCause> *failure_cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/
} // namespace wrench

#endif//WRENCH_ACTION_EXECUTOR_H
