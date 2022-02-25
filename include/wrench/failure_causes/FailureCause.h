/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FAILURECAUSE_H
#define WRENCH_FAILURECAUSE_H

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
     * @brief A top-level class to describe all simulation-valid failures that can occur during
     *        workflow execution (and should/could be handled by a WMS)
     *
     */
    class FailureCause {

    public:

        FailureCause() = default;

        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        virtual ~FailureCause() = default;
        /***********************/
        /** \endcond           */
        /***********************/

        /**
         * @brief Return an error message that describes the failure cause (to be overridden)
         *
         * @return an error message
         */
        virtual std::string toString() = 0;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_FAILURECAUSE_H
