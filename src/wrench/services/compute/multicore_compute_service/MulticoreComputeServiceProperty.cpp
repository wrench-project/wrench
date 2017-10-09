/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/MulticoreComputeServiceProperty.h"

namespace wrench {

    SET_PROPERTY_NAME(MulticoreComputeServiceProperty, NOT_ENOUGH_CORES_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(MulticoreComputeServiceProperty, FLOP_RATE_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(MulticoreComputeServiceProperty, FLOP_RATE_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(MulticoreComputeServiceProperty, THREAD_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(MulticoreComputeServiceProperty, JOB_SELECTION_POLICY);
    SET_PROPERTY_NAME(MulticoreComputeServiceProperty, RESOURCE_ALLOCATION_POLICY);

};

