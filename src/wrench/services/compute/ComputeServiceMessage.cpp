/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <iostream>
#include <utility>
#include <wrench/services/compute/ComputeServiceMessage.h>

namespace wrench {

    /**
     * @brief Constructor
     * @param name: message name
     * @param payload: message size in bytes
     */
    ComputeServiceMessage::ComputeServiceMessage(double payload) :
            ServiceMessage( payload) {
    }

    /**
   * @brief Constructor
   * @param answer_mailbox: mailbox to which the answer message should be sent
   * @param job: a compound job submitted for execution
   * @param service_specific_args: a map of extra arguments (each specified by a name and value, both strings) required by some services
   * @param payload: message size in bytes
   *
   * @throw std::invalid_arguments
   */
    ComputeServiceSubmitCompoundJobRequestMessage::ComputeServiceSubmitCompoundJobRequestMessage(
            simgrid::s4u::Mailbox *answer_mailbox,
            std::shared_ptr<CompoundJob> job,
            const std::map<std::string, std::string> service_specific_args,
            double payload) :
            ComputeServiceMessage(payload),
            answer_mailbox(answer_mailbox), job(job), service_specific_args(service_specific_args) {
        if ((answer_mailbox == nullptr) || (job == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceSubmitCompoundJobRequestMessage::ComputeServiceSubmitCompoundJobRequestMessage(): Invalid arguments");
        }
    }

    /**
     * @brief Constructor
     * @param job: a compound job that had been submitted for execution
     * @param compute_service: the compute service
     * @param success: true on success, false otherwise
     * @param failure_cause: cause of the failure (nullptr if success == true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceSubmitCompoundJobAnswerMessage::ComputeServiceSubmitCompoundJobAnswerMessage(std::shared_ptr<CompoundJob> job,
                                                                                               std::shared_ptr<ComputeService> compute_service,
                                                                                               bool success,
                                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                                               double payload) :
            ComputeServiceMessage(payload) {
        if ((job == nullptr) || (compute_service == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "ComputeServiceSubmitCompoundJobAnswerMessage::ComputeServiceSubmitCompoundJobAnswerMessage(): Invalid arguments");
        }
        this->job = job;
        this->compute_service = compute_service;
        this->success = success;
        this->failure_cause = failure_cause;
    }

    /**
     * @brief Constructor
     * @param job: a compound job that has completed
     * @param cs: the compute service on which the job has completed
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceCompoundJobDoneMessage::ComputeServiceCompoundJobDoneMessage(std::shared_ptr<CompoundJob> job,
                                                                               std::shared_ptr<ComputeService> cs,
                                                                               double payload)
            : ComputeServiceMessage(payload) {
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceCompoundJobDoneMessage::ComputeServiceCompoundJobDoneMessage(): Invalid arguments");
        }
        this->job = job;
        this->compute_service = cs;
    }

    /**
     * @brief Constructor
     * @param job: a compound job that has failed
     * @param cs: the compute service on which the job has failed
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceCompoundJobFailedMessage::ComputeServiceCompoundJobFailedMessage(std::shared_ptr<CompoundJob> job,
                                                                                   std::shared_ptr<ComputeService> cs,
                                                                                   double payload)
            : ComputeServiceMessage(payload) {
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceCompoundJobFailedMessage::ComputeServiceCompoundJobFailedMessage(): Invalid arguments");
        }
        this->job = job;
        this->compute_service = cs;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: mailbox to which the answer message should be sent
    * @param job: a compound job to terminate
    * @param payload: message size in bytes
    *
    * @throw std::invalid_arguments
    */
    ComputeServiceTerminateCompoundJobRequestMessage::ComputeServiceTerminateCompoundJobRequestMessage(
            simgrid::s4u::Mailbox *answer_mailbox,
            std::shared_ptr<CompoundJob> job,
            double payload) :
            ComputeServiceMessage( payload), answer_mailbox(answer_mailbox), job(job) {
        if ((answer_mailbox == nullptr) || (job == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceTerminateCompoundJobRequestMessage::ComputeServiceTerminateCompoundJobRequestMessage(): Invalid arguments");
        }
    }

    /**
     * @brief Constructor
     * @param job: a compound job whose termination was requested
     * @param compute_service: the compute service that was executing the standard job
     * @param success: true on success, false otherwise
     * @param failure_cause: cause of the failure (nullptr if success == true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceTerminateCompoundJobAnswerMessage::ComputeServiceTerminateCompoundJobAnswerMessage(std::shared_ptr<CompoundJob> job,
                                                                                                     std::shared_ptr<ComputeService> compute_service,
                                                                                                     bool success,
                                                                                                     std::shared_ptr<FailureCause> failure_cause,
                                                                                                     double payload) :
            ComputeServiceMessage(payload) {
        if ((job == nullptr) || (compute_service == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "ComputeServiceTerminateCompoundJobAnswerMessage::ComputeServiceTerminateCompoundJobAnswerMessage(): Invalid arguments");
        }
        this->job = job;
        this->compute_service = compute_service;
        this->success = success;
        this->failure_cause = failure_cause;
    }

//    /**
//     * @brief Constructor
//     * @param answer_mailbox: mailbox to which the answer message should be sent
//     * @param job: a pilot job submitted for execution
//     * @param service_specific_args: a map of extra arguments (each specified by a name and value, both strings) required by some services
//     * @param payload: message size in bytes
//     *
//     * @throw std::invalid_argument
//     */
//    ComputeServiceSubmitPilotJobRequestMessage::ComputeServiceSubmitPilotJobRequestMessage(std::string answer_mailbox,
//                                                                                           std::shared_ptr<PilotJob> job,
//                                                                                           const std::map<std::string, std::string> service_specific_args,
//                                                                                           double payload)
//            : ComputeServiceMessage(
//            "SUBMIT_PILOT_JOB_REQUEST", payload) {
//        if ((job == nullptr) || (answer_mailbox == "")) {
//            throw std::invalid_argument(
//                    "ComputeServiceSubmitPilotJobRequestMessage::ComputeServiceSubmitPilotJobRequestMessage(): Invalid arguments");
//        }
//        this->answer_mailbox = answer_mailbox;
//        this->job = job;
//        this->service_specific_args = service_specific_args;
//    }
//
//    /**
//     * @brief Constructor
//     * @param job: the pilot job
//     * @param compute_service: the compute service to which the job had been submitted
//     * @param success: whether the submission was successful or not
//     * @param failure_cause: cause of the failure (nullptr if success == true)
//     * @param payload: message size in bytes
//     *
//     * @throw std::invalid_argument
//     */
//    ComputeServiceSubmitPilotJobAnswerMessage::ComputeServiceSubmitPilotJobAnswerMessage(std::shared_ptr<PilotJob> job,
//                                                                                         std::shared_ptr<ComputeService> compute_service,
//                                                                                         bool success,
//                                                                                         std::shared_ptr<FailureCause> failure_cause,
//                                                                                         double payload)
//            : ComputeServiceMessage(
//            "SUBMIT_PILOT_JOB_ANSWER", payload) {
//        if ((job == nullptr) || (compute_service == nullptr) ||
//            (success && (failure_cause != nullptr)) ||
//            (!success && (failure_cause == nullptr))) {
//            throw std::invalid_argument(
//                    "ComputeServiceSubmitPilotJobAnswerMessage::ComputeServiceSubmitPilotJobAnswerMessage(): Invalid arguments");
//        }
//        this->job = job;
//        this->compute_service = compute_service;
//        this->success = success;
//        this->failure_cause = failure_cause;
//    }
//
    /**
     * @brief Constructor
     * @param job: a pilot job that has started execution
     * @param cs: the compute service on which the pilot job has started
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServicePilotJobStartedMessage::ComputeServicePilotJobStartedMessage(std::shared_ptr<PilotJob> job,
                                                                               std::shared_ptr<ComputeService> cs,
                                                                               double payload)
            : ComputeServiceMessage(payload) {

        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServicePilotJobStartedMessage::ComputeServicePilotJobStartedMessage(): Invalid arguments");
        }
        this->job = job;
        this->compute_service = cs;
    }

    /**
     * @brief Constructor
     * @param job: a pilot job that has expired
     * @param cs: the compute service on which the pilot job has expired
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServicePilotJobExpiredMessage::ComputeServicePilotJobExpiredMessage(std::shared_ptr<PilotJob> job,
                                                                               std::shared_ptr<ComputeService> cs,
                                                                               double payload)
            : ComputeServiceMessage(payload) {
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServicePilotJobExpiredMessage::ComputeServicePilotJobExpiredMessage(): Invalid arguments");
        }
        this->job = job;
        this->compute_service = cs;
    }

    /**
     * @brief Constructor
     * @param job: a pilot job that has failed
     * @param cs: the compute service on which the pilot job has failed
     * @param cause: the failure cause
     * @param payload: message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServicePilotJobFailedMessage::ComputeServicePilotJobFailedMessage(std::shared_ptr<PilotJob> job,
                                                                             std::shared_ptr<ComputeService> cs,
                                                                             std::shared_ptr<FailureCause> cause,
                                                                             double payload) : ComputeServiceMessage(
            payload) {
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServicePilotJobFailedMessage::ComputeServicePilotJobFailedMessage(): Invalid arguments");
        }
        this->job = job;
        this->compute_service = cs;
        this->cause = cause;
    }

    /**
    * @brief Constructor
    * @param answer_mailbox: mailbox to which the answer message should be sent
    * @param job: a pilot job whose termination is requested
    * @param payload: message size in bytes
    *
    * @throw std::invalid_arguments
    */
    ComputeServiceTerminatePilotJobRequestMessage::ComputeServiceTerminatePilotJobRequestMessage(
            simgrid::s4u::Mailbox *answer_mailbox,
            std::shared_ptr<PilotJob> job,
            double payload) :
            ComputeServiceMessage(payload) {
        if ((answer_mailbox == nullptr) || (job == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceTerminatePilotJobRequestMessage::ComputeServiceTerminatePilotJobRequestMessage(): Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->job = job;
    }

    /**
     * @brief Constructor
     * @param job: a pilot job whose termination had been requested
     * @param compute_service: the compute service
     * @param success: true on success, false otherwise
     * @param failure_cause: cause of the failure (nullptr if success == true)
     * @param payload: message size in bytes
     *
     * @throw std::invalid_arguments
     */
    ComputeServiceTerminatePilotJobAnswerMessage::ComputeServiceTerminatePilotJobAnswerMessage(std::shared_ptr<PilotJob> job,
                                                                                               std::shared_ptr<ComputeService> compute_service,
                                                                                               bool success,
                                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                                               double payload) :
            ComputeServiceMessage( payload) {
        if ((job == nullptr) || (compute_service == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            (!success && (failure_cause == nullptr))) {
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
     * @param answer_mailbox: the mailbox to which the answer should be sent
     * @param key: the desired resource information (i.e., dictionary key) that's needed)
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceResourceInformationRequestMessage::ComputeServiceResourceInformationRequestMessage(
            simgrid::s4u::Mailbox *answer_mailbox,
            const std::string &key,
            double payload)
            : ComputeServiceMessage(payload), answer_mailbox(answer_mailbox), key(key) {
        if ((answer_mailbox == nullptr) or key.empty()) {
            throw std::invalid_argument(
                    "ComputeServiceResourceInformationRequestMessage::ComputeServiceResourceInformationRequestMessage(): Invalid arguments");
        }
    }


    /**
     * @brief Constructor
     * @param info: the resource description map
     * @param payload: the message size in bytes
     *
     * @throw std::invalid_argument
     */
    ComputeServiceResourceInformationAnswerMessage::ComputeServiceResourceInformationAnswerMessage(
            std::map<std::string, double> info, double payload)
            : ComputeServiceMessage( payload), info(std::move(info)) {}


    /**
    * @brief Constructor
    *
    * @param answer_mailbox: the mailbox to which to send the answer
    * @param num_cores: the desired number of cores
    * @param ram: the desired RAM
    * @param payload: the message size in bytes
    *
    * @throw std::invalid_argument
    */
    ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage::ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(
            simgrid::s4u::Mailbox *answer_mailbox, unsigned long num_cores, double ram, double payload) : ComputeServiceMessage(

            payload) {

        if (not answer_mailbox) {
            throw std::invalid_argument(
                    "ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage::ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(): "
                    "Invalid arguments");
        }
        this->answer_mailbox = answer_mailbox;
        this->num_cores = num_cores;
        this->ram = ram;
    }

    /**
     * @brief Constructor
     *
     * @param answer: true/false
     * @param payload: the message size in bytes
     */
    ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage::ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage(
            bool answer, double payload) : ComputeServiceMessage(payload), answer(answer) {}

};
