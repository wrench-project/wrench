//
// Created by suraj on 9/16/17.
//

#ifndef WRENCH_BATCHSERVICEMESSAGE_H
#define WRENCH_BATCHSERVICEMESSAGE_H


#include "services/compute/ComputeServiceMessage.h"
#include "BatchJob.h"

namespace wrench{
    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level BatchServiceMessage class
     */
    class BatchServiceMessage : public ComputeServiceMessage {
    protected:
        BatchServiceMessage(std::string name, double payload);
    };

    /**
     * @brief BatchServiceJobRequestMessage class
     */
    class BatchServiceJobRequestMessage : public BatchServiceMessage {
    public:
        BatchServiceJobRequestMessage(std::string answer_mailbox, std::unique_ptr<BatchJob> job , double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The batch job */
        std::unique_ptr<BatchJob> job;
    };

    /**
     * @brief AlarmJobTimeOutMessage class
     */
    class AlarmJobTimeOutMessage : public ServiceMessage {
    public:
        AlarmJobTimeOutMessage(WorkflowJob* job,double payload);
        WorkflowJob* job;
    };



}


#endif //WRENCH_BATCHSERVICEMESSAGE_H
