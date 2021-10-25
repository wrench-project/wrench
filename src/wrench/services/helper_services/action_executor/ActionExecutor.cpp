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
#include <wrench/failure_causes//HostError.h>

WRENCH_LOG_CATEGORY(wrench_core_action_executor, "Log category for  Action Executor");


namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the action executor will run
     * @param callback_mailbox: the callback mailbox to which a "action done" or "action failed" message will be sent
     * @param action: the action to perform
     */
    ActionExecutor::ActionExecutor(
            std::string hostname,
            std::string callback_mailbox,
            std::shared_ptr <Action> action) :
            Service(hostname, "action_executor", "action_executor") {

        if (action == nullptr) {
            throw std::invalid_argument("ActionExecutor::ActionExecutor(): action cannot be nullptr");
        }

        this->callback_mailbox = callback_mailbox;
        this->action = action;
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
    void ActionExecutor::commonCleanup(bool has_returned_from_main, int return_value) {
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


}
