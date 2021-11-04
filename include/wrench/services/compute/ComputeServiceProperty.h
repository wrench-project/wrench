/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPUTESERVICEPROPERTY_H
#define WRENCH_COMPUTESERVICEPROPERTY_H

#include "wrench/services/ServiceProperty.h"

namespace wrench {

    /**
     * @brief Configurable properties for a ComputeService
     */
    class ComputeServiceProperty : public ServiceProperty {
    public:
        /** @brief Whether the compute service supports standard jobs (true or false) **/
        DECLARE_PROPERTY_NAME(SUPPORTS_STANDARD_JOBS);
        /** @brief Whether the compute service supports pilot jobs (true or false) **/
        DECLARE_PROPERTY_NAME(SUPPORTS_PILOT_JOBS);
        /** @brief Whether the compute service supports compound jobs (true or false) **/
        DECLARE_PROPERTY_NAME(SUPPORTS_COMPOUND_JOBS);
    };
};

#endif //WRENCH_COMPUTESERVICEPROPERTY_H
