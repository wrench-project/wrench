/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/execution_controller/ExecutionControllerMessage.h>

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param payload: the message size in bytes
     */
    ExecutionControllerMessage::ExecutionControllerMessage(sg_size_t payload) : SimulationMessage(payload) {}

    /**
     * @brief Constructor
     * @param message: the (string) message to be sent
     * @param payload: message size in bytes
     *
     */
    ExecutionControllerAlarmTimerMessage::ExecutionControllerAlarmTimerMessage(std::string message, sg_size_t payload) : ExecutionControllerMessage(payload), message(std::move(message)) {}


}// namespace wrench
