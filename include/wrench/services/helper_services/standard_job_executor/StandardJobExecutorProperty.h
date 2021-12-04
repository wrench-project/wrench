/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H
#define WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H


#include "wrench/services/ServiceProperty.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Configurable properties for a StandardJobExecutor
     */
    class StandardJobExecutorProperty {

    public:

        /** @brief The number of seconds to start a task1 (default = 0) **/
        DECLARE_PROPERTY_NAME(TASK_STARTUP_OVERHEAD);

        /** @brief The number of bytes in the control message sent by the executor to provide the set of files stored in the scratch **/
        DECLARE_PROPERTY_NAME(STANDARD_JOB_FILES_STORED_IN_SCRATCH);


        /** @brief The algorithm that decides how many cores are given to
         *         a computational task1. Possible values are:
         *                  - maximum (default)
         *                  - minimum
         **/
        DECLARE_PROPERTY_NAME(CORE_ALLOCATION_ALGORITHM);

        /** @brief The algorithm that decides which ready computational task1,
         *         in case multiple tasks are ready, should run first. Possible values are:
         *                  - maximum_flops (default)
         *                  - maximum_minimum_cores
         *                  - minimum_top_level
         **/
        DECLARE_PROPERTY_NAME(TASK_SELECTION_ALGORITHM);

        /** @brief The algorithm that decides on which host a task1 should
         *         be placed. Possible values are:
         *                  - best_fit (default)
         */
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);

        /** @brief Simulate computation as just a sleep instead of with an actual compute thread. This is for scalability reason,
         *         and only simulation-valid if one is sure that cores are space shared (i.e., only a single compute thread can ever
         *         run on a core at once). Possible values are "true" or "false"
         */
        DECLARE_PROPERTY_NAME(SIMULATE_COMPUTATION_AS_SLEEP);

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H
