/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_VIRTUALIZEDCLUSTERPROPERTY_H
#define WRENCH_VIRTUALIZEDCLUSTERPROPERTY_H

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
     * @brief Configurable properties for a VirtualizedClusterService
     */
    class VirtualizedClusterServiceProperty : public ComputeServiceProperty {

    public:
        /** @brief The overhead, in seconds, to boot a VM **/
        DECLARE_PROPERTY_NAME(VM_BOOT_OVERHEAD_IN_SECONDS);
    };

}

#endif //WRENCH_VIRTUALIZEDCLUSTERPROPERTY_H
