/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/batch/BatchServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the message name
     * @param payload: the message size in bytes
     */
    BatchServiceMessage::BatchServiceMessage(std::string name, double payload) :
            ComputeServiceMessage("BatchServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param job_args_to_scheduler: the arguments required by batscheduler of batsim
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(std::string answer_mailbox,
                                                                                     std::string job_args_to_scheduler,
                                                                                     double payload)
            : BatchServiceMessage("BATCH_SIMULATION_BEGINS", payload) {
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

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchSchedReadyMessage::BatchSchedReadyMessage(std::string answer_mailbox, double payload)
            : BatchServiceMessage("BATCH_SCHED_READY", payload) {
      if (answer_mailbox.empty()) {
        throw std::invalid_argument(
                "BatchSchedReadyMessage::BatchSchedReadyMessage(): Empty answer mailbox");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param batsched_decision_reply: the decision replied by batsched (a batsim process)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(std::string answer_mailbox,
                                                                           std::string batsched_decision_reply,
                                                                           double payload)
            : BatchServiceMessage("BATCH_EXECUTE_JOB", payload) {
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
     * @param batsched_job_estimated_time: the estimated time to run the job done by batsched
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchQueryAnswerMessage::BatchQueryAnswerMessage(double estimated_waiting_time, double payload)
            : BatchServiceMessage("BATCH_QUERY_ANSWER", payload) {
      this->estimated_waiting_time = estimated_waiting_time;
    }

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to reply to
     * @param job: the batch job
     * @param job_args_to_scheduler: the arguments required by batscheduler of batsim
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(std::string answer_mailbox,
                                                                               WorkflowJob *job,
                                                                               std::string job_args_to_scheduler,
                                                                               double payload)
            : BatchServiceMessage("BATCH_JOB_SUBMISSION_TO_SCHEDULER", payload) {
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


//    /**
//     * @brief Constructor
//     * @param answer_mailbox: the mailbox to reply to
//     * @param json_data_string: the current and the expected resources required
//     * @param payload: message size in bytes
//     *
//     * @throw std::invalid_argument
//     */
//    BatchFakeJobSubmissionReplyMessage::BatchFakeJobSubmissionReplyMessage(std::string json_data_string, double payload)
//            : BatchServiceMessage("BATCH_FAKE_JOB_SUBMISSION_REPLY", payload) {
//      this->json_data_string = json_data_string;
//    }

    /**
     * @brief Constructor
     * @param reply: the replied answer by scheduler
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchJobReplyFromSchedulerMessage::BatchJobReplyFromSchedulerMessage(std::string reply, double payload)
            : BatchServiceMessage("BATCH_JOB_REPLY_FROM_SCHEDULER", payload), reply(reply) {}

    /**
     * @brief Constructor
     * @param answer: the workflow job
     * @param cs: the compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchServiceJobRequestMessage::BatchServiceJobRequestMessage(std::string answer_mailbox,
                                                                 BatchJob *job, double payload)
            : BatchServiceMessage("SUBMIT_BATCH_JOB_REQUEST", payload) {
      if (job == nullptr) {
        throw std::invalid_argument(
                "BatchServiceJobRequestMessage::BatchServiceJobRequestMessage(): Invalid arguments");
      }
      if (answer_mailbox.empty()) {
        throw std::invalid_argument(
                "BatchServiceJobRequestMessage::BatchServiceJobRequestMessage(): Empty answer mailbox");
      }
      this->job = job;
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param job: a workflow job
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    AlarmJobTimeOutMessage::AlarmJobTimeOutMessage(BatchJob *job, double payload)
            : ServiceMessage("ALARM_JOB_TIMED_OUT", payload) {
      if (job == nullptr) {
        throw std::invalid_argument(
                "AlarmJobTimeOutMessage::AlarmJobTimeOutMessage: Invalid argument");
      }
      this->job = job;
    }

    /**
     * @brief Constructor
     * @param job: a workflow job
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    AlarmNotifyBatschedMessage::AlarmNotifyBatschedMessage(std::string job_id, double payload)
            : ServiceMessage("ALARM_NOTIFY_BATSCHED", payload), job_id(job_id) {}

}
