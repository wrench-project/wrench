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

#include <set>

#include "wrench/execution_controller/ExecutionController.h"
#include "wrench/services/Service.h"
#include "wrench/action/Action.h"

namespace wrench {

    class Simulation;
    class Action;
    class ActionExecutionService;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An service that performs an Action
     */
    class ActionExecutor : public ExecutionController {

    public:

        ActionExecutor(
                std::string hostname,
                unsigned long num_cores,
                double ram_footprint,
                double thread_creation_overhead,
                bool simulate_computation_as_sleep,
                simgrid::s4u::Mailbox *callback_mailbox,
                std::shared_ptr<Action> action,
                std::shared_ptr<ActionExecutionService> action_execution_service);

        int main() override;
        void kill(bool job_termination);
        void cleanup(bool has_returned_from_main, int return_value) override;

        std::shared_ptr<Action> getAction();
        unsigned long getNumCoresAllocated() const;
        double getMemoryAllocated() const;

        std::shared_ptr<ActionExecutionService> getActionExecutionService() const;

        double getThreadCreationOverhead();
        bool getSimulateComputationAsSleep();

    private:

        std::shared_ptr<Action> action;
        std::shared_ptr<ActionExecutionService> action_execution_service;
        simgrid::s4u::Mailbox *callback_mailbox;
        bool killed_on_purpose;

        bool simulation_compute_as_sleep;
        double thread_creation_overhead;

        unsigned long num_cores;
        double ram_footprint;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_ACTION_EXECUTOR_H
