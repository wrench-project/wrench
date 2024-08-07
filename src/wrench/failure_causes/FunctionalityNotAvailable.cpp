/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/FunctionalityNotAvailable.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/Service.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_functionality_not_available, "Log category for FunctionalityNotAvailable");

namespace wrench {


    /**
     * @brief Constructor
     * @param service: the service
     * @param functionality_name: a description of the functionality that's not available
     */
    FunctionalityNotAvailable::FunctionalityNotAvailable(std::shared_ptr<Service> service,
                                                         std::string functionality_name) {
        this->service = std::move(service);
        this->functionality_name = std::move(functionality_name);
    }

    /**
     * @brief Get the service on which the functionality was not available
     * @return a service
     */
    std::shared_ptr<Service> FunctionalityNotAvailable::getService() {
        return this->service;
    }

    /**
     * @brief Get the name of the functionality that wasn't available
     * @return a functionality name
     */
    std::string FunctionalityNotAvailable::getFunctionalityName() {
        return this->functionality_name;
    }


    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FunctionalityNotAvailable::toString() {
        return "The request functionality (" + this->functionality_name + ") is not available on service " +
               this->service->getName();
    }


}// namespace wrench
