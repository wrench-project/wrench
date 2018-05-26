/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BATCHSERVICEMESSAGE_H
#define WRENCH_BATCHSERVICEMESSAGE_H


#include "wrench/services/compute/ComputeServiceMessage.h"
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
        BatchServiceJobRequestMessage(std::string answer_mailbox, BatchJob* job , double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The batch job */
        BatchJob* job;
    };

    /**
     * @brief A message sent by an alarm when a job goes over its
     *        requested execution time
     */
    class AlarmJobTimeOutMessage : public ServiceMessage {
    public:
        AlarmJobTimeOutMessage(BatchJob* job,double payload);
        /** @brief The batch job */
        BatchJob* job;
    };

    #if 0
    /**
     * @brief AlarmNotifyBatschedMessage class
     */
    class AlarmNotifyBatschedMessage : public ServiceMessage {
    public:
        AlarmNotifyBatschedMessage(std::string job_id, double payload);
        /** @brief the batch job's id */
        std::string job_id;
    };
    #endif


    /**
     * @brief BatchSimulationBeginsToSchedulerMessage class
     */
    class BatchSimulationBeginsToSchedulerMessage : public BatchServiceMessage {
    public:
        BatchSimulationBeginsToSchedulerMessage(std::string answer_mailbox, std::string job_args_to_scheduler, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief JSON data arguments to the scheduler */
        std::string job_args_to_scheduler;
    };

    /**
     * @brief BatchSchedReadyMessage class
     */
    class BatchSchedReadyMessage : public BatchServiceMessage {
    public:
        BatchSchedReadyMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
    };


    /**
     * @brief A message sent by a BatschedNetworkListener to a Batsched-enabled
     *        BatchService to tell it to start a job execution, passing it the JSON
     *        reply received from Batsched
     */
    class BatchExecuteJobFromBatSchedMessage : public BatchServiceMessage {
    public:
        BatchExecuteJobFromBatSchedMessage(std::string answer_mailbox, std::string batsched_decision_reply, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;

        /** @brief The decision reply from Batsched */
        std::string batsched_decision_reply;
    };

    /**
     * @brief BatchQueryAnswerMessage class
     */
    class BatchQueryAnswerMessage : public BatchServiceMessage {
    public:
        BatchQueryAnswerMessage(double estimated_job_start_time, double payload);

        /** @brief The estimated job start time */
        double estimated_start_time;
    };

    #if 0
//    /**
//     * @brief BatchFakeReplyMessage class
//     */
//    class BatchFakeJobSubmissionReplyMessage : public BatchServiceMessage {
//    public:
//        BatchFakeJobSubmissionReplyMessage(std::string json_data_string, double payload);
//
//        /** @brief The resources info in json data string */
//        std::string json_data_string;
//    };
    #endif


    /**
     * @brief BatchJobSubmissionToSchedulerMessage class
     */
    class BatchJobSubmissionToSchedulerMessage : public BatchServiceMessage {
    public:
        BatchJobSubmissionToSchedulerMessage(std::string answer_mailbox, WorkflowJob* job, std::string job_args_to_scheduler, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The batch job */
        WorkflowJob *job;
        /** @brief JSON data arguments to the scheduler */
        std::string job_args_to_scheduler;
    };

    #if 0
    /**
     * @brief BatchJobReplyFromSchedulerMessage class
     */
    class BatchJobReplyFromSchedulerMessage : public BatchServiceMessage {
    public:
        BatchJobReplyFromSchedulerMessage(std::string, double);

        /** @brief The message replied by the scheduler */
        std::string reply;
    };
    #endif

    /***********************/
    /** \endcond           */
    /***********************/

}


#endif //WRENCH_BATCHSERVICEMESSAGE_H
