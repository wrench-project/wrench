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
#include <wrench/data_file/DataFile.h>
#include <typeinfo>
#include <boost/core/demangle.hpp>

WRENCH_LOG_CATEGORY(wrench_core_simulation_message, "Log category for SimulationMessage");


namespace wrench {


    SimulationMessage::~SimulationMessage() {
        //        WRENCH_INFO("DELETE: %s (%lu)", name.c_str(), (unsigned long)(this));
    }


    /**
     * @brief Constructor
     * @param payload: message size in bytes
     */
    SimulationMessage::SimulationMessage(double payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (payload < 0) {
            throw std::invalid_argument("SimulationMessage::SimulationMessage(): Invalid arguments");
        }
#endif
        this->payload = std::max<double>(0, payload);
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
