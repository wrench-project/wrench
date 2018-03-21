//
// Created by suraj on 8/29/17.
//

#include "wrench/services/compute/batch/BatchServiceProperty.h"

namespace wrench {
    SET_PROPERTY_NAME(BatchServiceProperty, THREAD_STARTUP_OVERHEAD);
    SET_PROPERTY_NAME(BatchServiceProperty, HOST_SELECTION_ALGORITHM);
    SET_PROPERTY_NAME(BatchServiceProperty, JOB_SELECTION_ALGORITHM);

    SET_PROPERTY_NAME(BatchServiceProperty, SCHEDULER_REPLY_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(BatchServiceProperty, BATCH_SCHEDULING_ALGORITHM);
    SET_PROPERTY_NAME(BatchServiceProperty, BATCH_QUEUE_ORDERING_ALGORITHM);
    SET_PROPERTY_NAME(BatchServiceProperty, BATCH_SCHED_READY_PAYLOAD);
    SET_PROPERTY_NAME(BatchServiceProperty, BATCH_EXECUTE_JOB_PAYLOAD);
    SET_PROPERTY_NAME(BatchServiceProperty, BATCH_FAKE_JOB_REPLY_MESSAGE_PAYLOAD);
    SET_PROPERTY_NAME(BatchServiceProperty, BATCH_RJMS_DELAY);
    SET_PROPERTY_NAME(BatchServiceProperty, SIMULATED_WORKLOAD_TRACE_FILE);

}