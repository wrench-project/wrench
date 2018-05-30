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

        /** @brief The number of seconds to start a thread (default = 0) **/
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);

        /** @brief The number of bytes in the control message sent by the executor to provide the set of files stored in the scratch **/
        DECLARE_PROPERTY_NAME(STANDARD_JOB_FILES_STORED_IN_SCRATCH);


        /** @brief The algorithm that decides how many cores are given to
         *         a computational task. Possible values are:
         *                  - maximum (default)
         *                  - minimum
         **/
        DECLARE_PROPERTY_NAME(CORE_ALLOCATION_ALGORITHM);

        /** @brief The algorithm that decides which ready computational task,
         *         in case multiple tasks are ready, should run first. Possible values are:
         *                  - maximum_flops (default)
         *                  - maximum_minimum_cores
         **/
        DECLARE_PROPERTY_NAME(TASK_SELECTION_ALGORITHM);

        /** @brief The algorithm that decides on which host a task should
         *         be placed. Possible values are:
         *                  - best_fit (default)
         */
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H
