/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/failure_causes/ServiceIsDown.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/ComputeService.h"

WRENCH_LOG_CATEGORY(wrench_core_service_is_down, "Log category for ServiceIsDown");

namespace wrench {

    /**
     * @brief Constructor
     * @param service: the service that was down
     */
    ServiceIsDown::ServiceIsDown(std::shared_ptr<Service> service) {
        this->service = service;
    }

    /**
     * @brief Getter
     * @return the service
     */
    std::shared_ptr<Service> ServiceIsDown::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string ServiceIsDown::toString() {
        return "Service " + this->service->getName() + " on host " + this->service->getHostname() + " was terminated";
    }


}
