/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceProperty.h>

namespace wrench {

    SET_PROPERTY_NAME(ActionExecutionServiceProperty, THREAD_CREATION_OVERHEAD);
    SET_PROPERTY_NAME(ActionExecutionServiceProperty, SIMULATE_COMPUTATION_AS_SLEEP);
    SET_PROPERTY_NAME(ActionExecutionServiceProperty, TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN);
    SET_PROPERTY_NAME(ActionExecutionServiceProperty, FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH);

}// namespace wrench
