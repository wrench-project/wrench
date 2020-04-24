/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/failure_causes/ServiceIsSuspended.h"
#include "wrench/services/Service.h"

WRENCH_LOG_CATEGORY(wrench_core_service_is_suspended, "Log category for ServiceIsSuspended");

namespace wrench {


    /**
     * @brief Constructor
     * @param service: the service that was suspended
     */
    ServiceIsSuspended::ServiceIsSuspended(std::shared_ptr<Service> service) {
        this->service = service;
    }

    /**
     * @brief Getter
     * @return the service
     */
    std::shared_ptr<Service> ServiceIsSuspended::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string ServiceIsSuspended::toString() {
        return "Service " + this->service->getName() + " on host " + this->service->getHostname() + " is suspended";
    }


};
