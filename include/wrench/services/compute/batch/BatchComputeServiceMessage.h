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

namespace wrench {
    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a BatchComputeService
     */
    class BatchComputeServiceMessage : public ComputeServiceMessage {
    protected:
        BatchComputeServiceMessage(sg_size_t payload);
    };

    /**
     * @brief A message sent to a BatchComputeService to submit a batch job for execution
     */
    class BatchComputeServiceJobRequestMessage : public BatchComputeServiceMessage {
    public:
        BatchComputeServiceJobRequestMessage(S4U_CommPort *answer_commport, std::shared_ptr<BatchJob> job, sg_size_t payload);

        /** @brief The commport_name to answer to */
        S4U_CommPort *answer_commport;
        /** @brief The batch job */
        std::shared_ptr<BatchJob> job;
    };

    /**
     * @brief A message sent by an alarm when a job goes over its
     *        requested execution time
     */
    class AlarmJobTimeOutMessage : public ServiceMessage {
    public:
        AlarmJobTimeOutMessage(std::shared_ptr<BatchJob> job, sg_size_t payload);
        /** @brief The batch job */
        std::shared_ptr<BatchJob> job;
    };

#if 0
    /**
     * @brief AlarmNotifyBatschedMessage class
     */
    class AlarmNotifyBatschedMessage : public ServiceMessage {
    public:
        AlarmNotifyBatschedMessage(std::string job_id, sg_size_t payload);
        /** @brief the batch job's id */
        std::string job_id;
    };
#endif

#if 0
    /**
     * @brief BatchSimulationBeginsToSchedulerMessage class
     */
    class BatchSimulationBeginsToSchedulerMessage : public BatchComputeServiceMessage {
    public:
        BatchSimulationBeginsToSchedulerMessage(std::string answer_commport, std::string job_args_to_scheduler, sg_size_t payload);

        /** @brief The commport_name to answer to */
        std::string answer_commport;
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
        BatchSchedReadyMessage(std::string answer_commport, sg_size_t payload);

        /** @brief The commport_name to answer to */
        std::string answer_commport;
    };
#endif


    /**
     * @brief A message sent by a BatschedNetworkListener to a Batsched-enabled
     *        BatchComputeService to tell it to start a job execution, passing it the JSON
     *        reply received from Batsched
     */
    class BatchExecuteJobFromBatSchedMessage : public BatchComputeServiceMessage {
    public:
        BatchExecuteJobFromBatSchedMessage(S4U_CommPort *answer_commport, std::string batsched_decision_reply, sg_size_t payload);

        /** @brief The commport_name to answer to */
        S4U_CommPort *answer_commport;

        /** @brief The decision reply from Batsched */
        std::string batsched_decision_reply;
    };

    /**
     * @brief A message send by a BatschedNetworkListener to a Batsched-enabled BatchComputeService
     *        with a job start time estimate obtained from Batsched
     */
    class BatchQueryAnswerMessage : public BatchComputeServiceMessage {
    public:
        BatchQueryAnswerMessage(double estimated_job_start_time, sg_size_t payload);

        /** @brief The estimated job start time */
        double estimated_start_time;
    };

#if 0
//    /**
//     * @brief BatchFakeReplyMessage class
//     */
//    class BatchFakeJobSubmissionReplyMessage : public BatchComputeServiceMessage {
//    public:
//        BatchFakeJobSubmissionReplyMessage(std::string json_data_string, sg_size_t payload);
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
        BatchJobSubmissionToSchedulerMessage(S4U_CommPort *answer_commport, Job* job, std::string job_args_to_scheduler, sg_size_t payload);

        /** @brief The commport_name to answer to */
        S4U_CommPort *answer_commport;
        /** @brief The batch job */
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

}// namespace wrench


#endif//WRENCH_BATCHSERVICEMESSAGE_H
