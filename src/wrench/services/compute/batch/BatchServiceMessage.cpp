//
// Created by suraj on 9/16/17.
//

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
                                                                                     std::string job_args_to_scheduler, double payload)
            : BatchServiceMessage("BATCH_SIMULATION_BEGINS", payload) {
        if (job_args_to_scheduler.empty()) {
            throw std::invalid_argument("BatchSimulationBeginsToSchedulerMessage::BatchSimulationBeginsToSchedulerMessage(): Empty job arguments to scheduler");
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
    BatchExecuteJobFromBatSchedMessage::BatchExecuteJobFromBatSchedMessage(std::string answer_mailbox, std::string batsched_decision_reply, double payload)
            : BatchServiceMessage("BATCH_EXECUTE_JOB", payload) {
        this->answer_mailbox = answer_mailbox;
        this->batsched_decision_reply = batsched_decision_reply;
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
    BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(std::string answer_mailbox, WorkflowJob* job,
                                                                               std::string job_args_to_scheduler, double payload)
            : BatchServiceMessage("BATCH_JOB_SUBMISSION_TO_SCHEDULER", payload) {
        if (job_args_to_scheduler.empty()) {
            throw std::invalid_argument("BatchJobSubmissionToSchedulerMessage::BatchJobSubmissionToSchedulerMessage(): Empty job arguments to scheduler");
        }
        this->job_args_to_scheduler = job_args_to_scheduler;
        this->answer_mailbox = answer_mailbox;
        this->job = job;
    }

    /**
     * @brief Constructor
     * @param reply: the replied answer by scheduler
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchJobReplyFromSchedulerMessage::BatchJobReplyFromSchedulerMessage(std::string reply, double payload)
            : BatchServiceMessage("BATCH_JOB_REPLY_FROM_SCHEDULER", payload) {
        this->reply = reply;
    }

    /**
     * @brief Constructor
     * @param answer: the workflow job
     * @param cs: the compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    BatchServiceJobRequestMessage::BatchServiceJobRequestMessage(std::string answer_mailbox,
                                                                 BatchJob* job,double payload)
            : BatchServiceMessage("SUBMIT_BATCH_JOB_REQUEST", payload) {
        if (job == nullptr) {
            throw std::invalid_argument("BatchServiceJobRequestMessage::BatchServiceJobRequestMessage(): Invalid arguments");
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
    AlarmJobTimeOutMessage::AlarmJobTimeOutMessage(WorkflowJob* job,double payload)
            : ServiceMessage("ALARM_JOB_TIMED_OUT", payload) {
        this->job = job;
    }

}