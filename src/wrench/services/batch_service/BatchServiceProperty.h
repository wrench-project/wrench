//
// Created by suraj on 8/29/17.
//

#ifndef WRENCH_BATCHSERVICEPROPERTY_H
#define WRENCH_BATCHSERVICEPROPERTY_H

#include <services/compute_services/ComputeServiceProperty.h>

namespace wrench {

    class BatchServiceProperty: public ComputeServiceProperty {

    public:
        /** @brief The overhead to start a thread execution, in seconds **/
        DECLARE_PROPERTY_NAME(THREAD_STARTUP_OVERHEAD);
        DECLARE_PROPERTY_NAME(STANDARD_JOB_DONE_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(STANDARD_JOB_FAILED_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(SUBMIT_BATCH_JOB_ANSWER_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(SUBMIT_BATCH_JOB_REQUEST_MESSAGE_PAYLOAD);
        DECLARE_PROPERTY_NAME(HOST_SELECTION_ALGORITHM);
        DECLARE_PROPERTY_NAME(JOB_SELECTION_ALGORITHM);
    };
}


#endif //WRENCH_BATCHSERVICEPROPERTY_H
