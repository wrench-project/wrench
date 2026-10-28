/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/InvalidOperation.h>

#include <wrench/logging/TerminalOutput.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_invalid_operation, "Log category for InvalidOperation");

namespace wrench {

    /**
     * @brief Constructor
     * @param error_message: a custom error message
     */
    InvalidOperation::InvalidOperation(std::string error_message) {
        this->error_message = std::move(error_message);
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string InvalidOperation::toString() {
        return "Invalid operation (" + this->error_message + ")";
    }

}// namespace wrench
