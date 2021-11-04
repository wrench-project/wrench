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

        /** @brief Whether the ActionExecutionService should terminate if all its hosts are down **/
        DECLARE_PROPERTY_NAME(TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN);

        /** @brief Put back an action in READY state in case its action executor host has crashed  **/
        DECLARE_PROPERTY_NAME(RE_READY_ACTION_AFTER_ACTION_EXECUTOR_CRASH);
    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_ACTION_EXECUTION_SERVICE_PROPERTY_H
