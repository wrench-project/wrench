/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/SomeActionsHaveFailed.h>

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_some_actions_have_failed, "Log category for SomeActionsHaveFailed");

namespace wrench {

    /**
    * @brief Constructor
    *
    */
    SomeActionsHaveFailed::SomeActionsHaveFailed() {
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string SomeActionsHaveFailed::toString() {
        if (this->message.empty()) {
            return std::string("Some actions have failed");
        } else {
            return std::string(this->message + " (some actions have failed))");
        }
    }

}// namespace wrench
