/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wms/WMSMessage.h"
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    WMSMessage::WMSMessage(std::string name, double payload) : SimulationMessage("WMSMessage::" + name, payload) {}

    /**
     * @brief Constructor
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    AlarmWMSDeferredStartMessage::AlarmWMSDeferredStartMessage(double payload) : WMSMessage("WMS_START_TIME",
                                                                                            payload) {}


    /**
     * @brief Constructor
     * @param message: the (string) message to be sent
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    AlarmWMSTimerMessage::AlarmWMSTimerMessage(std::string message, double payload) : WMSMessage("WMS_START_TIME",
                                                                                            payload), message(message) {}


};
