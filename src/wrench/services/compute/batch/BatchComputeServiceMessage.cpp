/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    BatchComputeServiceMessage::BatchComputeServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("BatchComputeServiceMessage::" + name, payload) {
    }

#if 0
    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param job_args_to_scheduler: the arguments required by batscheduler of batsim
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(std::string answer_mailbox,
                                                                                     std::string job_args_to_scheduler,
                                                                                     double payload)
            : BatchComputeServiceMessage("BATCH_SIMULATION_BEGINS", payload) {
      if (job_args_to_scheduler.empty()) {
        throw std::invalid_argument(
                "BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(): Empty job arguments to scheduler");
      }
      if (answer_mailbox.empty()) {
        throw std::invalid_argument(
                "BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(): Empty answer mailbox");
      }
      this->answer_mailbox = answer_mailbox;
      this->job_args_to_scheduler = job_args_to_scheduler;
    }
#endif

#if 0
    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchSchedReadyMessage::BatchSchedReadyMessage(std::string answer_mailbox, double payload)
            : BatchComputeServiceMessage("BATCH_SCHED_READY", payload) {
      if (answer_mailbox.empty()) {
        throw std::invalid_argument(
                "BatchSchedReadyMessage::BatchSchedReadyMessage(): Empty answer mailbox");
      }
      this->answer_mailbox = answer_mailbox;
    }
#endif

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param batsched_decision_reply: the decision reply from Batsched
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(std::string answer_mailbox,
                                                                           std::string batsched_decision_reply,
                                                                           double payload)
            : BatchComputeServiceMessage("BatchExecuteJobFromBatSchedMessage", payload) {
        if (answer_mailbox.empty()) {
            throw std::invalid_argument(
                    "BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(): Empty answer mailbox");
        }
        if (batsched_decision_reply.empty()) {
            throw std::invalid_argument(
                    "BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(): Empty batsched decision reply");
        }
        this->answer_mailbox = answer_mailbox;
        this->batsched_decision_reply = batsched_decision_reply;
    }

    /**
     * @brief Constructor
     * @param estimated_job_start_time: the estimated job start time
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchQueryAnswerMessage::BatchQueryAnswerMessage(double estimated_job_start_time, double payload)
            : BatchComputeServiceMessage("BatchQueryAnswerMessage", payload) {
        this->estimated_start_time = estimated_job_start_time;
    }

#if 0
    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param job: the batch job
     * @param job_args_to_scheduler: the arguments required by batscheduler of batsim
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(std::string answer_mailbox,
                                                                               Job *job,
                                                                               std::string job_args_to_scheduler,
                                                                               double payload)
            : BatchComputeServiceMessage("BATCH_JOB_SUBMISSION_TO_SCHEDULER", payload) {
      if (job_args_to_scheduler.empty()) {
        throw std::invalid_argument(
                "BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(): Empty job arguments to scheduler");
      }
      if (job == nullptr) {
        throw std::invalid_argument("BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(): invalid job");
      }
      if (answer_mailbox.empty()) {
        throw std::invalid_argument("BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(): invalid answer mailbox");
      }
      this->job_args_to_scheduler = job_args_to_scheduler;
      this->answer_mailbox = answer_mailbox;
      this->job = job;
    }
#endif


#if 0
    /**
     * @brief Constructor
     * @param reply: the replied answer by scheduler
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchJobReplyFromSchedulerMessage::BatchJobReplyFromSchedulerMessage(std::string reply, double payload)
            : BatchComputeServiceMessage("BATCH_JOB_REPLY_FROM_SCHEDULER", payload), reply(reply) {}

#endif

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which the answer should be sent back
     * @param job: the batch job
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchComputeServiceJobRequestMessage::BatchComputeServiceJobRequestMessage(std::string answer_mailbox,
                                                                               std::shared_ptr<BatchJob> job, double payload)
            : BatchComputeServiceMessage("BatchComputeServiceJobRequestMessage", payload) {
        if (job == nullptr) {
            throw std::invalid_argument(
                    "BatchComputeServiceJobRequestMessage::BatchComputeServiceJobRequestMessage(): Invalid arguments");
        }
        if (answer_mailbox.empty()) {
            throw std::invalid_argument(
                    "BatchComputeServiceJobRequestMessage::BatchComputeServiceJobRequestMessage(): Empty answer mailbox");
        }
        this->job = job;
        this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param job: a batch job
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_arguments
     */
    AlarmJobTimeOutMessage::AlarmJobTimeOutMessage(std::shared_ptr<BatchJob> job, double payload)
            : ServiceMessage("AlarmJobTimeOutMessage", payload) {
        if (job == nullptr) {
            throw std::invalid_argument(
                    "AlarmJobTimeOutMessage::AlarmJobTimeOutMessage: Invalid argument");
        }
        this->job = job;
    }

#if 0
    /**
     * @brief Constructor
     * @param job_id: the id of a batch job
     * @param payload:the message size in bytes
     *
     * @throw std::invalid_arguments
     */
    AlarmNotifyBatschedMessage::AlarmNotifyBatschedMessage(std::string job_id, double payload)
            : ServiceMessage("ALARM_NOTIFY_BATSCHED", payload), job_id(job_id) {}
#endif

}
