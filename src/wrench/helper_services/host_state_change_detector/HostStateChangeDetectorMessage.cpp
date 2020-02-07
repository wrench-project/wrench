/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/helpers/HostStateChangeDetectorMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     */
    HostStateChangeDetectorMessage::HostStateChangeDetectorMessage(std::string name) :
            SimulationMessage("HostStateChangeDetectorMessage::" + name, 0) {
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host that has turned on
     */
    HostHasTurnedOnMessage::HostHasTurnedOnMessage(std::string hostname) :
            HostStateChangeDetectorMessage("HostHasTurnedOnMessage") {
        this->hostname = hostname;
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host that has turned off
     */
    HostHasTurnedOffMessage::HostHasTurnedOffMessage(std::string hostname) :
            HostStateChangeDetectorMessage("HostHasTurnedOffMessage") {
        this->hostname = hostname;
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host that has changed speed
     * @param speed: the host's new speed
     */
    HostHasChangedSpeedMessage::HostHasChangedSpeedMessage(std::string hostname, double speed) :
            HostStateChangeDetectorMessage("HostHasChangedSpeedMessage") {
        this->hostname = hostname;
        this->speed = speed;
    }

}
