/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/FatalFailure.h>

#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(wrench_core_fatal_failure, "Log category for FatalFailure");

namespace wrench {

    /**
    * @brief Constructor
    *
    */
    FatalFailure::FatalFailure() {
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FatalFailure::toString() {
        return std::string("Internal implementation, likely a WRENCH bug");
    }

}
