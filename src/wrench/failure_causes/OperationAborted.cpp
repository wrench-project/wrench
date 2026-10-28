/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/OperationAborted.h>

#include <wrench/logging/TerminalOutput.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_operation_aborted, "Log category for OperationAborted");

namespace wrench {

    /**
     * @brief Constructor
     * @param error_message: a custom error message
     */
    OperationAborted::OperationAborted(std::string error_message) {
        this->error_message = std::move(error_message);
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string OperationAborted::toString() {
        return "Operation aborted (" + this->error_message + ")";
    }

}// namespace wrench
