/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/function_manager/RegisteredFunction.h"

namespace wrench {

    /**
     * @brief Constructs a RegisteredFunction object.
     * @param function The function to be registered.
     * @param time_limit_in_seconds The time limit for the function execution.
     * @param disk_space_limit_in_bytes The disk space limit for the function.
     * @param RAM_limit_in_bytes The RAM limit for the function.
     * @param ingress_in_bytes The ingress data limit for the function.
     * @param egress_in_bytes The egress data limit for the function.
     */
    RegisteredFunction::RegisteredFunction(const std::shared_ptr<Function>& function,
                                           double time_limit_in_seconds, 
                                           sg_size_t disk_space_limit_in_bytes, 
                                           sg_size_t RAM_limit_in_bytes,
                                           sg_size_t ingress_in_bytes, 
                                           sg_size_t egress_in_bytes)
        : _function(function), _time_limit(time_limit_in_seconds), _disk_space(disk_space_limit_in_bytes), 
        _ram_limit(RAM_limit_in_bytes), _ingress(ingress_in_bytes), _egress(egress_in_bytes) {}

    /**
     * @brief Get the authoritative (i.e., in a repo) location of the registered function's image
     * @return A file location
     */
    std::shared_ptr<FileLocation> RegisteredFunction::getOriginalImageLocation() const {
            return _function->getImage();
    }

    /**
     * @brief Get the registered function's image file
     * @return A file
     */
    std::shared_ptr<DataFile> RegisteredFunction::getImageFile() const {
        return _function->getImage()->getFile();
    }

    /**
     * @brief Get the registered function's actual function implementation
     * @return A function
     */
    std::shared_ptr<Function> RegisteredFunction::getFunction() {
        return _function;
    }

    /**
     * @brief Get the registered function's time limit
     * @return A time limit in seconds
     */
    double RegisteredFunction::getTimeLimit() const {
        return _time_limit;
    }

    /**
     * @brief Get the registered function's disk space limit
     * @return A RAN limit in bytes
     */
    sg_size_t RegisteredFunction::getDiskSpaceLimit() const {
        return _disk_space;
    }

    /**
     * @brief Get the registered function's RAM limit
     * @return A RAN limit in bytes
     */
    sg_size_t RegisteredFunction::getRAMLimit() const {
        return _ram_limit;
    }

} // namespace wrench
