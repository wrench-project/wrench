/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ACTION_EXECUTION_SERVICE_PROPERTY_H
#define WRENCH_ACTION_EXECUTION_SERVICE_PROPERTY_H


#include "wrench/services/ServiceProperty.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Configurable properties for a ActionExecutionService
     */
    class ActionExecutionServiceProperty {

    public:
        /** @brief The ActionExecutionService's overhead for creating a thread, in seconds **/
        DECLARE_PROPERTY_NAME(THREAD_CREATION_OVERHEAD);

        /** @brief Whether the ActionExecutionService should simulation computations as sleep **/
        DECLARE_PROPERTY_NAME(SIMULATE_COMPUTATION_AS_SLEEP);

        /** @brief Whether the ActionExecutionService should terminate if all its hosts are down **/
        DECLARE_PROPERTY_NAME(TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN);

        /** @brief If true, fail action after an executor crash, otherwise re-ready it and try again  **/
        DECLARE_PROPERTY_NAME(FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH);
    };

    /***********************/
    /** \endcond           */
    /***********************/

};// namespace wrench


#endif//WRENCH_ACTION_EXECUTION_SERVICE_PROPERTY_H
