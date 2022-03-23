/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/FatalFailure.h>

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_fatal_failure, "Log category for FatalFailure");

namespace wrench {

    /**
    * @brief Constructor
    * @param message: the failure message
    */
    FatalFailure::FatalFailure(std::string message) {
        this->message = message;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FatalFailure::toString() {
        if (this->message.empty()) {
            return std::string("Internal implementation failure, likely a WRENCH bug");
        } else {
            return std::string(this->message + " (internal implementation failure)");
        }
    }

}// namespace wrench
