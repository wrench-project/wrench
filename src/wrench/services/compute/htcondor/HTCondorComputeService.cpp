/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceProperty.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/workflow/failure_causes/NotEnoughResources.h>
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/htcondor/HTCondorComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/workflow/failure_causes/JobTypeNotSupported.h"
#include "wrench/workflow/failure_causes/NetworkError.h"

WRENCH_LOG_CATEGORY(wrench_core_HTCondor, "Log category for HTCondorComputeService Scheduler");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which to start the service
     * @param compute_services: a set of 'child' compute services that have been added to the
     *    simulation and that are available to and usable through the HTCondor pool.
     *   - BatchComputeService instances will be used for Condor jobs in the "grid" universe
     *   - BareMetalComputeService instances will be used for Condor jobs not in the "grid" universe
     *   - other types of compute services are disallowed
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    HTCondorComputeService::HTCondorComputeService(const std::string &hostname,
                                                   std::set<std::shared_ptr<ComputeService>> compute_services,
                                                   std::map<std::string, std::string> property_list,
                                                   std::map<std::string, double> messagepayload_list) :
            ComputeService(hostname, "htcondor_service", "htcondor_service", "") {

        // Check the property list for things that the user should not specify
        if (property_list.find(ComputeServiceProperty::SUPPORTS_STANDARD_JOBS) != property_list.end()) {
            std::invalid_argument("HTCondorComputeService::HTCondorComputeService(): ComputeServiceProperty::SUPPORTS_STANDARD_JOBS cannot be set for "
                                  "an HTCondorComputeService, as it is based on the capabilities of the child compute services");
        }
        if (property_list.find(ComputeServiceProperty::SUPPORTS_PILOT_JOBS) != property_list.end()) {
            std::invalid_argument("HTCondorComputeService::HTCondorComputeService(): ComputeServiceProperty::SUPPORTS_PILOT_JOBS cannot be set for "
                                  "an HTCondorComputeService, as it is based on the capabilities of the child compute services");
        }

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

//        // Check that there are child services
//        if (compute_services.empty()) {
//            throw std::invalid_argument("HTCondorComputeService::HTCondorComputeService(): at least one 'child' compute service should be provided");
//        }

        // Check that all services are of allowed types
        for (auto const &cs: compute_services) {
            if ((not std::dynamic_pointer_cast<BatchComputeService>(cs)) and
                (not std::dynamic_pointer_cast<BareMetalComputeService>(cs))) {
                throw std::invalid_argument(
                        "HTCondorComputeService::HTCondorComputeService(): Only BatchComputeService or BareMetalComputeService instances can be used as 'child' services");
            }
        }

        // Determine pilot job support
        bool at_least_one_service_supports_pilot_jobs = false;
        for (auto const &cs: compute_services) {
            if (cs->supportsPilotJobs()) {
                at_least_one_service_supports_pilot_jobs = true;
                break;
            }
        }

        // Determine standard job support
        bool at_least_one_service_supports_standard_jobs = false;
        for (auto const &cs: compute_services) {
            if (cs->supportsStandardJobs()) {
                at_least_one_service_supports_standard_jobs = true;
                break;
            }
        }

        // Determine if there is at least one batch service
        bool at_least_one_batch_service = false;
        for (auto const &cs: compute_services) {
            if (std::dynamic_pointer_cast<BatchComputeService>(cs)) {
                at_least_one_batch_service = true;
                break;
            }
        }

        // create central manager service
        this->central_manager = std::make_shared<HTCondorCentralManagerService>(
                hostname,
                getPropertyValueAsDouble(HTCondorComputeServiceProperty::NEGOTIATOR_OVERHEAD),
                compute_services, property_list, messagepayload_list);
    }

    /**
     * @brief Destructor
     */
    HTCondorComputeService::~HTCondorComputeService() {
        this->central_manager = nullptr;
        this->default_property_values.clear();
        this->default_messagepayload_values.clear();
    }

    /**
     * @brief Add a new 'child' compute service
     */
    void HTCondorComputeService::addComputeService(std::shared_ptr<ComputeService> compute_service) {
        this->central_manager->addComputeService(compute_service);
    }

    /**
     * @brief Submit a standard job to the HTCondor service
     *
     * @param job: a standard job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void HTCondorComputeService::submitStandardJob(std::shared_ptr<StandardJob> job,
                                                   const std::map<std::string, std::string> &service_specific_args) {

        serviceSanityCheck();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

        //  send a "run a standard job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new ComputeServiceSubmitStandardJobRequestMessage(
                            answer_mailbox, job, service_specific_args,
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "HTCondorComputeService::submitStandardJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }


    /**
     * @brief Asynchronously submit a pilot job to the cloud service
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void HTCondorComputeService::submitPilotJob(std::shared_ptr<PilotJob> job,
                                                const std::map<std::string, std::string> &service_specific_args) {
        serviceSanityCheck();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

        //  send a "run a pilot job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new ComputeServiceSubmitPilotJobRequestMessage(
                            answer_mailbox, job, service_specific_args,
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "ComputeService::submitPilotJob(): Received an unexpected [" + message->getName() + "] message!");
        }
    }

    /**
     * @brief Terminate a standard job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorComputeService::terminateStandardJob(std::shared_ptr<StandardJob> job) {
        throw std::runtime_error("HTCondorComputeService::terminateStandardJob(): Not implemented yet!");
    }

    /**
     * @brief Terminate a pilot job to the compute service
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorComputeService::terminatePilotJob(std::shared_ptr<PilotJob> job) {
        throw std::runtime_error("HTCondorComputeService::terminatePilotJob(): Not implemented yet!");
    }

    /**
     * @brief Get the service's local storage service
     * @return the local storage service object
     */
    std::shared_ptr<StorageService> HTCondorComputeService::getLocalStorageService() const {
        return this->local_storage_service;
    }

    /**
     * @brief Set the service's local storage service
     * @param local_storage_service: a storage service
     */
    void HTCondorComputeService::setLocalStorageService(std::shared_ptr<wrench::StorageService> local_storage_service) {
        this->local_storage_service = local_storage_service;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int HTCondorComputeService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);WRENCH_INFO(
                "HTCondor Service starting on host %s listening on mailbox_name %s", this->hostname.c_str(),
                this->mailbox_name.c_str());

        // start the central manager service
        this->central_manager->simulation = this->simulation;
        this->central_manager->start(this->central_manager, true, false); // Daemonized, no auto-restart


        // main loop
        while (this->processNextMessage()) {
            // no specific action
        }

        WRENCH_INFO("HTCondor Service on host %s cleanly terminating!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool HTCondorComputeService::processNextMessage() {
        // Wait for a message
        std::shared_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) { WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->terminate();
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                HTCondorComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
            processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
            processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Process a submit standard job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     * @param service_specific_args: service specific arguments
     *
     * @throw std::runtime_error
     */
    void HTCondorComputeService::processSubmitStandardJob(const std::string &answer_mailbox,
                                                          std::shared_ptr<StandardJob> job,
                                                          const std::map<std::string, std::string> &service_specific_args) {

        WRENCH_INFO("Asked to run a standard job with %ld tasks", job->getNumTasks());
        // Check that the job kind is supported
        if (not this->central_manager->jobKindIsSupported(job, service_specific_args)) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitStandardJobAnswerMessage(
                            job, this->getSharedPtr<HTCondorComputeService>(), false, std::shared_ptr<FailureCause>(
                                    new JobTypeNotSupported(job, this->getSharedPtr<HTCondorComputeService>())),
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        // Check that the job can run on some child service
        if (not this->central_manager->jobCanRunSomewhere(job, service_specific_args)) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitStandardJobAnswerMessage(
                            job, this->getSharedPtr<HTCondorComputeService>(), false, std::shared_ptr<FailureCause>(
                                    new NotEnoughResources(job, this->getSharedPtr<HTCondorComputeService>())),
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }


        // Submit the job to the central manager
        this->central_manager->submitStandardJob(job, service_specific_args);

        // send positive answer
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new
                        ComputeServiceSubmitStandardJobAnswerMessage(
                        job,
                        this->

                                getSharedPtr<HTCondorComputeService>(),

                        true, nullptr, this->
                                getMessagePayloadValue(
                                HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)
                ));
        return;
    }

/**
 * @brief Process a submit pilot job request
 *
 * @param answer_mailbox: the mailbox to which the answer message should be sent
 * @param job: the job
 * @param service_specific_args: service specific arguments
 *
 * @throw std::runtime_error
 */
    void HTCondorComputeService::processSubmitPilotJob(const std::string &answer_mailbox, std::shared_ptr<PilotJob> job,
                                                       const std::map<std::string, std::string> &service_specific_args) {

        WRENCH_INFO("Asked to run a pilot job");
        // Check that the job kind is supported
        if (not this->central_manager->jobKindIsSupported(job, service_specific_args)) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitPilotJobAnswerMessage(
                            job, this->getSharedPtr<HTCondorComputeService>(), false, std::shared_ptr<FailureCause>(
                                    new JobTypeNotSupported(job, this->getSharedPtr<HTCondorComputeService>())),
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        // Check that the job can run on some child service
        if (not this->central_manager->jobCanRunSomewhere(job, service_specific_args)) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitPilotJobAnswerMessage(
                            job, this->getSharedPtr<HTCondorComputeService>(), false, std::shared_ptr<FailureCause>(
                                    new NotEnoughResources(job, this->getSharedPtr<HTCondorComputeService>())),
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        // Submit the job to the central manager
        this->central_manager->submitPilotJob(job, service_specific_args);

        // send positive answer
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitPilotJobAnswerMessage(
                        job, this->getSharedPtr<HTCondorComputeService>(), true, nullptr, this->getMessagePayloadValue(
                                HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        return;
    }

    /**
     * @brief Terminate the daemon.
     */
    void HTCondorComputeService::terminate() {
        this->setStateToDown();
        this->central_manager->stop();
        this->default_property_values.clear();
        this->default_messagepayload_values.clear();
    }


}
