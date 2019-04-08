/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_HOSTSTATECHANGEDETECTORMESSAGE_H
#define WRENCH_HOSTSTATECHANGEDETECTORMESSAGE_H


#include <wrench/simulation/SimulationMessage.h>
#include <wrench-dev.h>

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a HostStateChangeDetector
     */
    class HostStateChangeDetectorMessage : public SimulationMessage {
    protected:
        explicit HostStateChangeDetectorMessage(std::string name);
    };

    /**
     * @brief A message sent by the HostStateChangeDetector to notify some listener that a host has turned on 
     */
    class HostHasTurnedOnMessage : public HostStateChangeDetectorMessage {
    public:
        explicit HostHasTurnedOnMessage(std::string hostname);
        std::string hostname;
    };

    /**
     * @brief A message sent by the HostStateChangeDetector to notify some listener that a host has turned off 
     */
    class HostHasTurnedOffMessage : public HostStateChangeDetectorMessage {
    public:
        explicit HostHasTurnedOffMessage(std::string hostname);
        std::string hostname;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_HOSTSTATECHANGEDETECTORMESSAGE_H
