/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/action/Action.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/failure_causes/HostError.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>

#include "wrench/failure_causes/OperationTimeout.h"

WRENCH_LOG_CATEGORY(wrench_core_action_executor, "Log category for  Action Executor");


namespace wrench {
    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the action executor will run
     * @param num_cores: the number of cores
     * @param ram_footprint: the RAM footprint
     * @param thread_creation_overhead: the thread creation overhead in seconds
     * @param simulate_computation_as_sleep: whether to simulate computation as sleep
     * @param callback_commport: the callback commport to which an "action done" or "action failed" message will be sent (if nullptr, then no message is sent)
     * @param custom_callback_msg: a custom callback message (if nullptr, an ActionExecutorDoneMessage message wil be sent)
     * @param action: the action to perform
     * @param action_execution_service: the parent action execution service (nullptr if none)
     */
    ActionExecutor::ActionExecutor(
        const std::string& hostname,
        unsigned long num_cores,
        sg_size_t ram_footprint,
        double thread_creation_overhead,
        bool simulate_computation_as_sleep,
        S4U_CommPort* callback_commport,
        SimulationMessage* custom_callback_msg,
        const std::shared_ptr<Action>& action,
        const std::shared_ptr<ActionExecutionService>& action_execution_service) : ExecutionController(
        hostname, "action_executor") {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (action == nullptr) {
            throw std::invalid_argument("ActionExecutor::ActionExecutor(): action cannot be nullptr");
        }
#endif

        this->callback_commport = callback_commport;
        this->custom_callback_message = custom_callback_msg;
        this->action = action;
        this->action_execution_service = action_execution_service;
        this->num_cores = num_cores;
        this->ram_footprint = ram_footprint;
        this->action_startup_overhead = thread_creation_overhead;
        this->simulation_compute_as_sleep = simulate_computation_as_sleep;

        this->killed_on_purpose = false;

        this->action->setNumCoresAllocated(this->num_cores);
        this->action->setRAMAllocated(this->ram_footprint);
        this->action->setExecutionHost(this->hostname);
    }

    /**
     * @brief Return the executor's thread creation overhead
     * @return an overhead (in seconds)
     */
    double ActionExecutor::getThreadCreationOverhead() const {
        return this->action_startup_overhead;
    }

    /**
     * @brief Returns whether computation should be simulated as sleep
     * @return true or false
     */
    bool ActionExecutor::getSimulateComputationAsSleep() const {
        return this->simulation_compute_as_sleep;
    }

    /**
     * @brief Set the action timeout
     * @param timeout A timeout in seconds
     */
    void ActionExecutor::setActionTimeout(const double timeout) {
        this->action_timeout = timeout;
    }


    /**
     * @brief Returns the executor's action
     * @return the action
     */
    std::shared_ptr<Action> ActionExecutor::getAction() {
        return this->action;
    }

    /**
     * @brief Cleanup method that implements the cleanup basics
     * @param has_returned_from_main: true if main has returned
     * @param return_value: main's return value
     */
    void ActionExecutor::cleanup(bool has_returned_from_main, int return_value) {
        WRENCH_DEBUG(
            "In on_exit.cleanup(): ActionExecutor: %s has_returned_from_main = %d (return_value = %d, killed_on_purpose = %d)",
            this->getName().c_str(), has_returned_from_main, return_value,
            this->killed_on_purpose);

        // Handle brutal failure or termination
        if (not has_returned_from_main and this->action->getState() == Action::State::STARTED) {
            this->action->setEndDate(Simulation::getCurrentSimulatedDate());
            if (this->killed_on_purpose) {
                this->action->setState(Action::State::KILLED);
            }
            else {
                this->action->setState(Action::State::FAILED);
                // If no failure cause was set, then it's a host failure
                if (not this->action->getFailureCause()) {
                    this->action->setFailureCause(
                        std::make_shared<HostError>(this->hostname));
                }
            }
        }
    }

    /**
     * @brief Helper method to execute an action
     */
    void ActionExecutor::execute_action() {
        if (this->action_timeout <= 0) {
            this->action->execute(this->getSharedPtr<ActionExecutor>());
        }
        else {
            this->execute_action_with_timeout();
        }
    }

