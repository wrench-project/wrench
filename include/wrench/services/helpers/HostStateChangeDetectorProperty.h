/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HOSTSTATECHANGEDETECTORPROPERTY_H
#define WRENCH_HOSTSTATECHANGEDETECTORPROPERTY_H

#include <wrench/services/ServiceProperty.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Configurable properties for a HostStateChangeDetector
     */

    class HostStateChangeDetectorProperty:public ServiceProperty {
    public:

        /** @brief The monitoring period in seconds (default: 1.0) **/
        DECLARE_PROPERTY_NAME(MONITORING_PERIOD);

    };

    /***********************/
    /** \endcond           */
    /***********************/
}


#endif //WRENCH_HOSTSTATECHANGEDETECTORPROPERTY_H
