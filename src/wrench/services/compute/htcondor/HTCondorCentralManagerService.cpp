/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/htcondor/HTCondorComputeServiceMessagePayload.h>
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/services/compute/htcondor/HTCondorCentralManagerService.h"
#include "wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h"
#include "wrench/services/compute/htcondor/HTCondorNegotiatorService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include <wrench/workflow/failure_causes/NetworkError.h>


WRENCH_LOG_CATEGORY(wrench_core_HTCondorCentralManager, "Log category for HTCondorCentralManagerService");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param compute_resources: a set of compute resources available via the HTCondor pool
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    HTCondorCentralManagerService::HTCondorCentralManagerService(
            const std::string &hostname,
            std::set<ComputeService *> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list)
            : ComputeService(hostname, "htcondor_central_manager", "htcondor_central_manager", "") {

        // Check compute_resource viability
        if (compute_resources.empty()) {
            throw std::invalid_argument("At least one compute service should be provided");
        }
        for (auto const &cs : compute_resources) {
            if (dynamic_cast<CloudComputeService *>(cs) and
                (not dynamic_cast<VirtualizedClusterComputeService *>(cs))) {
                throw std::invalid_argument(
                        "An HTCondorCentralManagerService cannot use a CloudComputeService - use a VirtualizedClusterComputeService instead");
            }
        }

        this->compute_resources = compute_resources;

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
    }

    /**
     * @brief Destructor
     */
    HTCondorCentralManagerService::~HTCondorCentralManagerService() {
        this->pending_jobs.clear();
        this->compute_resources.clear();
        this->compute_resources_map.clear();
        this->running_jobs.clear();
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
    void HTCondorCentralManagerService::submitStandardJob(
            StandardJob *job,
            std::map<std::string, std::string> &service_specific_args) {

        serviceSanityCheck();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

        //  send a "run a standard job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new ComputeServiceSubmitStandardJobRequestMessage(
                            answer_mailbox, job, service_specific_args,
                            this->getMessagePayloadValue(
                                    HTCondorCentralManagerServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
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
    void HTCondorCentralManagerService::submitPilotJob(PilotJob *job,
                                                       std::map<std::string, std::string> &service_specific_args) {
        serviceSanityCheck();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

        //  send a "run a pilot job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox_name,
                    new ComputeServiceSubmitPilotJobRequestMessage(
                            answer_mailbox, job, service_specific_args,
                            this->getMessagePayloadValue(
                                    HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

        if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitPilotJobAnswerMessage>(message)) {
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
    void HTCondorCentralManagerService::terminateStandardJob(StandardJob *job) {
        throw std::runtime_error("HTCondorCentralManagerService::terminateStandardJob(): Not implemented yet!");
    }

    /**
     * @brief Terminate a pilot job to the compute service
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::terminatePilotJob(PilotJob *job) {
        throw std::runtime_error("HTCondorCentralManagerService::terminatePilotJob(): Not implemented yet!");
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int HTCondorCentralManagerService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("HTCondor Service starting on host %s listening on mailbox_name %s",
                    this->hostname.c_str(), this->mailbox_name.c_str());

        // start the compute resource services
        try {
            for (auto cs : this->compute_resources) {
                auto cs_shared_ptr = this->simulation->startNewService(cs);

                unsigned long sum_num_idle_cores = 0;

                if (auto virtualized_cluster = dynamic_cast<VirtualizedClusterComputeService *>(cs)) {
                    // for cloud services, we must create/start VMs, each of which
                    // is a BareMetalComputeService. Right now, we greedily use the whole Cloud!

                    for (auto const &host : virtualized_cluster->getExecutionHosts()) {
                        unsigned long num_cores = Simulation::getHostNumCores(host);
                        double ram = Simulation::getHostMemoryCapacity(host);
                        auto vm_name = virtualized_cluster->createVM(num_cores, ram);
                        auto vm_cs = virtualized_cluster->startVM(vm_name, host);
                        // set the number of idle cores
                        sum_num_idle_cores = vm_cs->getTotalNumIdleCores();
                        this->compute_resources_map.insert(std::make_pair(vm_cs, sum_num_idle_cores));
                    }

                } else if (auto cloud = dynamic_cast<CloudComputeService *>(cs)) {
                    throw std::runtime_error(
                            "An HTCondorCentralManagerService cannot use a CloudComputeService - use a VirtualizedClusterComputeService instead");
                } else {

                    // set the number of available cores
                    sum_num_idle_cores = cs->getTotalNumIdleCores();
                    this->compute_resources_map.insert(std::make_pair(cs_shared_ptr, sum_num_idle_cores));
                }

            }
        } catch (WorkflowExecutionException &e) {
            throw std::runtime_error("Unable to acquire compute resources: " + e.getCause()->toString());
        }

        // main loop
        while (this->processNextMessage()) {
            if (not this->dispatching_jobs && not this->resources_unavailable) {

                // dispatching standard or pilot jobs
                if (not this->pending_jobs.empty()) {

                    this->dispatching_jobs = true;
                    auto negotiator = std::shared_ptr<HTCondorNegotiatorService>(
                            new HTCondorNegotiatorService(this->hostname, this->compute_resources_map,
                                                          this->running_jobs,
                                                          this->pending_jobs, this->mailbox_name));
                    negotiator->simulation = this->simulation;
                    negotiator->start(negotiator, true, false); // Daemonized, no auto-restart

                }
            }
        }

        WRENCH_INFO("HTCondorCentralManager Service on host %s cleanly terminating!",
                    S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool HTCondorCentralManagerService::processNextMessage() {
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

        WRENCH_DEBUG("HTCondor Central Manager got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            this->terminate();
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(
                        msg->ack_mailbox,
                        new ServiceDaemonStoppedMessage(
                                this->getMessagePayloadValue(
                                        HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitStandardJobRequestMessage>(message)) {
            processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitPilotJobRequestMessage>(message)) {
            processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServicePilotJobStartedMessage>(message)) {
            processPilotJobStarted(msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServicePilotJobExpiredMessage>(message)) {
            processPilotJobCompletion(msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceStandardJobDoneMessage>(message)) {
            processStandardJobCompletion(msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<NegotiatorCompletionMessage>(message)) {
            processNegotiatorCompletion(msg->scheduled_jobs);
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
    void HTCondorCentralManagerService::processSubmitStandardJob(
            const std::string &answer_mailbox, StandardJob *job,
            std::map<std::string, std::string> &service_specific_args) {

        this->pending_jobs.push_back(std::make_tuple(job, service_specific_args));
        this->resources_unavailable = false;

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitStandardJobAnswerMessage(
                        job, this->getSharedPtr<HTCondorCentralManagerService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                HTCondorCentralManagerServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
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
    void HTCondorCentralManagerService::processSubmitPilotJob(
            const std::string &answer_mailbox, PilotJob *job,
            std::map<std::string, std::string> &service_specific_args) {

        this->pending_jobs.push_back(std::make_tuple(job, service_specific_args));
        this->resources_unavailable = false;

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitPilotJobAnswerMessage(
                        job, this->getSharedPtr<HTCondorCentralManagerService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Process a pilot job started event
     *
     * @param job: the pilot job
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::processPilotJobStarted(wrench::PilotJob *job) {
        // Forward the notification
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServicePilotJobStartedMessage(
                                         job, this->getSharedPtr<HTCondorCentralManagerService>(),
                                         this->getMessagePayloadValue(
                                                 HTCondorComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Process a pilot job completion
     *
     * @param job: the pilot job
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::processPilotJobCompletion(wrench::PilotJob *job) {
        // Forward the notification
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(
                                         job, this->getSharedPtr<HTCondorCentralManagerService>(),
                                         this->getMessagePayloadValue(
                                                 HTCondorComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Process a standard job completion
     *
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::processStandardJobCompletion(StandardJob *job) {
        WRENCH_INFO("A standard job has completed: %s", job->getName().c_str());
        std::string callback_mailbox = job->popCallbackMailbox();

        for (auto task : job->getTasks()) {

            WRENCH_INFO("    Task completed: %s (%s)", task->getID().c_str(), callback_mailbox.c_str());
        }

        // Send the callback to the originator
        S4U_Mailbox::dputMessage(
                callback_mailbox, new ComputeServiceStandardJobDoneMessage(
                        job, this->getSharedPtr<HTCondorCentralManagerService>(), this->getMessagePayloadValue(
                                HTCondorCentralManagerServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD)));
        this->resources_unavailable = false;

        auto cs = this->running_jobs.find(job);
        auto cs_map = this->compute_resources_map.find(cs->second);
        cs_map->second = cs_map->second + job->getMinimumRequiredNumCores();
        this->running_jobs.erase(job);
    }

    /**
     * @brief Process a negotiator cycle completion
     *
     * @param scheduled_jobs: list of scheduled jobs upon negotiator cycle completion
     */
    void HTCondorCentralManagerService::processNegotiatorCompletion(
            std::vector<wrench::WorkflowJob *> &scheduled_jobs) {

        if (scheduled_jobs.empty()) {
            this->resources_unavailable = true;
            this->dispatching_jobs = false;
            return;
        }

        for (auto sjob : scheduled_jobs) {
            for (auto it = this->pending_jobs.begin(); it != this->pending_jobs.end(); ++it) {
                auto pjob = std::get<0>(*it);
                if (sjob == pjob) {
                    this->pending_jobs.erase(it);
                    break;
                }
            }
        }

        this->dispatching_jobs = false;
    }

    /**
     * @brief Terminate the daemon.
     */
    void HTCondorCentralManagerService::terminate() {
        this->setStateToDown();
        for (auto cs : this->compute_resources) {
            cs->stop();
        }
        this->compute_resources.clear();
        this->compute_resources_map.clear();
        this->pending_jobs.clear();
        this->running_jobs.clear();
    }

}
