/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/NoScratchSpace.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/data_file/DataFile.h>

WRENCH_LOG_CATEGORY(wrench_core_no_scratch_space, "Log category for NoScratchSpace");

namespace wrench {

    /**
     * @brief Constructor
     * @param error: error message
     */
    NoScratchSpace::NoScratchSpace(std::string error) {
        this->error = error;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string NoScratchSpace::toString() {
        return error;
    }

}// namespace wrench
