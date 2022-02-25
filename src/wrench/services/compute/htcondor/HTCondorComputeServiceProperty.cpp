/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/htcondor/HTCondorComputeServiceProperty.h>

namespace wrench {

    SET_PROPERTY_NAME(HTCondorComputeServiceProperty, NEGOTIATOR_OVERHEAD);
    SET_PROPERTY_NAME(HTCondorComputeServiceProperty, GRID_PRE_EXECUTION_DELAY);
    SET_PROPERTY_NAME(HTCondorComputeServiceProperty, GRID_POST_EXECUTION_DELAY);
    SET_PROPERTY_NAME(HTCondorComputeServiceProperty, NON_GRID_PRE_EXECUTION_DELAY);
    SET_PROPERTY_NAME(HTCondorComputeServiceProperty, NON_GRID_POST_EXECUTION_DELAY);

}