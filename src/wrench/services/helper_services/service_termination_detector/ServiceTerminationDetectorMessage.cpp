/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>

#include <utility>

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
    ServiceHasCrashedMessage::ServiceHasCrashedMessage(std::shared_ptr<Service> service) :
            ServiceTerminationDetectorMessage("ServiceHasCrashedMessage") {
        this->service = std::move(service);
    }

    /**
     * @brief Constructor
     *
     * @param service: the service that has terminated
     * @param exit_code: the service exit_code
     */
    ServiceHasTerminatedMessage::ServiceHasTerminatedMessage(std::shared_ptr<Service> service, int exit_code) :
            ServiceTerminationDetectorMessage("ServiceHasTerminatedMessage") {
        this->service = std::move(service);
        this->exit_code = exit_code;
    }

}
