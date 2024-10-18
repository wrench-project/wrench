/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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
     * @param payload: the message size in bytes
     */
    BatchComputeServiceMessage::BatchComputeServiceMessage(sg_size_t payload) : ComputeServiceMessage(payload) {
    }

#if 0
    /**
     * @brief Constructor
     * @param answer_commport: the commport to reply to
     * @param job_args_to_scheduler: the arguments required by batscheduler of batsim
     * @param payload: the message size in bytes
     *
     */
    BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(S4U_CommPort *answer_commport,
                                                                                     std::string job_args_to_scheduler,
                                                                                     sg_size_t payload)
            : BatchComputeServiceMessage("BATCH_SIMULATION_BEGINS", payload) {
      if (job_args_to_scheduler.empty()) {
        throw std::invalid_argument(
                "BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(): Empty job arguments to scheduler");
      }
      if (answer_commport.empty()) {
        throw std::invalid_argument(
                "BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(): Empty answer commport");
      }
      this->answer_commport = answer_commport;
      this->job_args_to_scheduler = job_args_to_scheduler;
    }
#endif

#if 0
    /**
     * @brief Constructor
     * @param answer_commport: the commport to reply to
     * @param payload: the message size in bytes
     *
     */
    BatchSchedReadyMessage::BatchSchedReadyMessage(std::string &answer_commport, sg_size_t payload)
            : BatchComputeServiceMessage("BATCH_SCHED_READY", payload) {
      if (answer_commport.empty()) {
        throw std::invalid_argument(
                "BatchSchedReadyMessage::BatchSchedReadyMessage(): Empty answer commport");
      }
      this->answer_commport = answer_commport;
    }
#endif

    /**
     * @brief Constructor
     * @param answer_commport: the commport to reply to
     * @param batsched_decision_reply: the decision reply from Batsched
     * @param payload: the message size in bytes
     *
     */
    BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(S4U_CommPort *answer_commport,
                                                                           std::string batsched_decision_reply,
                                                                           sg_size_t payload)
        : BatchComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (answer_commport == nullptr) {
            throw std::invalid_argument(
                    "BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(): Empty answer commport");
        }
        if (batsched_decision_reply.empty()) {
            throw std::invalid_argument(
                    "BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(): Empty batsched decision reply");
        }
#endif
        this->answer_commport = answer_commport;
        this->batsched_decision_reply = std::move(batsched_decision_reply);
    }

    /**
     * @brief Constructor
     * @param estimated_job_start_time: the estimated job start time
     * @param payload: the message size in bytes
     *
     */
    BatchQueryAnswerMessage::BatchQueryAnswerMessage(double estimated_job_start_time, sg_size_t payload)
        : BatchComputeServiceMessage(payload) {
        this->estimated_start_time = estimated_job_start_time;
    }

#if 0
    /**
     * @brief Constructor
     * @param answer_commport: the commport to reply to
     * @param job: the BatchComputeService job
     * @param job_args_to_scheduler: the arguments required by batscheduler of batsim
     * @param payload: the message size in bytes
     *
     */
    BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(std::string &answer_commport,
                                                                               Job *job,
                                                                               std::string job_args_to_scheduler,
                                                                               sg_size_t payload)
            : BatchComputeServiceMessage("BATCH_JOB_SUBMISSION_TO_SCHEDULER", payload) {
      if (job_args_to_scheduler.empty()) {
        throw std::invalid_argument(
                "BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(): Empty job arguments to scheduler");
      }
      if (job == nullptr) {
        throw std::invalid_argument("BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(): invalid job");
      }
      if (answer_commport.empty()) {
        throw std::invalid_argument("BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(): invalid answer commport");
      }
      this->job_args_to_scheduler = job_args_to_scheduler;
      this->answer_commport = answer_commport;
      this->job = job;
    }
#endif


#if 0
    /**
     * @brief Constructor
     * @param reply: the replied answer by scheduler
     * @param payload: the message size in bytes
     *
     */
    BatchJobReplyFromSchedulerMessage::BatchJobReplyFromSchedulerMessage(std::string reply, sg_size_t payload)
            : BatchComputeServiceMessage("BATCH_JOB_REPLY_FROM_SCHEDULER", payload), reply(reply) {}

#endif

    /**
     * @brief Constructor
     * @param answer_commport: the commport to which the answer should be sent back
     * @param job: the BatchComputeService job
     * @param payload: the message size in bytes
     *
     */
    BatchComputeServiceJobRequestMessage::BatchComputeServiceJobRequestMessage(S4U_CommPort *answer_commport,
                                                                               std::shared_ptr<BatchJob> job, sg_size_t payload)
        : BatchComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (job == nullptr) {
            throw std::invalid_argument(
                    "BatchComputeServiceJobRequestMessage::BatchComputeServiceJobRequestMessage(): Invalid arguments");
        }
        if (answer_commport == nullptr) {
            throw std::invalid_argument(
                    "BatchComputeServiceJobRequestMessage::BatchComputeServiceJobRequestMessage(): Empty answer commport");
        }
#endif
        this->answer_commport = answer_commport;
        this->job = std::move(job);
    }

    /**
     * @brief Constructor
     * @param job: a BatchComputeService job
     * @param payload: the message size in bytes
     *
     */
    AlarmJobTimeOutMessage::AlarmJobTimeOutMessage(std::shared_ptr<BatchJob> job, sg_size_t payload)
        : ServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (job == nullptr) {
            throw std::invalid_argument(
                    "AlarmJobTimeOutMessage::AlarmJobTimeOutMessage: Invalid argument");
        }
#endif
        this->job = std::move(job);
    }

#if 0
    /**
     * @brief Constructor
     * @param job_id: the id of a BatchComputeService job
     * @param payload:the message size in bytes
     *
     */
    AlarmNotifyBatschedMessage::AlarmNotifyBatschedMessage(std::string job_id, sg_size_t payload)
            : ServiceMessage("ALARM_NOTIFY_BATSCHED", payload), job_id(job_id) {}
#endif

}// namespace wrench
