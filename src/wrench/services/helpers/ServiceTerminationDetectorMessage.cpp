/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/helpers/ServiceTerminationDetectorMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     */
    ServiceTerminationDetectorMessage::ServiceTerminationDetectorMessage(std::string name) :
            SimulationMessage("ServiceTerminationDetectorMessage::" + name, 0) {
    }


    /**
     * @brief Constructor
     *
     * @param service: the service that has crashed
     */
    ServiceHasCrashedMessage::ServiceHasCrashedMessage(Service *service) :
            ServiceTerminationDetectorMessage("ServiceHasCrashedMessage") {
      this->service = service;
    }

    /**
     * @brief Constructor
     *
     * @param service: the service that has terminated
     */
    ServiceHasTerminatedMessage::ServiceHasTerminatedMessage(Service *service, int return_value) :
            ServiceTerminationDetectorMessage("ServiceHasTerminatedMessage") {
        this->service = service;
        this->return_value = return_value;
    }

}
