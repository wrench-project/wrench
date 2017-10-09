/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeServiceProperty.h"

namespace wrench {

    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, NOT_ENOUGH_CORES_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, FLOP_RATE_REQUEST_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, FLOP_RATE_ANSWER_MESSAGE_PAYLOAD);

    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, THREAD_STARTUP_OVERHEAD);

    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, JOB_SELECTION_POLICY);
    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, RESOURCE_ALLOCATION_POLICY);

    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, TASK_SCHEDULING_CORE_ALLOCATION_ALGORITHM);
    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, TASK_SCHEDULING_TASK_SELECTION_ALGORITHM);
    SET_PROPERTY_NAME(MultihostMulticoreComputeServiceProperty, TASK_SCHEDULING_HOST_SELECTION_ALGORITHM);


};

