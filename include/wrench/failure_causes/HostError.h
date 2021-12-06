/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HOST_ERROR_H
#define WRENCH_HOST_ERROR_H

#include <set>
#include <string>

#include "wrench/services/Service.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "host error" failure cause (e.g., attempted to start a daemon on a host that is off)
     */
    class HostError : public FailureCause {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        HostError(std::string hostname);

        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString() override;

    private:
        std::string hostname;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_HOST_ERROR_H
