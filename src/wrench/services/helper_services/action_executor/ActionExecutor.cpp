/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/helper_services/action_executor/ActionExecutor.h"

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

}
