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
                S4U_CommPort *callback_commport,
                std::shared_ptr<Action> action,
                const std::shared_ptr<ActionExecutionService>& action_execution_service);

        int main() override;
        void kill(bool job_termination);
        void cleanup(bool has_returned_from_main, int return_value) override;
        std::shared_ptr<ActionExecutionService> getActionExecutionService() const;
        bool getSimulateComputationAsSleep() const;

    private:
        std::shared_ptr<Action> action;
        std::shared_ptr<ActionExecutionService> action_execution_service;
        S4U_CommPort *callback_commport;
        bool killed_on_purpose;

        bool simulation_compute_as_sleep;
        double thread_creation_overhead;

        unsigned long num_cores;
        sg_size_t ram_footprint;

        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#endif//WRENCH_ACTION_EXECUTOR_H
