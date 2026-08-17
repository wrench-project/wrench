/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeServiceProperty.h>

namespace wrench {

    SET_PROPERTY_NAME(ServerlessComputeServiceProperty, INVOCATION_PROCESSING_OVERHEAD);
    SET_PROPERTY_NAME(ServerlessComputeServiceProperty, CONTAINER_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(ServerlessComputeServiceProperty, CONTAINER_IDLE_TIMEOUT);
    SET_PROPERTY_NAME(ServerlessComputeServiceProperty, IDLE_CONTAINER_EVICTION_POLICY);
    SET_PROPERTY_NAME(ServerlessComputeServiceProperty, STORAGE_SERVICES_BUFFER_SIZE);
    SET_PROPERTY_NAME(ServerlessComputeServiceProperty, SIMULATE_REMOTE_IMAGE_DOWNLOADS);

}// namespace wrench
