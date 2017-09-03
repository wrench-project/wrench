/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "SimulationMessage.h"
#include "wrench/workflow/WorkflowFile.h"

namespace wrench {


    /**
     * @brief Constructor
     * @param name: message name (a "type" really)
     * @param payload: message size in bytes
     */
    SimulationMessage::SimulationMessage(std::string name, double payload) {
      if ((name == "") || (payload < 0)) {
        throw std::invalid_argument("SimulationMessage::SimulationMessage(): Invalid arguments");
      }
      this->name = name;
      this->payload = payload;
    }

    /**
     * @brief Retrieve the message name
     * @return the name
     */
    std::string SimulationMessage::getName() {
      return this->name;
    }




};
