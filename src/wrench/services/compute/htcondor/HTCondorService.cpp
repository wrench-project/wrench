/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/htcondor/HTCondorService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(HTCondor, "Log category for HTCondorService Scheduler");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param pool_name: HTCondor pool name
     * @param compute_resources: a set of compute resources available via the HTCondor pool
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    HTCondorService::HTCondorService(const std::string &hostname,
                                     const std::string &pool_name,
                                     std::set<ComputeService *> compute_resources,
                                     std::map<std::string, std::string> property_list,
                                     std::map<std::string, double> messagepayload_list) :
            ComputeService(hostname, "htcondor_service", "htcondor_service", 100) {

        if (pool_name.empty()) {
            throw std::runtime_error("A pool name for the HTCondor service should be provided.");
        }
        this->pool_name = pool_name;

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        // create central manager service
        this->central_manager = std::make_shared<HTCondorCentralManagerService>(hostname, compute_resources,
                                                                                property_list, messagepayload_list);
    }

    /**
     * @brief Destructor
     */
    HTCondorService::~HTCondorService() {
        this->central_manager = nullptr;
        this->default_property_values.clear();
        this->default_messagepayload_values.clear();
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
    void HTCondorService::submitStandardJob(StandardJob *job,
                                            std::map<std::string, std::string> &service_specific_args) {

        serviceSanityCheck();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

        //  send a "run a standard job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new ComputeServiceSubmitStandardJobRequestMessage(
                            answer_mailbox, job, service_specific_args,
                            this->SgetMessagePayloadValue(
                                    HTCondorServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Get the answer
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitStandardJobAnswerMessage>(message)) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "ComputeService::submitStandardJob(): Received an unexpected [" + message->getName() +
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
    void HTCondorService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_args) {
        throw std::runtime_error("HTCondorService::terminateStandardJob(): Not implemented yet!");
    }

    /**
     * @brief Terminate a standard job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorService::terminateStandardJob(StandardJob *job) {
        throw std::runtime_error("HTCondorService::terminateStandardJob(): Not implemented yet!");
    }

    /**
     * @brief Terminate a pilot job to the compute service
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorService::terminatePilotJob(PilotJob *job) {
        throw std::runtime_error("HTCondorService::terminatePilotJob(): Not implemented yet!");
    }

    /**
     * @brief Get the service's local storage service
     * @return the local storage service object
     */
    std::shared_ptr<StorageService> HTCondorService::getLocalStorageService() const {
        return this->local_storage_service;
    }

    /**
     * @brief Set the service's local storage service
     * @param local_storage_service: a storage service
     */
    void HTCondorService::setLocalStorageService(std::shared_ptr<wrench::StorageService> local_storage_service) {
        this->local_storage_service = local_storage_service;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int HTCondorService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("HTCondor Service starting on host %s listening on mailbox_name %s", this->hostname.c_str(),
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
    bool HTCondorService::processNextMessage() {
        // Wait for a message
        std::shared_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            this->terminate();
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                HTCondorServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitStandardJobRequestMessage>(message)) {
            processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
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
    void HTCondorService::processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                                   std::map<std::string, std::string> &service_specific_args) {

        WRENCH_INFO("Asked to run a standard job with %ld tasks", job->getNumTasks());
        if (not this->supportsStandardJobs()) {
//            try {
                S4U_Mailbox::dputMessage(
                        answer_mailbox,
                        new ComputeServiceSubmitStandardJobAnswerMessage(
                                job, this->getSharedPtr<HTCondorService>(), false, std::shared_ptr<FailureCause>(
                                        new JobTypeNotSupported(job, this->getSharedPtr<HTCondorService>())),
                                this->getMessagePayloadValue(
                                        HTCondorServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
//            } catch (std::shared_ptr<NetworkError> &cause) {
//                return;
//            }
            return;
        }

        this->central_manager->submitStandardJob(job, service_specific_args);

        // send positive answer
//        try {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitStandardJobAnswerMessage(
                            job, this->getSharedPtr<HTCondorService>(), true, nullptr, this->getMessagePayloadValue(
                                    HTCondorServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
//            return;
//        } catch (std::shared_ptr<NetworkError> &cause) {
//            return;
//        }
    }

    /**
     * @brief Terminate the daemon.
     */
    void HTCondorService::terminate() {
        this->setStateToDown();
        this->central_manager->stop();
        this->default_property_values.clear();
        this->default_messagepayload_values.clear();
    }
}
