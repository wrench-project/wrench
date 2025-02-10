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
     * @param callback_commport: the callback commport to which a "action done" or "action failed" message will be sent
     * @param action: the action to perform
     * @param action_execution_service: the parent action execution service
     */
    ActionExecutor::ActionExecutor(
            const std::string& hostname,
            unsigned long num_cores,
            sg_size_t ram_footprint,
            double thread_creation_overhead,
            bool simulate_computation_as_sleep,
            S4U_CommPort *callback_commport,
            std::shared_ptr<Action> action,
            std::shared_ptr<ActionExecutionService> action_execution_service) : ExecutionController(hostname, "action_executor") {

#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (action == nullptr) {
            throw std::invalid_argument("ActionExecutor::ActionExecutor(): action cannot be nullptr");
        }
#endif

        this->callback_commport = callback_commport;
        this->action = action;
        this->action_execution_service = action_execution_service;
        this->num_cores = num_cores;
        this->ram_footprint = ram_footprint;
        this->thread_creation_overhead = thread_creation_overhead;
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
        return this->thread_creation_overhead;
    }

    /**
     * @brief Returns whether computation should be simulated as sleep
     * @return true or false
     */
    bool ActionExecutor::getSimulateComputationAsSleep() const {
        return this->simulation_compute_as_sleep;
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
            } else {
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
     * @brief Main method of the action executor
     *
     * @return 1 if a failure timestamp should be generated, 0 otherwise
     *
     */
    int ActionExecutor::main() {
        S4U_Simulation::computeZeroFlop();// to block in case pstate speed is 0

        // If no action, just hang forever until oyu get killed (HACK!)
        //        if (action == nullptr) {
        //            std::cerr << "ACTION EXECUTOR SUSPENDING ITSELF\n";
        //            simgrid::s4u::this_actor::suspend();
        //            return 0;
        //        }

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("New Action Executor started to do action %s", this->action->getName().c_str());
        this->action->setStartDate(S4U_Simulation::getClock());
        this->action->setState(Action::State::STARTED);
        try {
            this->action->execute(this->getSharedPtr<ActionExecutor>());
            this->action->terminate(this->getSharedPtr<ActionExecutor>());
            this->action->setState(Action::State::COMPLETED);
        } catch (ExecutionException &e) {
            this->action->setState(Action::State::FAILED);
            this->action->setFailureCause(e.getCause());
        }
        for (auto const &child: this->action->getChildren()) {
            child->updateState();
        }
        this->action->setEndDate(S4U_Simulation::getClock());

        auto msg_to_send_back = new ActionExecutorDoneMessage(this->getSharedPtr<ActionExecutor>());

        WRENCH_INFO("Action executor for action %s terminating and action has %s",
                    this->action->getName().c_str(),
                    (this->action->getState() == Action::State::COMPLETED ? "succeeded" : ("failed (" + this->action->getFailureCause()->toString() + ")").c_str()));
        try {
            this->callback_commport->putMessage(msg_to_send_back);
        } catch (ExecutionException &e) {
            WRENCH_INFO("Action executor can't report back due to network error.. oh well!");
        }
        return 0;
    }

    /**
    * @brief Kill the worker thread
    *
    * @param job_termination: if the reason for being killed is that the job was terminated by the submitter
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
}// namespace wrench
