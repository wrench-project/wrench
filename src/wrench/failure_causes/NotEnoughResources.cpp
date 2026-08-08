/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/NotEnoughResources.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/job/Job.h>
#include <wrench/services/Service.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_not_enough_resources, "Log category for NotEnoughResources");

namespace wrench {


    /**
     * @brief Constructor
     * @param message: a message
     */
    NotEnoughResources::NotEnoughResources(std::string message) {
        this->message = message;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NotEnoughResources::toString() {
        std::string text_msg = "Not enough resources: " + this->message;
        return text_msg;
    }

}// namespace wrench
