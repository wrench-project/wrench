/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/execution_controller/ExecutionControllerMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param payload: the message size in bytes
     */
    ExecutionControllerMessage::ExecutionControllerMessage(double payload) : SimulationMessage(payload) {}


    /**
     * @brief Constructor
     * @param message: the (string) message to be sent
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ExecutionControllerAlarmTimerMessage::ExecutionControllerAlarmTimerMessage(std::string message, double payload) : ExecutionControllerMessage(payload), message(message) {}


};// namespace wrench
