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
        if (payload < 0) {
            throw std::invalid_argument("SimulationMessage::SimulationMessage(): Invalid arguments");
        }
        this->payload = payload;
    }

    /**
     * @brief Retrieve the message name
     * @return the name
     */
    std::string SimulationMessage::getName() {
        char const * name = typeid( *this).name();
        return boost::core::demangle( name );
    }


};
