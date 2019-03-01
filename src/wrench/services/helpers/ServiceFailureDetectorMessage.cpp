/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/helpers/ServiceFailureDetectorMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     */
    ServiceFailureDetectorMessage::ServiceFailureDetectorMessage(std::string name) :
            SimulationMessage("ServiceFailureDetectorMessage::" + name, 0) {
    }


    /**
     * @brief Constructor
     *
     * @param service: the service that has failed
     */
    ServiceHasCrashedMessage::ServiceHasCrashedMessage(Service *service) :
            ServiceFailureDetectorMessage("ServiceFailureDetectorServiceHasCrashedMessage") {
      this->service = service;
    }

}
