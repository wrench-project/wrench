/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef INVOCATION_H
#define INVOCATION_H

#include <memory>
#include <wrench/managers/function_manager/Function.h>
#include <wrench/managers/function_manager/RegisteredFunction.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>

namespace wrench {
   
    /**
     * @class Invocation
     * @brief Represents an invocation of a registered function.
     */
    class Invocation {

    public:
        /**
         * @brief Constructor for Invocation.
         * @param registered_function The registered function to be invoked.
         * @param function_input The input for the function.
         * @param notify_commport The communication port for notifications.
         */
        Invocation(std::shared_ptr<RegisteredFunction> registered_function,
                   std::shared_ptr<FunctionInput> function_input,
                   S4U_CommPort* notify_commport);

        /**
         * @brief Gets the output of the function invocation.
         * @return A shared pointer to the function output.
         */
        // std::shared_ptr<FunctionOutput> get_output();

        /**
         * @brief Checks if the function is currently running.
         * @return True if the function is running, false otherwise.
         */
        bool is_running();

        /**
         * @brief Checks if the function invocation is done.
         * @return True if the function invocation is done, false otherwise.
         */
        bool is_done();

    private:
        friend class FunctionManager;
        friend class ServerlessComputeService;

        std::shared_ptr<RegisteredFunction> _registered_function; ///< The registered function to be invoked.
        std::shared_ptr<FunctionInput> _function_input; ///< The input for the function.
        // std::shared_ptr<FunctionOutput> _function_output; ///< The output of the function invocation.
        S4U_CommPort* _notify_commport; ///< The communication port for notifications.

        bool running = false; ///< Indicates if the function is currently running.
        bool done = false; ///< Indicates if the function invocation is done.
    };
}

#endif //INVOCATION_H
