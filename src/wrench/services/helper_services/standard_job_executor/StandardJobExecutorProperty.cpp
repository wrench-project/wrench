/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/services/helper_services/standard_job_executor/StandardJobExecutorProperty.h"

namespace wrench {

    SET_PROPERTY_NAME(StandardJobExecutorProperty, TASK_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(StandardJobExecutorProperty, STANDARD_JOB_FILES_STORED_IN_SCRATCH);

    SET_PROPERTY_NAME(StandardJobExecutorProperty, CORE_ALLOCATION_ALGORITHM);
    SET_PROPERTY_NAME(StandardJobExecutorProperty, TASK_SELECTION_ALGORITHM);
    SET_PROPERTY_NAME(StandardJobExecutorProperty, HOST_SELECTION_ALGORITHM);

    SET_PROPERTY_NAME(StandardJobExecutorProperty, SIMULATE_COMPUTATION_AS_SLEEP);
};
