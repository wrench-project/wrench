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
//        /** @brief The number of bytes in the control message sent by the service to notify a submitter that a job has completed.  **/
//        DECLARE_PROPERTY_NAME(STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
//        /** @brief The number of bytes in the control message sent by the service to notify a submitter that a job has failed.  **/
//        DECLARE_PROPERTY_NAME(STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
//        /** @brief The number of bytes in the control message sent to the service to submit a job.  **/
//        DECLARE_PROPERTY_NAME(SUBMIT_BATCH_JOB_ANSWER_MESSAGE_PAYLOAD);
//        /** @brief The number of bytes in the control message sent by the service in answer to a job submission.  **/
//        DECLARE_PROPERTY_NAME(SUBMIT_BATCH_JOB_REQUEST_MESSAGE_PAYLOAD);
        /** @brief The host selection algorithm. Can be:
         *    - FIRSTFIT
         *    - BESTFIT
         **/
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);
        /** @brief The job selection algorithm. Can be:
         *    - FCFS
         **/
        DECLARE_PROPERTY_NAME(JOB_SELECTION_ALGORITHM);
    };
}


#endif //WRENCH_BATCHSERVICEPROPERTY_H
