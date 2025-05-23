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
    ComputeServiceMessage::ComputeServiceMessage(sg_size_t payload) : ServiceMessage(payload) {
    }

    /**
   * @brief Constructor
   * @param answer_commport: commport to which the answer message should be sent
   * @param job: a compound job submitted for execution
   * @param service_specific_args: a map of extra arguments (each specified by a name and value, both strings) required by some services
   * @param payload: message size in bytes
   *
   */
    ComputeServiceSubmitCompoundJobRequestMessage::ComputeServiceSubmitCompoundJobRequestMessage(
            S4U_CommPort *answer_commport,
            std::shared_ptr<CompoundJob> job,
            std::map<std::string, std::string> service_specific_args,
            sg_size_t payload) : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (job == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceSubmitCompoundJobRequestMessage::ComputeServiceSubmitCompoundJobRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
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
     */
    ComputeServiceSubmitCompoundJobAnswerMessage::ComputeServiceSubmitCompoundJobAnswerMessage(std::shared_ptr<CompoundJob> job,
                                                                                               std::shared_ptr<ComputeService> compute_service,
                                                                                               bool success,
                                                                                               std::shared_ptr<FailureCause> failure_cause,
                                                                                               sg_size_t payload) : ComputeServiceMessage(payload) {
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
     */
    ComputeServiceCompoundJobDoneMessage::ComputeServiceCompoundJobDoneMessage(std::shared_ptr<CompoundJob> job,
                                                                               std::shared_ptr<ComputeService> cs,
                                                                               sg_size_t payload)
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
     */
    ComputeServiceCompoundJobFailedMessage::ComputeServiceCompoundJobFailedMessage(std::shared_ptr<CompoundJob> job,
                                                                                   std::shared_ptr<ComputeService> cs,
                                                                                   sg_size_t payload)
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
    * @param answer_commport: commport to which the answer message should be sent
    * @param job: a compound job to terminate
    * @param payload: message size in bytes
    *
    */
    ComputeServiceTerminateCompoundJobRequestMessage::ComputeServiceTerminateCompoundJobRequestMessage(
            S4U_CommPort *answer_commport,
            std::shared_ptr<CompoundJob> job,
            sg_size_t payload) : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (job == nullptr)) {
            throw std::invalid_argument(
                    "ComputeServiceTerminateCompoundJobRequestMessage::ComputeServiceTerminateCompoundJobRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
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
     */
    ComputeServiceTerminateCompoundJobAnswerMessage::ComputeServiceTerminateCompoundJobAnswerMessage(std::shared_ptr<CompoundJob> job,
                                                                                                     std::shared_ptr<ComputeService> compute_service,
                                                                                                     bool success,
                                                                                                     std::shared_ptr<FailureCause> failure_cause,
                                                                                                     sg_size_t payload) : ComputeServiceMessage(payload) {
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
     */
    ComputeServicePilotJobStartedMessage::ComputeServicePilotJobStartedMessage(std::shared_ptr<PilotJob> job,
                                                                               std::shared_ptr<ComputeService> cs,
                                                                               sg_size_t payload)
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
     */
    ComputeServicePilotJobExpiredMessage::ComputeServicePilotJobExpiredMessage(std::shared_ptr<PilotJob> job,
                                                                               std::shared_ptr<ComputeService> cs,
                                                                               sg_size_t payload)
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
     * @param answer_commport: the commport to which the answer should be sent
     * @param key: the desired resource information (i.e., dictionary key) that's needed)
     * @param payload: the message size in bytes
     *
     */
    ComputeServiceResourceInformationRequestMessage::ComputeServiceResourceInformationRequestMessage(
            S4U_CommPort *answer_commport,
            const std::string &key,
            sg_size_t payload)
        : ComputeServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) or key.empty()) {
            throw std::invalid_argument(
                    "ComputeServiceResourceInformationRequestMessage::ComputeServiceResourceInformationRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->key = key;
    }


    /**
     * @brief Constructor
     * @param info: the resource description map
     * @param payload: the message size in bytes
     *
     */
    ComputeServiceResourceInformationAnswerMessage::ComputeServiceResourceInformationAnswerMessage(
            std::map<std::string, double> info, sg_size_t payload)
        : ComputeServiceMessage(payload), info(std::move(info)) {}


    /**
    * @brief Constructor
    *
    * @param answer_commport: the commport to which to send the answer
    * @param num_cores: the desired number of cores
    * @param ram: the desired RAM
    * @param payload: the message size in bytes
    *
    */
    ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage::ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(
            S4U_CommPort *answer_commport, unsigned long num_cores, sg_size_t ram, sg_size_t payload) : ComputeServiceMessage(payload) {

#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (not answer_commport) {
            throw std::invalid_argument(
                    "ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage::ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(): "
                    "Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
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
            bool answer, sg_size_t payload) : ComputeServiceMessage(payload), answer(answer) {}
}// namespace wrench
