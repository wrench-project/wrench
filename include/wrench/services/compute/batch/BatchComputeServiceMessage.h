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
     * @brief Top-level class for messages received/sent by a BatchComputeService
     */
    class BatchComputeServiceMessage : public ComputeServiceMessage {
    protected:
        BatchComputeServiceMessage(std::string name, double payload);
    };

    /**
     * @brief A message sent to a BatchComputeService to submit a batch_standard_and_pilot_jobs job for execution
     */
    class BatchComputeServiceJobRequestMessage : public BatchComputeServiceMessage {
    public:
        BatchComputeServiceJobRequestMessage(std::string answer_mailbox, std::shared_ptr<BatchJob> job , double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The batch_standard_and_pilot_jobs job */
        std::shared_ptr<BatchJob> job;
    };

    /**
     * @brief A message sent by an alarm when a job goes over its
     *        requested execution time
     */
    class AlarmJobTimeOutMessage : public ServiceMessage {
    public:
        AlarmJobTimeOutMessage(std::shared_ptr<BatchJob> job,double payload);
        /** @brief The batch_standard_and_pilot_jobs job */
        std::shared_ptr<BatchJob> job;
    };

    #if 0
    /**
     * @brief AlarmNotifyBatschedMessage class
     */
    class AlarmNotifyBatschedMessage : public ServiceMessage {
    public:
        AlarmNotifyBatschedMessage(std::string job_id, double payload);
        /** @brief the batch_standard_and_pilot_jobs job's id */
        std::string job_id;
    };
    #endif

#if 0
    /**
     * @brief BatchSimulationBeginsToSchedulerMessage class
     */
    class BatchSimulationBeginsToSchedulerMessage : public BatchComputeServiceMessage {
    public:
        BatchSimulationBeginsToSchedulerMessage(std::string answer_mailbox, std::string job_args_to_scheduler, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief JSON data arguments to the scheduler */
        std::string job_args_to_scheduler;
    };
#endif

    #if 0
    /**
     * @brief BatchSchedReadyMessage class
     */
    class BatchSchedReadyMessage : public BatchComputeServiceMessage {
    public:
        BatchSchedReadyMessage(std::string answer_mailbox, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
    };
    #endif


    /**
     * @brief A message sent by a BatschedNetworkListener to a Batsched-enabled
     *        BatchComputeService to tell it to start a job execution, passing it the JSON
     *        reply received from Batsched
     */
    class BatchExecuteJobFromBatSchedMessage : public BatchComputeServiceMessage {
    public:
        BatchExecuteJobFromBatSchedMessage(std::string answer_mailbox, std::string batsched_decision_reply, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;

        /** @brief The decision reply from Batsched */
        std::string batsched_decision_reply;
    };

    /**
     * @brief A message send by a BatschedNetworkListener to a Batsched-enabled BatchComputeService
     *        with a job start time estimate obtained from Batsched
     */
    class BatchQueryAnswerMessage : public BatchComputeServiceMessage {
    public:
        BatchQueryAnswerMessage(double estimated_job_start_time, double payload);

        /** @brief The estimated job start time */
        double estimated_start_time;
    };

    #if 0
//    /**
//     * @brief BatchFakeReplyMessage class
//     */
//    class BatchFakeJobSubmissionReplyMessage : public BatchComputeServiceMessage {
//    public:
//        BatchFakeJobSubmissionReplyMessage(std::string json_data_string, double payload);
//
//        /** @brief The resources info in json data string */
//        std::string json_data_string;
//    };
    #endif


    #if 0
    /**
     * @brief BatchJobSubmissionToSchedulerMessage class
     */
    class BatchJobSubmissionToSchedulerMessage : public BatchComputeServiceMessage {
    public:
        BatchJobSubmissionToSchedulerMessage(std::string answer_mailbox, Job* job, std::string job_args_to_scheduler, double payload);

        /** @brief The mailbox to answer to */
        std::string answer_mailbox;
        /** @brief The batch_standard_and_pilot_jobs job */
        Job *job;
        /** @brief JSON data arguments to the scheduler */
        std::string job_args_to_scheduler;
    };
    #endif

    #if 0
    /**
     * @brief BatchJobReplyFromSchedulerMessage class
     */
    class BatchJobReplyFromSchedulerMessage : public BatchComputeServiceMessage {
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
