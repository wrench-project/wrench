/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/OperationTimeout.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/job/Job.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_operation_timeout, "Log category for OperationTimeout");

namespace wrench {

    /**
    * @brief Constructor    *
    */
    OperationTimeout::OperationTimeout() = default;


    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string OperationTimeout::toString() {
        return {"Operation has timed out"};
    }

}// namespace wrench
