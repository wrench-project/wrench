/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/hadoop/HadoopComputeServiceProperty.h"


namespace wrench {

    SET_PROPERTY_NAME(HadoopComputeServiceProperty, MAP_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(HadoopComputeServiceProperty, REDUCER_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(HadoopComputeServiceProperty, MAP_SIDE_SPILL_PHASE);
    SET_PROPERTY_NAME(HadoopComputeServiceProperty, MAP_SIDE_MERGE_PHASE);
    SET_PROPERTY_NAME(HadoopComputeServiceProperty, HDFS_READ);
    SET_PROPERTY_NAME(HadoopComputeServiceProperty, HDFS_WRITE);

}
