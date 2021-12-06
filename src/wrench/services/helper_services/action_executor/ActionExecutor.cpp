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
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>

WRENCH_LOG_CATEGORY(wrench_core_action_executor, "Log category for  Action Executor");


namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the action executor will run
     * @param num_cores: the number of cores
     * @param ram_footprint: the RAM footprint
     * @param callback_mailbox: the callback mailbox to which a "action done" or "action failed" message will be sent
     * @param action: the action to perform
     * @param action_execution_service: the parent action execution service
     */
    ActionExecutor::ActionExecutor(
            std::string hostname,
            unsigned long num_cores,
            double ram_footprint,
            std::string callback_mailbox,
            std::shared_ptr <Action> action,
            std::shared_ptr<ActionExecutionService> action_execution_service) :
            Service(hostname, "action_executor", "action_executor") {

        if (action == nullptr) {
            throw std::invalid_argument("ActionExecutor::ActionExecutor(): action cannot be nullptr");
        }

        this->callback_mailbox = std::move(callback_mailbox);
        this->action = action;
        this->action_execution_service = action_execution_service;
        this->num_cores = num_cores;
        this->ram_footprint = ram_footprint;
        this->killed_on_purpose = false;

        this->action->setNumCoresAllocated(this->num_cores);
        this->action->setRAMAllocated(this->ram_footprint);
        this->action->setExecutionHost(this->hostname);
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
                "In on_exit.cleanup(): ActionExecutor: %s has_returned_from_main = %d (return_value = %d, killed_on_pupose = %d)",
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
                            std::shared_ptr<HostError>(new HostError(this->hostname)));
                }
            }
        }
    }


    /**
     * @brief Main method of the action executor
     *
     * @return 1 if a failure timestamp should be generated, 0 otherwise
     *
     * @throw std::runtime_error
     */
    int ActionExecutor::main() {

        S4U_Simulation::computeZeroFlop(); // to block in case pstate speed is 0

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("New Action Executor started to do action %s", this->action->getName().c_str());

        this->action->setStartDate(S4U_Simulation::getClock());
        this->action->setState(Action::State::STARTED);
        try {
            this->action->execute(this->getSharedPtr<ActionExecutor>());
            this->action->setState(Action::State::COMPLETED);
        } catch (ExecutionException &e) {
            this->action->setState(Action::State::FAILED);
            this->action->setFailureCause(e.getCause());
        }
        for (auto const &child : this->action->children) {
            child->updateState();
        }
        this->action->setEndDate(S4U_Simulation::getClock());

        auto msg_to_send_back = new ActionExecutorDoneMessage(this->getSharedPtr<ActionExecutor>());

        try {
            S4U_Mailbox::putMessage(this->callback_mailbox, msg_to_send_back);
        } catch (std::shared_ptr <NetworkError> &cause) {
            WRENCH_INFO("Action executor can't report back due to network error.. oh well!");
        }
        WRENCH_INFO("Action executor for action %s terminating!", this->action->getName().c_str());

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
    double ActionExecutor::getMemoryAllocated() const {
        return this->ram_footprint;
    }

    /**
     * @brief Return the action executor's allocated nuber of cores
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
}
