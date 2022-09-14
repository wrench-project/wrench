/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceProperty.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/failure_causes/NotEnoughResources.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/htcondor/HTCondorComputeService.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/services/compute/ComputeService.h>
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/failure_causes/JobKilled.h>

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
                                                   WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                   WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : ComputeService(hostname, "htcondor_service", "") {
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

#if 0
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

        // Determine if there is at least one BatchComputeService service
        bool at_least_one_batch_service = false;
        for (auto const &cs: compute_services) {
            if (std::dynamic_pointer_cast<BatchComputeService>(cs)) {
                at_least_one_batch_service = true;
                break;
            }
        }
#endif

        // create central manager service
        this->central_manager = std::make_shared<HTCondorCentralManagerService>(
                hostname,
                this->getPropertyValueAsTimeInSecond(HTCondorComputeServiceProperty::NEGOTIATOR_OVERHEAD),
                this->getPropertyValueAsTimeInSecond(HTCondorComputeServiceProperty::GRID_PRE_EXECUTION_DELAY),
                this->getPropertyValueAsTimeInSecond(HTCondorComputeServiceProperty::GRID_POST_EXECUTION_DELAY),
                this->getPropertyValueAsTimeInSecond(HTCondorComputeServiceProperty::NON_GRID_PRE_EXECUTION_DELAY),
                this->getPropertyValueAsTimeInSecond(HTCondorComputeServiceProperty::NON_GRID_POST_EXECUTION_DELAY),
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
     * @param compute_service: the compute service to add
     */
    void HTCondorComputeService::addComputeService(std::shared_ptr<ComputeService> compute_service) {
        this->central_manager->addComputeService(compute_service);
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
    void HTCondorComputeService::submitCompoundJob(std::shared_ptr<CompoundJob> job,
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
                                    HTCondorComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));
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
                    "HTCondorComputeService::submitCompoundJob(): Received an unexpected [" + message->getName() +
                    "] message!");
        }
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
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO(
                "HTCondor Service starting on host %s listening on mailbox_name %s", this->hostname.c_str(),
                this->mailbox->get_cname());

        // start the central manager service
        this->central_manager->setSimulation(this->simulation);
        this->central_manager->start(this->central_manager, true, false);// Daemonized, no auto-restart


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
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
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

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitCompoundJobRequestMessage *>(message.get())) {
            processSubmitCompoundJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
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
    void HTCondorComputeService::processSubmitCompoundJob(simgrid::s4u::Mailbox *answer_mailbox,
                                                          std::shared_ptr<CompoundJob> job,
                                                          const std::map<std::string, std::string> &service_specific_args) {
        WRENCH_INFO("Asked to run compound job %s, which has %ld actions", job->getName().c_str(), job->getActions().size());

        // Check that the job can run on some child service
        if (not this->central_manager->jobCanRunSomewhere(job, service_specific_args)) {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitCompoundJobAnswerMessage(
                            job, this->getSharedPtr<HTCondorComputeService>(), false, std::shared_ptr<FailureCause>(new NotEnoughResources(job, this->getSharedPtr<HTCondorComputeService>())),
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        // Check that, if the job is non-grid, then is has no service_specific args
        bool service_specific_args_valid = true;
        if (not service_specific_args.empty()) {
            if (service_specific_args.find("-universe") == service_specific_args.end()) {
                service_specific_args_valid = false;
            } else if (service_specific_args.at("-universe") != "grid") {
                service_specific_args_valid = false;
            }
        }

        if (not service_specific_args_valid) {
            std::string error_message = "Non-grid universe jobs submitted to HTCondor cannot have service-specific arguments";
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitCompoundJobAnswerMessage(
                            job, this->getSharedPtr<HTCondorComputeService>(), false, std::shared_ptr<FailureCause>(new NotAllowed(this->getSharedPtr<HTCondorComputeService>(), error_message)),
                            this->getMessagePayloadValue(
                                    HTCondorComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
        }

        this->central_manager->submitCompoundJob(job, service_specific_args);

        // send positive answer
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitCompoundJobAnswerMessage(
                        job,
                        this->getSharedPtr<HTCondorComputeService>(),
                        true, nullptr, this->getMessagePayloadValue(HTCondorComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
    }

    ///**
    // * @brief Process a submit pilot job request
    // *
    // * @param answer_mailbox: the mailbox to which the answer message should be sent
    // * @param job: the job
    // * @param service_specific_args: service specific arguments
    // *
    // * @throw std::runtime_error
    // */
    //    void HTCondorComputeService::processSubmitPilotJob(const std::string &answer_mailbox, std::shared_ptr<PilotJob> job,
    //                                                       const std::map<std::string, std::string> &service_specific_args) {
    //
    //        WRENCH_INFO("Asked to run a pilot job");
    //
    //        // Check that the job can run on some child service
    //        if (not this->central_manager->jobCanRunSomewhere(job, service_specific_args)) {
    //            S4U_Mailbox::dputMessage(
    //                    answer_mailbox,
    //                    new ComputeServiceSubmitPilotJobAnswerMessage(
    //                            job, this->getSharedPtr<HTCondorComputeService>(), false, std::shared_ptr<FailureCause>(
    //                                    new NotEnoughResources(job, this->getSharedPtr<HTCondorComputeService>())),
    //                            this->getMessagePayloadValue(
    //                                    HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
    //            return;
    //        }
    //
    //        // Submit the job to the central manager
    //        this->central_manager->submitPilotJob(job, service_specific_args);
    //
    //        // send positive answer
    //        S4U_Mailbox::dputMessage(
    //                answer_mailbox,
    //                new ComputeServiceSubmitPilotJobAnswerMessage(
    //                        job, this->getSharedPtr<HTCondorComputeService>(), true, nullptr, this->getMessagePayloadValue(
    //                                HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
    //        return;
    //    }


    /**
      * @brief Process a host available resource request
      * @param answer_mailbox: the answer mailbox
      * @param num_cores: the desired number of cores
      * @param ram: the desired RAM
      */
    void HTCondorComputeService::processIsThereAtLeastOneHostWithAvailableResources(simgrid::s4u::Mailbox *answer_mailbox, unsigned long num_cores, double ram) {
        throw std::runtime_error("HTCondorComputeService::processIsThereAtLeastOneHostWithAvailableResources(): A HTCondor service does not support this operation");
    }


    /**
     * @brief Terminate the daemon.
     */
    void HTCondorComputeService::terminate() {
        this->setStateToDown();
        this->central_manager->stop(true, ComputeService::TerminationCause::TERMINATION_JOB_KILLED);
        this->default_property_values.clear();
        this->default_messagepayload_values.clear();
    }

    /**
     * @brief Determine whether compute service has scratch
     *
     * @param service_specific_args: the service-specific arguments (useful for some services)
     */
    void HTCondorComputeService::validateJobsUseOfScratch(std::map<std::string, std::string> &service_specific_args) {
        throw std::invalid_argument("Jobs submitted to an HT-Condor compute service cannot make use of scratch");
    }

    /**
     * @brief Method the validates service-specific arguments (throws std::invalid_argument if invalid)
     * @param job: the job that's being submitted
     * @param service_specific_args: the service-specific arguments
     */
    void HTCondorComputeService::validateServiceSpecificArguments(std::shared_ptr<CompoundJob> job,
                                                                  std::map<std::string, std::string> &service_specific_args) {
        // Is the job a grid universe job
        bool grid_universe = false;
        if (service_specific_args.find("-universe") != service_specific_args.end()) {
            if (service_specific_args.at("-universe") != "grid") {
                throw std::invalid_argument("The value for the '-universe' key can only be 'grid'");
            }
            grid_universe = true;
        }

        if (not grid_universe) {
            if (not service_specific_args.empty()) {
                throw std::invalid_argument("A non-grid-universe job cannot have any service-specific arguments");
            }
            return;
        }

        // At this point, the job is a grid-universe job

        // Determine the target BatchComputeService compute service
        std::set<std::shared_ptr<BatchComputeService>> batch_cses;
        std::shared_ptr<BatchComputeService> target_cs = nullptr;
        for (auto const &cs: this->central_manager->compute_services) {
            if (auto batch_cs = std::dynamic_pointer_cast<BatchComputeService>(cs)) {
                batch_cses.insert(batch_cs);
            }
        }
        if (service_specific_args.find("-service") != service_specific_args.end()) {
            for (auto const &cs: batch_cses) {
                if (cs->getName() == service_specific_args.at("-service")) {
                    target_cs = cs;
                }
            }
        } else {
            if (batch_cses.size() != 1) {
                throw std::invalid_argument("'-service' service-specific argument required for grid-universe job as there are more than one BatchComputeService compute services available");
            } else {
                target_cs = *(batch_cses.begin());
            }
        }
        if (target_cs == nullptr) {
            throw std::invalid_argument("Not able to determine target BatchComputeService compute service for grid-universe job");
        }

        // Now, invoke the BatchComputeService compute service with the arguments (sort of) to validate them
        auto stripped_service_specific_args = service_specific_args;
        stripped_service_specific_args.erase("-universe");
        stripped_service_specific_args.erase("-service");// which may not be there, but whatever
        try {
            target_cs->validateServiceSpecificArguments(job, stripped_service_specific_args);
        } catch (ExecutionException &e) {
            throw;
        } catch (std::invalid_argument &e) {
            throw;
        }
    }

    /**
 * @brief Returns true if the service supports standard jobs
 * @return true or false
 */
    bool HTCondorComputeService::supportsStandardJobs() {
        return true;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool HTCondorComputeService::supportsCompoundJobs() {
        return true;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool HTCondorComputeService::supportsPilotJobs() {
        return true;
    }


}// namespace wrench
