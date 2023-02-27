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
        /** @brief The buffer size of the compute service's scratch space (see documentation of StorageServiceProperty::BUFFER_SIZE)
         **/
        DECLARE_PROPERTY_NAME(SCRATCH_SPACE_BUFFER_SIZE);
    };
}// namespace wrench

#endif//WRENCH_COMPUTESERVICEPROPERTY_H
