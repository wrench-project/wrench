/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NOSCRATCHSPACE_H
#define WRENCH_NOSCRATCHSPACE_H

#include <set>
#include <string>

#include "wrench/workflow/failure_causes/FailureCause.h"
#include "wrench/workflow/failure_causes/NoScratchSpace.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "no scratch space" failure cause
     */
    class NoScratchSpace : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        NoScratchSpace(std::string error);
        /***********************/
        /** \endcond           */
        /***********************/

        std::string toString();
    private:
        std::string error;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_NOSCRATCHSPACE_H
