/**
* Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_REGISTEREDFUNCTION_H
#define WRENCH_REGISTEREDFUNCTION_H

#include <string>
#include <functional>
#include <memory>
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/managers/function_manager/Function.h"

namespace wrench {

    class ServerlessComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Represents a serverless function, encapsulating its metadata and behavior.
     */
    class RegisteredFunction {
    public:
        /**
         * @brief Constructs a RegisteredFunction object.
         * @param function The function to be registered.
         * @param time_limit_in_seconds The time limit for the function execution.
         * @param disk_space_limit_in_bytes The disk space limit for the function.
         * @param RAM_limit_in_bytes The RAM limit for the function.
         * @param ingress_in_bytes The ingress data limit for the function.
         * @param egress_in_bytes The egress data limit for the function.
         */
        RegisteredFunction(const std::shared_ptr<Function> function, 
                           double time_limit_in_seconds, 
                           sg_size_t disk_space_limit_in_bytes, 
                           sg_size_t RAM_limit_in_bytes,
                           sg_size_t ingress_in_bytes, 
                           sg_size_t egress_in_bytes);

        std::shared_ptr<DataFile> getFunctionImage();

    private:
        friend class FunctionManager;
        friend class ServerlessComputeService;
        
        std::shared_ptr<Function> _function; // the function to be registered
        double _time_limit; // the time limit for the function execution
        sg_size_t _disk_space; // the disk space limit for the function
        sg_size_t _ram_limit; // the RAM limit for the function
        sg_size_t _ingress; // the ingress data limit for the function
        sg_size_t _egress; // the egress data limit for the function
    };
    
    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_REGISTEREDFUNCTION_H
