/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simulation/SimulationMessage.h>
#include <typeinfo>
#include <boost/core/demangle.hpp>

WRENCH_LOG_CATEGORY(wrench_core_simulation_message, "Log category for SimulationMessage");


namespace wrench {


    SimulationMessage::~SimulationMessage() {
        UNTRACK_OBJECT("message");
        //                WRENCH_INFO("DELETE: %s (%p)", this->getName().c_str(), this);
    }


    /**
     * @brief Constructor
     * @param payload: message size in bytes
     */
    SimulationMessage::SimulationMessage(sg_size_t payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (payload < 0) {
            throw std::invalid_argument("SimulationMessage::SimulationMessage(): Invalid arguments");
        }
#endif
        this->payload = std::max<sg_size_t>(0, payload);
        TRACK_OBJECT("message");
    }

    /**
     * @brief Retrieve the message name
     * @return the name
     */
    std::string SimulationMessage::getName() {
        char const *name = typeid(*this).name();
        return boost::core::demangle(name);
    }


}// namespace wrench
