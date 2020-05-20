/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HADOOPCOMPUTESERVICEPROPERTY_H
#define WRENCH_HADOOPCOMPUTESERVICEPROPERTY_H


#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
     * @brief Configurable properties for a HadoopComputeService
     */
    class HadoopComputeServiceProperty : public ComputeServiceProperty {
    public:
        /**
         * @brief The overhead to start a map task (placeholder example property to be changed later)
         */
        DECLARE_PROPERTY_NAME(MAP_STARTUP_OVERHEAD);
        DECLARE_PROPERTY_NAME(REDUCER_STARTUP_OVERHEAD);
        DECLARE_PROPERTY_NAME(MAP_SIDE_SPILL_PHASE);
        DECLARE_PROPERTY_NAME(MAP_SIDE_MERGE_PHASE);
        DECLARE_PROPERTY_NAME(HDFS_READ);
        DECLARE_PROPERTY_NAME(HDFS_WRITE);
    };
}

#endif //WRENCH_HADOOPCOMPUTESERVICEPROPERTY_H
