/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/HostError.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/failure_causes/FailureCause.h>

WRENCH_LOG_CATEGORY(wrench_core_host_error, "Log category for HostError");

namespace wrench {

    /**
     * @brief Constructor
     * @param hostname: the name of the host that experienced the error
     */
    HostError::HostError(std::string hostname) {
        this->hostname = hostname;
    }

    /** @brief Get the human-readable failure message
     * @return the message
     */
    std::string HostError::toString() {
        return "The host (" + this->hostname + ") is down / has crashed";
    }


}
