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
     * @param payload: message size in bytes
     */
    ComputeServiceMessage::ComputeServiceMessage(double payload) : ServiceMessage(payload) {
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
            std::map<std::string, std::string> service_specific_args,
            double payload) : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (job == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceSubmitCompoundJobRequestMessage::ComputeServiceSubmitCompoundJobRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->job = std::move(job);
        this->service_specific_args = std::move(service_specific_args);
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
                                                                                               double payload) : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (compute_service == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "ComputeServiceSubmitCompoundJobAnswerMessage::ComputeServiceSubmitCompoundJobAnswerMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(compute_service);
        this->success = success;
        this->failure_cause = std::move(failure_cause);
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

#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceCompoundJobDoneMessage::ComputeServiceCompoundJobDoneMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(cs);
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
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceCompoundJobFailedMessage::ComputeServiceCompoundJobFailedMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(cs);
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
            double payload) : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) || (job == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceTerminateCompoundJobRequestMessage::ComputeServiceTerminateCompoundJobRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->job = std::move(job);
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
                                                                                                     double payload) : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (compute_service == nullptr) ||
            (success && (failure_cause != nullptr)) ||
            (!success && (failure_cause == nullptr))) {
            throw std::invalid_argument(
                    "ComputeServiceTerminateCompoundJobAnswerMessage::ComputeServiceTerminateCompoundJobAnswerMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(compute_service);
        this->success = success;
        this->failure_cause = std::move(failure_cause);
    }


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
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServicePilotJobStartedMessage::ComputeServicePilotJobStartedMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(cs);
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
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (cs == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServicePilotJobExpiredMessage::ComputeServicePilotJobExpiredMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(cs);
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
        : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_mailbox == nullptr) or key.empty()) {
            throw std::invalid_argument(
                    "ComputeServiceResourceInformationRequestMessage::ComputeServiceResourceInformationRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_mailbox = answer_mailbox;
        this->key = key;
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
        : ComputeServiceMessage(payload), info(std::move(info)) {}


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

#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (not answer_mailbox) {
            throw std::invalid_argument(
                    "ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage::ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(): "
                    "Invalid arguments");
        }
#endif
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
}// namespace wrench
