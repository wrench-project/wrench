/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>

#include <wrench/failure_causes/ComputationHasDied.h>

WRENCH_LOG_CATEGORY(wrench_core_computation_has_died, "Log category for ComputationHasDied");

namespace wrench {

    /** @brief Constructor
     *
     */
    ComputationHasDied::ComputationHasDied() {
    }


    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string ComputationHasDied::toString() {
        return std::string("A computation has died (failed or killed)");
    }

}// namespace wrench
