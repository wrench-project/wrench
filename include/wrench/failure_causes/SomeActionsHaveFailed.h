/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SOME_ACTIONS_HAVE_FAILED_H
#define WRENCH_SOME_ACTIONS_HAVE_FAILED_H

#include <set>
#include <string>

#include "FailureCause.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
    * @brief An "Unknown" failure cause (should not happen)
    */
    class SomeActionsHaveFailed : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        SomeActionsHaveFailed();
        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();

    private:
        std::string message;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_SOME_ACTIONS_HAVE_FAILED_H
