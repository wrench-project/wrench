/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/serverless/Invocation.h"

namespace wrench {

    /**
     * @brief Constructor for Invocation.
     * @param registered_function The registered function to be invoked.
     * @param function_input The input for the function.
     * @param notify_commport The communication port for notifications.
     */
    Invocation::Invocation(std::shared_ptr<RegisteredFunction> registered_function,
                           std::shared_ptr<FunctionInput> function_input,
                           S4U_CommPort* notify_commport) : _registered_function(registered_function),
                                                            _function_input(function_input),
                                                            _notify_commport(notify_commport)
    {
    }

    /**
     * @brief Gets the output of the function invocation.
     * @return A shared pointer to the function output.
     */
    std::shared_ptr<FunctionOutput> Invocation::get_output() const { return _function_output; }

} // namespace wrench
