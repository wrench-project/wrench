/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/batch/BatchComputeServiceProperty.h>

namespace wrench {

    SET_PROPERTY_NAME(BatchComputeServiceProperty, THREAD_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, HOST_SELECTION_ALGORITHM);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, TASK_SELECTION_ALGORITHM);

    SET_PROPERTY_NAME(BatchComputeServiceProperty, BATCH_SCHEDULING_ALGORITHM);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, BATCH_QUEUE_ORDERING_ALGORITHM);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, BATCH_RJMS_PADDING_DELAY);

    SET_PROPERTY_NAME(BatchComputeServiceProperty, SIMULATED_WORKLOAD_TRACE_FILE);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE);

    SET_PROPERTY_NAME(BatchComputeServiceProperty, OUTPUT_CSV_JOB_LOG);

    SET_PROPERTY_NAME(BatchComputeServiceProperty, SIMULATE_COMPUTATION_AS_SLEEP);

    SET_PROPERTY_NAME(BatchComputeServiceProperty, BATSCHED_LOGGING_MUTED);
    SET_PROPERTY_NAME(BatchComputeServiceProperty, BATSCHED_CONTIGUOUS_ALLOCATION);

}// namespace wrench