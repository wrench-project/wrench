/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/bare_metal/BareMetalComputeServiceProperty.h>

namespace wrench {

    SET_PROPERTY_NAME(BareMetalComputeServiceProperty, TASK_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(BareMetalComputeServiceProperty, FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH);
    SET_PROPERTY_NAME(BareMetalComputeServiceProperty, TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN);

};// namespace wrench
