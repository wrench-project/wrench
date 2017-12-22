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
        /** @brief The host selection algorithm. Can be:
         *    - FIRSTFIT
         *    - BESTFIT
         **/
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);
        /** @brief The job selection algorithm. Can be:
         *    - FCFS
         **/
        DECLARE_PROPERTY_NAME(JOB_SELECTION_ALGORITHM);

        DECLARE_PROPERTY_NAME(SCHEDULER_REPLY_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(BATCH_SCHEDULING_ALGORITHM);
        DECLARE_PROPERTY_NAME(BATCH_QUEUE_ORDERING_ALGORITHM);
        DECLARE_PROPERTY_NAME(BATCH_SCHED_READY_PAYLOAD);
        DECLARE_PROPERTY_NAME(BATCH_EXECUTE_JOB_PAYLOAD);
    };
}


#endif //WRENCH_BATCHSERVICEPROPERTY_H
