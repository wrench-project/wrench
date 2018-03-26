//
// Created by suraj on 8/29/17.
//

#ifndef WRENCH_BATCHSERVICEPROPERTY_H
#define WRENCH_BATCHSERVICEPROPERTY_H

#include "wrench/services/compute/ComputeServiceProperty.h"

namespace wrench {

    /**
     * @brief Properties for a BatchService
     */
    class BatchServiceProperty: public ComputeServiceProperty {

    public:
        /** @brief The overhead to start a thread execution, in seconds **/
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);

        /** @brief The batch scheduling algorithm. Can be:
         *    - If ENABLE_BATSCHED is set to off / not set:
         *      - "FCFS": First Come First Serve
         *    - If ENABLE_BATSCHED is set to on:
         *      - whatever scheduling algorithm is supported by Batsched
         *                  (by default: "easy_bf")
         *
         **/
        DECLARE_PROPERTY_NAME(BATCH_SCHEDULING_ALGORITHM);

        /** @brief The batch queue ordering algorithm
         *     - If ENABLE_BATSCHED is set to off / not set: ignored
         *     - If ENABLE_BATSCHED is set to on:
         *       - whatever queue ordering algorithm is supported by Batsched
         *                  (by default: "fcfs")
         */
        DECLARE_PROPERTY_NAME(BATCH_QUEUE_ORDERING_ALGORITHM);

        /** @brief The host selection algorithm (only used if
        *         ENABLE_BATSCHED is set of off or not set). Can be:
        *    - FIRSTFIT
        *    - BESTFIT
        **/
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);

        DECLARE_PROPERTY_NAME(SCHEDULER_REPLY_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(BATCH_SCHED_READY_PAYLOAD);
        DECLARE_PROPERTY_NAME(BATCH_EXECUTE_JOB_PAYLOAD);
        DECLARE_PROPERTY_NAME(BATCH_FAKE_JOB_REPLY_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(BATCH_RJMS_DELAY);
        DECLARE_PROPERTY_NAME(SIMULATED_WORKLOAD_TRACE_FILE);
    };
}


#endif //WRENCH_BATCHSERVICEPROPERTY_H
