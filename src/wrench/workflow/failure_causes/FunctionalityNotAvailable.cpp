/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/failure_causes/FunctionalityNotAvailable.h>
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/Service.h"

WRENCH_LOG_CATEGORY(wrench_core_functionality_not_available, "Log category for FunctionalityNotAvailable");

namespace wrench {


    /**
     * @brief Constructor
     * @param service: the service
     * @param functionality_name: a description of the functionality that's not available
     */
    FunctionalityNotAvailable::FunctionalityNotAvailable(std::shared_ptr<Service> service,
                                                         std::string functionality_name) {
        this->service = service;
        this->functionality_name = std::move(functionality_name);
    }

    /**
     * @brief Getter
     * @return the service
     */
    std::shared_ptr<Service> FunctionalityNotAvailable::getService() {
        return this->service;
    }

    /**
     * @brief Getter
     * @return the functionality name
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



};
