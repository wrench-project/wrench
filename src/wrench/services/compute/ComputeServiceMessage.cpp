/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "ComputeServiceMessage.h"

namespace wrench {

    /**
     * @brief Constructor
     * @param name: message name
     * @param payload: message payload
     */
    ComputeServiceMessage::ComputeServiceMessage(std::string name, double payload) :
            ServiceMessage("ComputeServiceMessage::" + name, payload) {
    }

    /**
     * @brief Constructor
     * @param job: a workflow job
     * @param cs: a compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceJobTypeNotSupportedMessage::ComputeServiceJobTypeNotSupportedMessage(WorkflowJob *job,
                                                                                       ComputeService *cs,
                                                                                       double payload)
            : ComputeServiceMessage("JOB_TYPE_NOT_SUPPORTED", payload) {
      if ((job == nullptr) || (cs == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceJobTypeNotSupportedMessage::ComputeServiceJobTypeNotSupportedMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = cs;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: mailbox to which the answer message should be sent
    * @param job: a standard job
    * @param payload: message size in bytes
    *
    * @throw std::invalid_arguments
    */
    ComputeServiceSubmitStandardJobRequestMessage::ComputeServiceSubmitStandardJobRequestMessage(
            std::string answer_mailbox,
            StandardJob *job,
            double payload) :
            ComputeServiceMessage("SUBMIT_STANDARD_JOB_REQUEST", payload) {
      if ((answer_mailbox == "") || (job == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceSubmitStandardJobRequestMessage::ComputeServiceSubmitStandardJobRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->job = job;
    }

    /**
     * @brief Constructor
     * @param job: a standard job
     * @param compute_service: the compute service
     * @param success: true on success, false otherwise
     * @param failure_cause: cause of the failure (nullptr is success=true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceSubmitStandardJobAnswerMessage::ComputeServiceSubmitStandardJobAnswerMessage(StandardJob *job,
                                                                                               ComputeService *compute_service,
                                                                                               bool success,
                                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                                               double payload) :
            ComputeServiceMessage("SUBMIT_STANDARD_JOB_ANSWER", payload) {
      if ((job == nullptr) || (compute_service == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceSubmitStandardJobAnswerMessage::ComputeServiceSubmitStandardJobAnswerMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = compute_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }

    /**
     * @brief Constructor
     * @param job: a standard job
     * @param cs: points to a ComputeService
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceStandardJobDoneMessage::ComputeServiceStandardJobDoneMessage(StandardJob *job, ComputeService *cs,
                                                                               double payload)
            : ComputeServiceMessage("STANDARD_JOB_DONE", payload) {
      if (job == nullptr) {
        throw std::invalid_argument(
                "ComputeServiceStandardJobDoneMessage::ComputeServiceStandardJobDoneMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = cs;
    }

    /**
     * @brief Constructor
     * @param job: a standard job
     * @param cs: a compute service
     * @param cause: the cause of the failure
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceStandardJobFailedMessage::ComputeServiceStandardJobFailedMessage(StandardJob *job,
                                                                                   ComputeService *cs,
                                                                                   std::shared_ptr<FailureCause> cause,
                                                                                   double payload)
            : ComputeServiceMessage("STANDARD_JOB_FAILED", payload) {
      if ((job == nullptr) || (cs == nullptr) || (cause == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceStandardJobFailedMessage::ComputeServiceStandardJobFailedMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = cs;
      this->cause = cause;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: mailbox to which the answer message should be sent
    * @param job: a standard job
    * @param payload: message size in bytes
    *
    * @throw std::invalid_arguments
    */
    ComputeServiceTerminateStandardJobRequestMessage::ComputeServiceTerminateStandardJobRequestMessage(
            std::string answer_mailbox,
            StandardJob *job,
            double payload) :
            ComputeServiceMessage("TERMINATE_STANDARD_JOB_REQUEST", payload) {
      if ((answer_mailbox == "") || (job == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceTerminateStandardJobRequestMessage::ComputeServiceTerminateStandardJobRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->job = job;
    }

    /**
     * @brief Constructor
     * @param job: a standard job
     * @param compute_service: the compute service
     * @param success: true on success, false otherwise
     * @param failure_cause: cause of the failure (nullptr is success=true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceTerminateStandardJobAnswerMessage::ComputeServiceTerminateStandardJobAnswerMessage(StandardJob *job,
                                                                                                     ComputeService *compute_service,
                                                                                                     bool success,
                                                                                                     std::shared_ptr<FailureCause> failure_cause,
                                                                                                     double payload) :
            ComputeServiceMessage("TERMINATE_STANDARD_JOB_ANSWER", payload) {
      if ((job == nullptr) || (compute_service == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceTerminateStandardJobAnswerMessage::ComputeServiceTerminateStandardJobAnswerMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = compute_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }


    /**
     * @brief Constructor
     * @param answer_mailbox: mailbox to which the answer message should be sent
     * @param job: a pilot job
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceSubmitPilotJobRequestMessage::ComputeServiceSubmitPilotJobRequestMessage(std::string answer_mailbox,
                                                                                           PilotJob *job,
                                                                                           double payload)
            : ComputeServiceMessage(
            "SUBMIT_PILOT_JOB_REQUEST", payload) {
      if ((job == nullptr) || (answer_mailbox == "")) {
        throw std::invalid_argument(
                "ComputeServiceSubmitPilotJobRequestMessage::ComputeServiceSubmitPilotJobRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->job = job;
    }

    /**
     * @brief Constructor
     * @param job: the pilot job
     * @param compute_service: the compute service
     * @param success: whether the submission is accepted
     * @param failure_cause: the failure cause (nullptr is success=true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceSubmitPilotJobAnswerMessage::ComputeServiceSubmitPilotJobAnswerMessage(PilotJob *job,
                                                                                         ComputeService *compute_service,
                                                                                         bool success,
                                                                                         std::shared_ptr<FailureCause> failure_cause,
                                                                                         double payload)
            : ComputeServiceMessage(
            "SUBMIT_PILOT_JOB_ANSWER", payload) {
      if ((job == nullptr) || (compute_service == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceSubmitPilotJobAnswerMessage::ComputeServiceSubmitPilotJobAnswerMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = compute_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }

    /**
     * @brief Constructor
     * @param job: a pilot job
     * @param cs: a compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServicePilotJobStartedMessage::ComputeServicePilotJobStartedMessage(PilotJob *job, ComputeService *cs,
                                                                               double payload)
            : ComputeServiceMessage("PILOT_JOB_STARTED", payload) {

      if ((job == nullptr) || (cs == nullptr)) {
        throw std::invalid_argument(
                "ComputeServicePilotJobStartedMessage::ComputeServicePilotJobStartedMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = cs;
    }

    /**
     * @brief Constructor
     * @param job: a pilot job
     * @param cs: a compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServicePilotJobExpiredMessage::ComputeServicePilotJobExpiredMessage(PilotJob *job, ComputeService *cs,
                                                                               double payload)
            : ComputeServiceMessage("PILOT_JOB_EXPIRED", payload) {
      if ((job == nullptr) || (cs == nullptr)) {
        throw std::invalid_argument(
                "ComputeServicePilotJobExpiredMessage::ComputeServicePilotJobExpiredMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = cs;
    }

    /**
     * @brief Constructor
     * @param job: a pilot job
     * @param cs: a compute service
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServicePilotJobFailedMessage::ComputeServicePilotJobFailedMessage(PilotJob *job, ComputeService *cs,
                                                                             double payload) : ComputeServiceMessage(
            "PILOT_JOB_FAILED", payload) {
      if ((job == nullptr) || (cs == nullptr)) {
        throw std::invalid_argument(
                "ComputeServicePilotJobFailedMessage::ComputeServicePilotJobFailedMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = cs;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: mailbox to which the answer message should be sent
    * @param job: a pilot job
    * @param payload: message size in bytes
    *
    * @throw std::invalid_arguments
    */
    ComputeServiceTerminatePilotJobRequestMessage::ComputeServiceTerminatePilotJobRequestMessage(
            std::string answer_mailbox,
            PilotJob *job,
            double payload) :
            ComputeServiceMessage("TERMINATE_PILOT_JOB_REQUEST", payload) {
      if ((answer_mailbox == "") || (job == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceTerminatePilotJobRequestMessage::ComputeServiceTerminatePilotJobRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
      this->job = job;
    }

    /**
     * @brief Constructor
     * @param job: a pilot job
     * @param compute_service: the compute service
     * @param success: true on success, false otherwise
     * @param failure_cause: cause of the failure (nullptr is success=true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceTerminatePilotJobAnswerMessage::ComputeServiceTerminatePilotJobAnswerMessage(PilotJob *job,
                                                                                               ComputeService *compute_service,
                                                                                               bool success,
                                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                                               double payload) :
            ComputeServiceMessage("TERMINATE_PILOT_JOB_ANSWER", payload) {
      if ((job == nullptr) || (compute_service == nullptr)) {
        throw std::invalid_argument(
                "ComputeServiceTerminatePilotJobAnswerMessage::ComputeServiceTerminatePilotJobAnswerMessage(): Invalid arguments");
      }
      this->job = job;
      this->compute_service = compute_service;
      this->success = success;
      this->failure_cause = failure_cause;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    ComputeServiceNumCoresRequestMessage::ComputeServiceNumCoresRequestMessage(
            std::string answer_mailbox, double payload) : ComputeServiceMessage("NUM_CORES_REQUEST", payload) {
      if (answer_mailbox.empty()) {
        throw std::invalid_argument(
                "ComputeServiceNumCoresRequestMessage::ComputeServiceNumCoresRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param num: number of cores
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceNumCoresAnswerMessage::ComputeServiceNumCoresAnswerMessage(unsigned int num, double payload)
            : ComputeServiceMessage("NUM_CORES_ANSWER", payload), num_cores(num) {}

    /**
     * @brief Constructor
     * @param answer_mailbox: the mailbox to which to send the answer
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceNumIdleCoresRequestMessage::ComputeServiceNumIdleCoresRequestMessage(std::string answer_mailbox,
                                                                                       double payload)
            : ComputeServiceMessage("NUM_IDLE_CORES_REQUEST", payload) {
      if (answer_mailbox.empty()) {
        throw std::invalid_argument(
                "ComputeServiceNumIdleCoresRequestMessage::ComputeServiceNumIdleCoresRequestMessage(): Invalid arguments");
      }
      this->answer_mailbox = answer_mailbox;
    }

    /**
     * @brief Constructor
     * @param num: number of idle cores
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceNumIdleCoresAnswerMessage::ComputeServiceNumIdleCoresAnswerMessage(unsigned long num, double payload)
            : ComputeServiceMessage("NUM_IDLE_CORES_ANSWER", payload), num_idle_cores(num) {}
};