    /**
    * @brief Helper method to execute an action with a timeout
    */
    void ActionExecutor::execute_action_with_timeout() {
        // Create and start an actor to execute the task, while catching a failure if any
        std::shared_ptr<FailureCause> failure_cause = nullptr;
        auto action_executor_with_timeout = std::make_shared<ActionExecutorActorWithTimeout>(
            this->hostname,
            this->num_cores,
            this->ram_footprint,
            this->action_startup_overhead,
            this->simulation_compute_as_sleep,
            this->callback_commport,
            this->custom_callback_message,
            this->action,
            this->action_execution_service,
            this->action_timeout,
            &failure_cause);

        action_executor_with_timeout->setSimulation(this->simulation_);
        action_executor_with_timeout->start(action_executor_with_timeout, true, false);
        auto now = simgrid::s4u::Engine::get_clock();
        action_executor_with_timeout->s4u_actor->join(this->action_timeout);

        // Did we have a timeout?
        auto elapsed = simgrid::s4u::Engine::get_clock() - now;
        if (elapsed >= this->action_timeout) {
            action_executor_with_timeout->kill(false);
            throw ExecutionException(std::make_shared<OperationTimeout>());
        }

        // Did we have a failure?
        if (failure_cause) {
            throw ExecutionException(failure_cause);
        }
    }

    /**
     * @brief Main method of the action executor
     *
     * @return 1 if a failure timestamp should be generated, 0 otherwise
     *
     */
    int ActionExecutor::main() {
        S4U_Simulation::computeZeroFlop(); // to block in case pstate speed is 0

        this->action->setStartDate(S4U_Simulation::getClock());
        this->action->setState(Action::State::STARTED);

        // Overhead
        if (this->action_startup_overhead > 0) {
            WRENCH_INFO("ACTION EXECUTOR: SLEEPING %lf", this->action_startup_overhead);
            S4U_Simulation::sleep(this->action_startup_overhead);
        }

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        // WRENCH_INFO("New Action Executor started to do action %s", this->action->getName().c_str());
        try {
            execute_action();
            this->action->setState(Action::State::COMPLETED);
            this->action->terminate(this->getSharedPtr<ActionExecutor>());
        }
        catch (ExecutionException& e) {
            this->action->setState(Action::State::FAILED);
            this->action->setFailureCause(e.getCause());
        }
        for (auto const& child : this->action->getChildren()) {
            child->updateState();
        }
        this->action->setEndDate(S4U_Simulation::getClock());

        // WRENCH_INFO("Action executor for action %s terminating and action has %s",
        //             this->action->getName().c_str(),
        //             (this->action->getState() == Action::State::COMPLETED ? "succeeded" : ("failed (" + this->action->
        //                 getFailureCause()->toString() + ")").c_str()));
        if (this->callback_commport) {
            try {
                this->callback_commport->putMessage(
                    (this->custom_callback_message)
                        ? this->custom_callback_message
                        : new ActionExecutorDoneMessage(this->getSharedPtr<ActionExecutor>()));
            }
            catch (ExecutionException& e) {
                WRENCH_INFO("Action executor can't report back due to network error.. oh well!");
            }
        }
        return 0;
    }

    /**
    * @brief Kill the worker thread
    *
    * @param job_termination: true if the reason for being killed is that the job was terminated by the submitter
    * (as opposed to being terminated because the above service was also terminated).
    */
    void ActionExecutor::kill(bool job_termination) {
        this->killed_on_purpose = job_termination;
        this->acquireDaemonLock();
        bool i_killed_it = this->killActor();
        this->releaseDaemonLock();
        if (i_killed_it) {
            this->action->terminate(this->getSharedPtr<ActionExecutor>());
        }
    }


    /**
     * @brief Return the action executor's allocated RAM
     * @return a number of bytes
     */
    sg_size_t ActionExecutor::getMemoryAllocated() const {
        return this->ram_footprint;
    }

    /**
     * @brief Return the action executor's allocated number of cores
     * @return a number of cores
     */
    unsigned long ActionExecutor::getNumCoresAllocated() const {
        return this->num_cores;
    }

    /**
     * @brief Get the action execution service that started this action executor (or nullptr if stand-alone action executor)
     * @return an action execution service
     */
    std::shared_ptr<ActionExecutionService> ActionExecutor::getActionExecutionService() const {
        return this->action_execution_service;
    }


    int ActionExecutorActorWithTimeout::main() {
        try {
            this->action->execute(this->getSharedPtr<ActionExecutor>());
        }
        catch (ExecutionException& e) {
            *(this->failure_cause) = e.getCause();
        }
        return 0;
    }
} // namespace wrench
