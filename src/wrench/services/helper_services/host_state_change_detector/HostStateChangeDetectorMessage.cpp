/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.h>

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     *
     */
    HostStateChangeDetectorMessage::HostStateChangeDetectorMessage() : SimulationMessage(0) {
    }


    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host that has turned on
     */
    HostHasTurnedOnMessage::HostHasTurnedOnMessage(const std::string& hostname) : HostStateChangeDetectorMessage() {
        this->hostname = hostname;
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host that has turned off
     */
    HostHasTurnedOffMessage::HostHasTurnedOffMessage(const std::string& hostname) : HostStateChangeDetectorMessage() {
        this->hostname = hostname;
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host that has changed speed
     * @param speed: the host's new speed
     */
    HostHasChangedSpeedMessage::HostHasChangedSpeedMessage(const std::string& hostname, double speed) : HostStateChangeDetectorMessage() {
        this->hostname = std::move(hostname);
        this->speed = speed;
    }

}// namespace wrench
