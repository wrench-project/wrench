/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ACTION_SCHEDULER_PROPERTY_H
#define WRENCH_ACTION_SCHEDULER_PROPERTY_H


#include "wrench/services/ServiceProperty.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Configurable properties for a ActionScheduler
     */
    class ActionSchedulerProperty {

    public:

        /** @brief Whether the ActionScheduler should terminate if all its hosts are down **/
        DECLARE_PROPERTY_NAME(TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN);

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_ACTION_SCHEDULER_PROPERTY_H
