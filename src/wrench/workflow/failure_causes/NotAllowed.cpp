/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/failure_causes/NotAllowed.h>

#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(wrench_core_not_allowed, "Log category for NotAllowed");

namespace wrench {

    /**
     * @brief Constructor
     * @param service: the service that cause the error (or nullptr if no known service for the error)
     * @param error_message: a custom error message
     */
    NotAllowed::NotAllowed(std::shared_ptr<Service> service, std::string &error_message) {
        this->service = service;
        this->error_message = error_message;
    }

    /**
     * @brief Get the service that caused the error
     * @return the service
     */
    std::shared_ptr<Service> NotAllowed::getService() {
        return this->service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotAllowed::toString() {
        if (this->service) {
            return "The service (" + this->service->getName() + ") does not allow the operation (" + this->error_message + ")";
        } else {
            return "Operation not allowed (" + this->error_message + ")";
        }
    }


}
