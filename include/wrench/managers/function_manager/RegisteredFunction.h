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

    class ComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Represents a serverless function, encapsulating its metadata and behavior.
     */
    class RegisteredFunction {
    public:
        /**
         * @brief Constructs a Function object.
         * @param name The name of the function.
         * @param lambda The function logic implemented as a lambda.
         * @param image The file location of the function's container image.
         * @param code The file location of the function's code.
         */
        RegisteredFunction(const std::shared_ptr<Function> function, double time_limit_in_seconds, sg_size_t disk_space_limit_in_bytes, sg_size_t RAM_limit_in_bytes,
                                                    sg_size_t ingress_in_bytes, sg_size_t egress_in_bytes)
            : _function(function), _time_limit(time_limit_in_seconds), _disk_space(disk_space_limit_in_bytes), 
            _ram_limit(RAM_limit_in_bytes), _ingress(ingress_in_bytes), _egress(egress_in_bytes) {}

        std::shared_ptr<Function> _function;
    private:
        double _time_limit;
        sg_size_t _disk_space;
        sg_size_t _ram_limit;
        sg_size_t _ingress;
        sg_size_t _egress;

    };
    
    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_FUNCTION_H
