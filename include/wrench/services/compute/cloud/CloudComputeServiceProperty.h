/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CLOUDSERVICEPROPERTY_H
#define WRENCH_CLOUDSERVICEPROPERTY_H

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
     * @brief Configurable properties for a CloudComputeService
     */
    class CloudComputeServiceProperty : public ComputeServiceProperty {

    public:
        /** @brief The overhead, in seconds, to boot a VM **/
        DECLARE_PROPERTY_NAME(VM_BOOT_OVERHEAD_IN_SECONDS);
        /** @brief The VM resource allocation algorithm by which VMs are started on physical hosts. Possible values are:
         *      - best-fit-ram-first (default): Start VMs on hosts using a best-fit algorithm,
         *        considering first the RAM and then the number of cores
         *      - best-fit-cores-first: Start VMs on hosts using a best-fit algorithm,
         *        considering first the number of cores and then then RAM
         *      - first-fit: a first-fit algorithm based on the order of the physical host list
         *
         **/
        DECLARE_PROPERTY_NAME(VM_RESOURCE_ALLOCATION_ALGORITHM);
    };
}// namespace wrench

#endif//WRENCH_CLOUDSERVICEPROPERTY_H
