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
     * @param compute_services: a set of 'child' compute resources available to and via the HTCondor pool
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    HTCondorCentralManagerService::HTCondorCentralManagerService(
            const std::string &hostname,
            std::set<shared_ptr<ComputeService>> compute_services,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list)
            : ComputeService(hostname, "htcondor_central_manager", "htcondor_central_manager", "") {

        this->compute_services = compute_services;

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
        this->compute_services.clear();
//        this->compute_resources_map.clear();
        this->running_jobs.clear();
    }

    /**
     * @brief Add a new 'child' compute service
     *
     * @param compute_service: the compute service to add
     */
     void HTCondorCentralManagerService::addComputeService(std::shared_ptr<ComputeService> compute_service) {
         this->compute_services.insert(compute_service);
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
            std::shared_ptr<StandardJob> job,
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
                                    HTCondorCentralManagerServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

        if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage*>(message.get())) {
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
    void HTCondorCentralManagerService::submitPilotJob(std::shared_ptr<PilotJob> job,
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
                                    HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

        if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage*>(message.get())) {
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
    void HTCondorCentralManagerService::terminateStandardJob(std::shared_ptr<StandardJob> job) {
        throw std::runtime_error("HTCondorCentralManagerService::terminateStandardJob(): Not implemented yet!");
    }

    /**
     * @brief Terminate a pilot job to the compute service
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::terminatePilotJob(std::shared_ptr<PilotJob> job) {
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

//        // start the compute resource services
//        try {
//            if(grid_universe_batch_service) {
//                this->grid_universe_batch_service_shared_ptr = this->simulation->startNewService(grid_universe_batch_service);
//                //WRENCH_INFO("starting service---> %p", grid_universe_batch_service_shared_ptr.get());
//            }
//            for (auto cs : this->compute_resources) {
//                auto cs_shared_ptr = this->simulation->startNewService(cs);
//
//                unsigned long sum_num_idle_cores = 0;
//
//                if (auto virtualized_cluster = dynamic_cast<VirtualizedClusterComputeService *>(cs)) {
//                    // for cloud services, we must create/start VMs, each of which
//                    // is a bare_metal. Right now, we greedily use the whole Cloud!
//
//                    for (auto const &host : virtualized_cluster->getExecutionHosts()) {
//                        unsigned long num_cores = Simulation::getHostNumCores(host);
//                        double ram = Simulation::getHostMemoryCapacity(host);
//                        auto vm_name = virtualized_cluster->createVM(num_cores, ram);
//                        auto vm_cs = virtualized_cluster->startVM(vm_name, host);
//                        // set the number of idle cores
//                        sum_num_idle_cores = vm_cs->getTotalNumIdleCores();
//                        this->compute_resources_map.insert(std::make_pair(vm_cs, sum_num_idle_cores));
//                    }
//
//                } else if (auto cloud = dynamic_cast<CloudComputeService *>(cs)) {
//                    throw std::runtime_error(
//                            "An HTCondorCentralManagerService cannot use a CloudComputeService - use a VirtualizedClusterComputeService instead");
//                } else {
//
//                    // set the number of available cores
//                    sum_num_idle_cores = cs->getTotalNumIdleCores();
//                    this->compute_resources_map.insert(std::make_pair(cs_shared_ptr, sum_num_idle_cores));
//                }
//
//            }
//        } catch (WorkflowExecutionException &e) {
//            throw std::runtime_error("Unable to acquire compute resources: " + e.getCause()->toString());
//        }

        // main loop
        while (this->processNextMessage()) {
            if (not this->dispatching_jobs && not this->resources_unavailable) {

                // dispatching standard or pilot jobs
                if (not this->pending_jobs.empty()) {

                    this->dispatching_jobs = true;
                    //WRENCH_INFO("adding batch service to new negotiator---> %p", this->grid_universe_batch_service_shared_ptr.get());
                    auto negotiator = std::shared_ptr<HTCondorNegotiatorService>(
                            new HTCondorNegotiatorService(this->hostname, this->compute_services,
                                                          this->running_jobs, this->pending_jobs, this->mailbox_name));
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

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage*>(message.get())) {
            this->terminate();
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(
                        msg->ack_mailbox,
                        new ServiceDaemonStoppedMessage(
                                this->getMessagePayloadValue(
                                        HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr <NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage*>(message.get())) {
            processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage*>(message.get())) {
            processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServicePilotJobStartedMessage*>(message.get())) {
            processPilotJobStarted(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage*>(message.get())) {
            processPilotJobCompletion(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceStandardJobDoneMessage*>(message.get())) {
            processStandardJobCompletion(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<NegotiatorCompletionMessage*>(message.get())) {
            processNegotiatorCompletion(msg->scheduled_jobs);
            return true;

        }else {
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
            const std::string &answer_mailbox, std::shared_ptr<StandardJob> job,
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
            const std::string &answer_mailbox, std::shared_ptr<PilotJob> job,
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
    void HTCondorCentralManagerService::processPilotJobStarted(std::shared_ptr<PilotJob> job) {
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
    void HTCondorCentralManagerService::processPilotJobCompletion(std::shared_ptr<PilotJob> job) {
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
    void HTCondorCentralManagerService::processStandardJobCompletion(std::shared_ptr<StandardJob> job) {
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
        this->running_jobs.erase(job);
    }

    /**
     * @brief Process a negotiator cycle completion
     *
     * @param scheduled_jobs: list of scheduled jobs upon negotiator cycle completion
     */
    void HTCondorCentralManagerService::processNegotiatorCompletion(
            std::vector<std::shared_ptr<WorkflowJob>> &scheduled_jobs) {

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
        this->compute_services.clear();
        this->pending_jobs.clear();
        this->running_jobs.clear();
    }

}
