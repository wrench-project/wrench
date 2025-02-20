/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/FunctionNotFound.h>
#include <wrench/managers/function_manager/Function.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/failure_causes/FailureCause.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_function_not_found, "Log category for FunctionNotFound");

namespace wrench {

    /**
     * @brief Constructor
     * @param function: the function to be invoked
     */
    FunctionNotFound::FunctionNotFound(std::shared_ptr<Function> function) {
        _function = std::move(function);
    }

    /** 
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FunctionNotFound::toString() {
        return "The function (" + _function->getName() + ") was not found";
    }

} // namespace wrench