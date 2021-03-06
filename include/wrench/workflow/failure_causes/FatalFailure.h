/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FATAL_FAILURE_H
#define WRENCH_FATAL_FAILURE_H

#include <set>
#include <string>

#include "wrench/workflow/failure_causes/FailureCause.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
    * @brief An "Unknown" failure cause (should not happen)
    */
    class FatalFailure : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FatalFailure();
        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();

    private:
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_FATAL_FAILURE_H
