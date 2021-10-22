/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_COMPUTE_THREAD_HAS_DIED_H
#define WRENCH_COMPUTE_THREAD_HAS_DIED_H

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
   * @brief A "compute thread has died" failure cause
   */
    class ComputeThreadHasDied : public FailureCause {
    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        ComputeThreadHasDied();
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


#endif //WRENCH_COMPUTE_THREAD_HAS_DIED_H
