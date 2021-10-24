/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/helper_services/action_executor/ActionExecutorMessage.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     */
    ActionExecutorMessage::ActionExecutorMessage(std::string name) :
            SimulationMessage("ActionExecutorMessage::" + name, 0) {
    }

    /**
     * @brief Constructor
     *
     * @param action_executor: The Action Executor
     */
    ActionExecutorDoneMessage::ActionExecutorDoneMessage(std::shared_ptr<ActionExecutor> action_executor) :
            ActionExecutorMessage("ActionExecutorDoneMessage") {
        this->action_executor = std::move(action_executor);
    }


}
