/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/htcondor/HTCondorComputeServiceMessagePayload.h>
#include <wrench/services/compute/htcondor/HTCondorComputeService.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h>
#include <wrench/services/compute/cloud/CloudComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/htcondor/HTCondorCentralManagerService.h>
#include <wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h>
#include <wrench/services/compute/htcondor/HTCondorNegotiatorService.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/failure_causes/NetworkError.h>

#include <memory>


WRENCH_LOG_CATEGORY(wrench_core_HTCondorCentralManager,
                    "Log category for HTCondorCentralManagerService");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param negotiator_startup_overhead: negotiator startup overhead
     * @param grid_pre_overhead: grid job pre-overhead
     * @param grid_post_overhead: grid job post-overhead
     * @param non_grid_pre_overhead: non-grid job pre-overhead
     * @param non_grid_post_overhead: non-grid job post-overhead
     * @param compute_services: a set of 'child' compute resources available to and via the HTCondor pool
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    HTCondorCentralManagerService::HTCondorCentralManagerService(
            const std::string &hostname,
            double negotiator_startup_overhead,
            double grid_pre_overhead,
            double grid_post_overhead,
            double non_grid_pre_overhead,
            double non_grid_post_overhead,
            std::set<shared_ptr<ComputeService>> compute_services,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list)
        : ComputeService(hostname, "htcondor_central_manager", "") {
        this->negotiator_startup_overhead = negotiator_startup_overhead;

        this->grid_pre_overhead = grid_pre_overhead;
        this->grid_post_overhead = grid_post_overhead;
        this->non_grid_pre_overhead = non_grid_pre_overhead;
        this->non_grid_post_overhead = non_grid_post_overhead;

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
        this->running_jobs.clear();
    }

    /**
     * @brief Add a new 'child' compute service
     *
     * @param compute_service: the compute service to add
     */
    void HTCondorCentralManagerService::addComputeService(std::shared_ptr<ComputeService> compute_service) {
        this->compute_services.insert(compute_service);
        //  send a "wake up" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox,
                    new CentralManagerWakeUpMessage(0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }
    }

    /**
     * @brief Submit a compound job to the HTCondor service
     *
     * @param job: a compound job
     * @param service_specific_args: service specific arguments
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::submitCompoundJob(
            std::shared_ptr<CompoundJob> job,
            const std::map<std::string, std::string> &service_specific_args) {
        serviceSanityCheck();

        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();

        //  send a "run a standard job" message to the daemon's mailbox_name
        try {
            S4U_Mailbox::putMessage(
                    this->mailbox,
                    new ComputeServiceSubmitCompoundJobRequestMessage(
                            answer_mailbox, job, service_specific_args,
                            this->getMessagePayloadValue(
                                    HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        // Get the answer
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(answer_mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        if (auto msg = dynamic_cast<ComputeServiceSubmitCompoundJobAnswerMessage *>(message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "HTCondorCentralManagerService::submitCompoundJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int HTCondorCentralManagerService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("HTCondor Service starting on host %s listening on mailbox_name %s",
                    this->hostname.c_str(), this->mailbox->get_cname());

        // main loop
        while (this->processNextMessage()) {
            if (not this->dispatching_jobs && not this->resources_unavailable) {
                // dispatching standard or pilot jobs
                if (not this->pending_jobs.empty()) {
                    this->dispatching_jobs = true;
                    //WRENCH_INFO("adding BatchComputeService service to new negotiator---> %p", this->grid_universe_batch_service_shared_ptr.get());
                    auto negotiator = std::make_shared<HTCondorNegotiatorService>(
                            this->hostname,
                            this->negotiator_startup_overhead,
                            this->grid_pre_overhead,
                            this->non_grid_pre_overhead,
                            this->compute_services,
                            this->running_jobs, this->pending_jobs, this->mailbox);
                    negotiator->setSimulation(this->simulation);
                    negotiator->start(negotiator, true, false);// Daemonized, no auto-restart
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
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_INFO("HTCondor Central Manager got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
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

        } else if (auto msg = dynamic_cast<CentralManagerWakeUpMessage *>(message.get())) {
            // Do nothing
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitCompoundJobRequestMessage *>(message.get())) {
            processSubmitCompoundJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

            //        } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
            //            processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            //            return true;
            //
            //        } else if (auto msg = dynamic_cast<ComputeServicePilotJobStartedMessage *>(message.get())) {
            //            processPilotJobStarted(msg->job);
            //            return true;
            //
            //        } else if (auto msg = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
            //            processPilotJobCompletion(msg->job);
            //            return true;
            //
        } else if (auto msg = dynamic_cast<ComputeServiceCompoundJobDoneMessage *>(message.get())) {
            if (HTCondorComputeService::isJobGridUniverse(msg->job)) {
                S4U_Simulation::sleep(this->grid_post_overhead);
            } else {
                S4U_Simulation::sleep(this->non_grid_post_overhead);
            }
            processCompoundJobCompletion(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceCompoundJobFailedMessage *>(message.get())) {
            if (HTCondorComputeService::isJobGridUniverse(msg->job)) {
                S4U_Simulation::sleep(this->grid_post_overhead);
            } else {
                S4U_Simulation::sleep(this->non_grid_post_overhead);
            }
            processCompoundJobFailure(msg->job);
            return true;

        } else if (auto msg = dynamic_cast<NegotiatorCompletionMessage *>(message.get())) {
            processNegotiatorCompletion(msg->scheduled_jobs);
            return true;

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Process a submit compound job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     * @param service_specific_args: service specific arguments
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::processSubmitCompoundJob(
            simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<CompoundJob> job,
            std::map<std::string, std::string> &service_specific_args) {
        this->pending_jobs.emplace_back(std::make_tuple(job, service_specific_args));
        this->resources_unavailable = false;

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitCompoundJobAnswerMessage(
                        job, this->getSharedPtr<HTCondorCentralManagerService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
    }

    //    /**
    //     * @brief Process a submit pilot job request
    //     *
    //     * @param answer_mailbox: the mailbox to which the answer message should be sent
    //     * @param job: the job
    //     * @param service_specific_args: service specific arguments
    //     *
    //     * @throw std::runtime_error
    //     */
    //    void HTCondorCentralManagerService::processSubmitPilotJob(
    //            const std::string &answer_mailbox, std::shared_ptr <PilotJob> job,
    //            std::map <std::string, std::string> &service_specific_args) {
    //        this->pending_jobs.push_back(std::make_tuple(job, service_specific_args));
    //        this->resources_unavailable = false;
    //
    //        S4U_Mailbox::dputMessage(
    //                answer_mailbox,
    //                new ComputeServiceSubmitPilotJobAnswerMessage(
    //                        job, this->getSharedPtr<HTCondorCentralManagerService>(), true, nullptr,
    //                        this->getMessagePayloadValue(
    //                                HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
    //    }

    //    /**
    //     * @brief Process a pilot job started event
    //     *
    //     * @param job: the pilot job
    //     *
    //     * @throw std::runtime_error
    //     */
    //    void HTCondorCentralManagerService::processPilotJobStarted(std::shared_ptr <PilotJob> job) {
    //        // Forward the notification
    //        S4U_Mailbox::dputMessage(
    //                job->popCallbackMailbox(),
    //                new ComputeServicePilotJobStartedMessage(
    //                        job, this->getSharedPtr<HTCondorCentralManagerService>(),
    //                        this->getMessagePayloadValue(
    //                                HTCondorComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD)));
    //    }

    //    /**
    //     * @brief Process a pilot job completion
    //     *
    //     * @param job: the pilot job
    //     *
    //     * @throw std::runtime_error
    //     */
    //    void HTCondorCentralManagerService::processPilotJobCompletion(std::shared_ptr <PilotJob> job) {
    //        // Forward the notification
    //        S4U_Mailbox::dputMessage(
    //                job->popCallbackMailbox(),
    //                new ComputeServicePilotJobExpiredMessage(
    //                        job, this->getSharedPtr<HTCondorCentralManagerService>(),
    //                        this->getMessagePayloadValue(
    //                                HTCondorComputeServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD)));
    //    }

    /**
     * @brief Process a compound job completion
     *
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::processCompoundJobCompletion(std::shared_ptr<CompoundJob> job) {
        WRENCH_INFO("A compound job has completed: %s", job->getName().c_str());
        auto callback_mailbox = job->popCallbackMailbox();

        // Send the callback to the originator
        S4U_Mailbox::dputMessage(
                callback_mailbox, new ComputeServiceCompoundJobDoneMessage(
                                          job, this->getSharedPtr<HTCondorCentralManagerService>(), this->getMessagePayloadValue(HTCondorCentralManagerServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD)));
        this->resources_unavailable = false;

        this->running_jobs.erase(job);
    }

    /**
     * @brief Process a compound job failure
     *
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void HTCondorCentralManagerService::processCompoundJobFailure(std::shared_ptr<CompoundJob> job) {
        WRENCH_INFO("A compound job has failed: %s", job->getName().c_str());
        auto callback_mailbox = job->popCallbackMailbox();

        // Send the callback to the originator
        S4U_Mailbox::dputMessage(
                callback_mailbox, new ComputeServiceCompoundJobFailedMessage(
                                          job, this->getSharedPtr<HTCondorCentralManagerService>(),
                                          this->getMessagePayloadValue(HTCondorCentralManagerServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD)));
        this->resources_unavailable = false;

        this->running_jobs.erase(job);
    }

    /**
     * @brief Process a negotiator cycle completion
     *
     * @param scheduled_jobs: list of scheduled jobs upon negotiator cycle completion
     */
    void HTCondorCentralManagerService::processNegotiatorCompletion(
            std::vector<std::shared_ptr<Job>> &scheduled_jobs) {
        if (scheduled_jobs.empty()) {
            this->resources_unavailable = true;
            this->dispatching_jobs = false;
            return;
        }

        for (auto sjob: scheduled_jobs) {
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

    /**
   * @brief Helper function to check whether a job kind is supported
   * @param job: the job
   * @param service_specific_arguments: the service-specific argument
   * @return true or false
   */
    bool HTCondorCentralManagerService::jobKindIsSupported(
            const std::shared_ptr<Job> &job,
            std::map<std::string, std::string> service_specific_arguments) {
        bool is_grid_universe =
                (service_specific_arguments.find("-universe") != service_specific_arguments.end()) and
                (service_specific_arguments["-universe"] == "grid");
        bool is_standard_job = (std::dynamic_pointer_cast<StandardJob>(job) != nullptr);

        bool found_one = false;
        for (auto const &cs: this->compute_services) {
            if (is_grid_universe and (std::dynamic_pointer_cast<BatchComputeService>(cs) == nullptr)) {
                continue;
            }
            if ((not is_grid_universe) and (std::dynamic_pointer_cast<BareMetalComputeService>(cs) == nullptr)) {
                continue;
            }
            if (is_standard_job and (not cs->supportsStandardJobs())) {
                continue;
            }
            if ((not is_standard_job) and (not cs->supportsPilotJobs())) {
                continue;
            }
            found_one = true;
            break;
        }

        return found_one;
    }

    /**
     * @brief Helper function to check whether a job can run on at least one child compute service
     * @param job: the job
     * @param service_specific_arguments: the service-specific argument
     * @return true or false
     */
    bool HTCondorCentralManagerService::jobCanRunSomewhere(
            std::shared_ptr<CompoundJob> job,
            std::map<std::string, std::string> service_specific_arguments) {
        bool is_grid_universe =
                (service_specific_arguments.find("-universe") != service_specific_arguments.end()) and
                (service_specific_arguments["-universe"] == "grid");
        bool is_standard_job = (std::dynamic_pointer_cast<StandardJob>(job) != nullptr);

        bool found_one = false;
        for (auto const &cs: this->compute_services) {
            // Check for type appropriateness
            if (is_grid_universe and (std::dynamic_pointer_cast<BatchComputeService>(cs) == nullptr)) {
                continue;
            }
            if ((not is_grid_universe) and (std::dynamic_pointer_cast<BareMetalComputeService>(cs) == nullptr)) {
                continue;
            }

            // Check for resources for a grid universe job
            if (is_grid_universe) {
                if (service_specific_arguments.find("-N") == service_specific_arguments.end()) {
                    throw std::invalid_argument(
                            "HTCondorCentralManagerService::jobCanRunSomewhere(): Grid universe job must have a '-N' service-specific argument");
                }
                if (service_specific_arguments.find("-c") == service_specific_arguments.end()) {
                    throw std::invalid_argument(
                            "HTCondorCentralManagerService::jobCanRunSomewhere(): Grid universe job must have a '-c' service-specific argument");
                }
                unsigned long num_hosts = 0;
                unsigned long num_cores_per_host = 0;
                try {
                    num_hosts = BatchComputeService::parseUnsignedLongServiceSpecificArgument(
                            "-N", service_specific_arguments);
                    num_cores_per_host = BatchComputeService::parseUnsignedLongServiceSpecificArgument(
                            "-c", service_specific_arguments);
                } catch (std::invalid_argument &e) {
                    throw;
                }
                if (cs->getNumHosts() < num_hosts) {
                    continue;
                }
                if ((*(cs->getPerHostNumCores().begin())).second < num_cores_per_host) {
                    continue;
                }
            }

            if (!is_grid_universe) {
                auto core_resources = cs->getPerHostNumCores();
                unsigned long max_cores = 0;
                for (auto const &entry: core_resources) {
                    max_cores = std::max<unsigned long>(max_cores, entry.second);
                }
                if (max_cores < job->getMinimumRequiredNumCores()) {
                    continue;
                }
                auto ram_resources = cs->getMemoryCapacity();
                double max_ram = 0;
                for (auto const &entry: ram_resources) {
                    max_ram = std::max<double>(max_ram, entry.second);
                }
                if (max_ram < job->getMinimumRequiredMemory()) {
                    continue;
                }
            }

            found_one = true;
            break;
        }

        return found_one;
    }

    /**
  * @brief Returns true if the service supports standard jobs
  * @return true or false
  */
    bool HTCondorCentralManagerService::supportsStandardJobs() {
        return true;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool HTCondorCentralManagerService::supportsCompoundJobs() {
        return true;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool HTCondorCentralManagerService::supportsPilotJobs() {
        return false;
    }


}// namespace wrench
