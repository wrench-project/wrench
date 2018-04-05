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

        /** @brief The number of seconds to start a thread **/
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);
        /** @brief The number of bytes in the control message sent by the executor to state that it has completed a job **/
        DECLARE_PROPERTY_NAME(STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
        /** @brief The number of bytes in the control message sent by the executor to state that a job has failed **/
        DECLARE_PROPERTY_NAME(STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);


        /** @brief The algorithm that decides how many cores are given to
         *         a computational task. Possible values are:
         *                  - maximum (default)
         *                  - minimum
         **/
        DECLARE_PROPERTY_NAME(CORE_ALLOCATION_ALGORITHM);

        /** @brief The algorithm that decides which ready computational task,
         *         in case multiple can be ran, should run first. Possible values are:
         *                  - maximum_flops (default)
         *                  - maximum_minimum_cores
         **/
        DECLARE_PROPERTY_NAME(TASK_SELECTION_ALGORITHM);

        /** @brief The algorithm that decides on which host a task should
         *         be placed. Possible values are:
         *                  - best fit (default)
         */
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_MULTINODEMULTICORESTANDARDJOBEXECUTORPROPERTY_H
