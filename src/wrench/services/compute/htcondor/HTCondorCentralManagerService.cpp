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
#include <wrench/services/compute/cloud/CloudComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/htcondor/HTCondorCentralManagerService.h>
#include <wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h>
#include <wrench/services/compute/htcondor/HTCondorNegotiatorService.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/failure_causes/NotEnoughResources.h>

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
     * @param fast_bmcs_resource_availability: whether to obtain direct availability information from bare-metal services
     * @param fcfs: whether to enforce FCFS scheduling of jobs
     * @param compute_services: a set of 'child' compute resources available to and via the HTCondor pool
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     */
    HTCondorCentralManagerService::HTCondorCentralManagerService(
            const std::string &hostname,
            double negotiator_startup_overhead,
            double grid_pre_overhead,
            double grid_post_overhead,
            double non_grid_pre_overhead,
            double non_grid_post_overhead,
            bool fast_bmcs_resource_availability,
            bool fcfs,
            std::set<shared_ptr<ComputeService>> compute_services,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list)
        : ComputeService(hostname, "htcondor_central_manager", "") {
        this->negotiator_startup_overhead = negotiator_startup_overhead;

        this->grid_pre_overhead = grid_pre_overhead;
        this->grid_post_overhead = grid_post_overhead;
        this->non_grid_pre_overhead = non_grid_pre_overhead;
        this->non_grid_post_overhead = non_grid_post_overhead;
        this->fast_bmcs_resource_availability = fast_bmcs_resource_availability;
        this->fcfs = fcfs;

        this->compute_services = compute_services;

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
    }

    /**
     * @brief Destructor
     */
    HTCondorCentralManagerService::~HTCondorCentralManagerService() = default;

    /**
     * @brief Add a new 'child' compute service
     *
     * @param compute_service: the compute service to add
     */
    void HTCondorCentralManagerService::addComputeService(std::shared_ptr<ComputeService> compute_service) {
        this->compute_services.insert(compute_service);
        //  send a "wake up" message to the daemon's commport
        this->commport->putMessage(
                new CentralManagerWakeUpMessage(0));
    }

    /**
     * @brief Submit a compound job to the HTCondor service
     *
     * @param job: a compound job
     * @param service_specific_args: service specific arguments
     *
     */
    void HTCondorCentralManagerService::submitCompoundJob(
            std::shared_ptr<CompoundJob> job,
            const std::map<std::string, std::string> &service_specific_args) {
        serviceSanityCheck();

        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
                new ComputeServiceSubmitCompoundJobRequestMessage(
                        answer_commport, job, service_specific_args,
                        this->getMessagePayloadValue(
                                HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ComputeServiceSubmitCompoundJobAnswerMessage>("HTCondorCentralManagerService::submitCompoundJob(): Received an");
        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int HTCondorCentralManagerService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);

        WRENCH_INFO("HTCondor Service starting on host %s listening on commport %s",
                    this->hostname.c_str(), this->commport->get_cname());

        // main loop
        while (this->processNextMessage()) {
            if (not this->dispatching_jobs && not this->resources_unavailable) {
                // dispatching standard or pilot jobs
                if (not this->pending_jobs.empty()) {
                    this->dispatching_jobs = true;
                    auto negotiator = std::make_shared<HTCondorNegotiatorService>(
                            this->hostname,
                            this->negotiator_startup_overhead,
                            this->grid_pre_overhead,
                            this->non_grid_pre_overhead,
                            this->fast_bmcs_resource_availability,
                            this->fcfs,
                            this->compute_services,
                            this->running_jobs, this->pending_jobs, this->commport);
                    negotiator->setSimulation(this->simulation_);
                    negotiator->start(negotiator, true, false);// Daemonized, no auto-restart
                }
            }
        }

        WRENCH_INFO("HTCondorCentralManager Service on host %s cleanly terminating!",
                    S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Construct a dict for resource information
     * @param key: the desired key
     * @return a dictionary
     */
    std::map<std::string, double> HTCondorCentralManagerService::constructResourceInformation(const std::string &key) {
        throw std::runtime_error("HTCondorComputeService::constructResourceInformation(): Not implemented in this service.");
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     */
    bool HTCondorCentralManagerService::processNextMessage() {
        // Wait for a message
        std::shared_ptr<SimulationMessage> message;

        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &e) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_INFO("HTCondor Central Manager got a [%s] message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            this->terminate();
            // This is Synchronous
            try {
                msg->ack_commport->putMessage(
                        new ServiceDaemonStoppedMessage(
                                this->getMessagePayloadValue(
                                        HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return false;
            }
            return false;

        } else if (std::dynamic_pointer_cast<CentralManagerWakeUpMessage>(message)) {
            // Do nothing
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceSubmitCompoundJobRequestMessage>(message)) {
            processSubmitCompoundJob(msg->answer_commport, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceCompoundJobDoneMessage>(message)) {
            if (HTCondorComputeService::isJobGridUniverse(msg->job)) {
                S4U_Simulation::sleep(this->grid_post_overhead);
            } else {
                S4U_Simulation::sleep(this->non_grid_post_overhead);
            }
            processCompoundJobCompletion(msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceCompoundJobFailedMessage>(message)) {
            if (HTCondorComputeService::isJobGridUniverse(msg->job)) {
                S4U_Simulation::sleep(this->grid_post_overhead);
            } else {
                S4U_Simulation::sleep(this->non_grid_post_overhead);
            }
            processCompoundJobFailure(msg->job);
            return true;

        } else if (auto msg = std::dynamic_pointer_cast<NegotiatorCompletionMessage>(message)) {
            processNegotiatorCompletion(msg->scheduled_jobs);
            return true;

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Process a submit compound job request
     *
     * @param answer_commport: the commport to which the answer message should be sent
     * @param job: the job
     * @param service_specific_args: service specific arguments
     *
     */
    void HTCondorCentralManagerService::processSubmitCompoundJob(
            S4U_CommPort *answer_commport, std::shared_ptr<CompoundJob> job,
            std::map<std::string, std::string> &service_specific_args) {
        this->pending_jobs.emplace_back(std::make_tuple(job, service_specific_args));
        this->resources_unavailable = false;

        answer_commport->dputMessage(
                new ComputeServiceSubmitCompoundJobAnswerMessage(
                        job, this->getSharedPtr<HTCondorCentralManagerService>(), true, nullptr,
                        this->getMessagePayloadValue(
                                HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD)));
    }

    //    /**
    //     * @brief Process a submit pilot job request
    //     *
    //     * @param answer_commport: the commport to which the answer message should be sent
    //     * @param job: the job
    //     * @param service_specific_args: service specific arguments
    //     *
    //     */
    //    void HTCondorCentralManagerService::processSubmitPilotJob(
    //            const std::string &answer_commport, std::shared_ptr <PilotJob> job,
    //            std::map <std::string, std::string> &service_specific_args) {
    //        this->pending_jobs.push_back(std::make_tuple(job, service_specific_args));
    //        this->resources_unavailable = false;
    //
    //        S4U_CommPort::dputMessage(
    //                answer_commport,
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
    //     */
    //    void HTCondorCentralManagerService::processPilotJobStarted(std::shared_ptr <PilotJob> job) {
    //        // Forward the notification
    //        S4U_CommPort::dputMessage(
    //                job->popCallbackCommPort(),
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
    //     */
    //    void HTCondorCentralManagerService::processPilotJobCompletion(std::shared_ptr <PilotJob> job) {
    //        // Forward the notification
    //        S4U_CommPort::dputMessage(
    //                job->popCallbackCommPort(),
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
     */
    void HTCondorCentralManagerService::processCompoundJobCompletion(const std::shared_ptr<CompoundJob> &job) {
        WRENCH_INFO("A compound job has completed: %s", job->getName().c_str());
        auto callback_commport = job->popCallbackCommPort();

        // Send the callback to the originator
        callback_commport->dputMessage(
                new ComputeServiceCompoundJobDoneMessage(
                        job, this->getSharedPtr<HTCondorCentralManagerService>(), this->getMessagePayloadValue(HTCondorCentralManagerServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD)));
        this->resources_unavailable = false;

        this->running_jobs.erase(job);
    }

    /**
     * @brief Process a compound job failure
     *
     * @param job: the job
     *
     */
    void HTCondorCentralManagerService::processCompoundJobFailure(const std::shared_ptr<CompoundJob> &job) {
        WRENCH_INFO("A compound job has failed: %s", job->getName().c_str());
        auto callback_commport = job->popCallbackCommPort();

        // Send the callback to the originator
        callback_commport->dputMessage(
                new ComputeServiceCompoundJobFailedMessage(
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
            std::set<std::shared_ptr<Job>> &scheduled_jobs) {
        if (scheduled_jobs.empty()) {
            this->resources_unavailable = true;
            this->dispatching_jobs = false;
            return;
        }

        for (const auto &sjob: scheduled_jobs) {
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

    //    /**
    //   * @brief Helper function to check whether a job kind is supported
    //   * @param job: the job
    //   * @param service_specific_arguments: the service-specific argument
    //   * @return true or false
    //   */
    //    bool HTCondorCentralManagerService::jobKindIsSupported(
    //            const std::shared_ptr<Job> &job,
    //            std::map<std::string, std::string> service_specific_arguments) {
    //        bool is_grid_universe =
    //                (service_specific_arguments.find("-universe") != service_specific_arguments.end()) and
    //                (service_specific_arguments["-universe"] == "grid");
    //        bool is_standard_job = (std::dynamic_pointer_cast<StandardJob>(job) != nullptr);
    //
    //        bool found_one = false;
    //        for (auto const &cs: this->compute_services) {
    //            if (is_grid_universe and (std::dynamic_pointer_cast<BatchComputeService>(cs) == nullptr)) {
    //                continue;
    //            }
    //            if ((not is_grid_universe) and (std::dynamic_pointer_cast<BareMetalComputeService>(cs) == nullptr)) {
    //                continue;
    //            }
    //            if (is_standard_job and (not cs->supportsStandardJobs())) {
    //                continue;
    //            }
    //            if ((not is_standard_job) and (not cs->supportsPilotJobs())) {
    //                continue;
    //            }
    //            found_one = true;
    //            break;
    //        }
    //
    //        return found_one;
    //    }

    /**
     * @brief Helper function to check whether a job can run on at least one child compute service
     * @param job: the job
     * @param service_specific_arguments: the service-specific argument
     * @return true or false
     */
    std::shared_ptr<FailureCause> HTCondorCentralManagerService::jobCanRunSomewhere(
            const std::shared_ptr<CompoundJob> &job,
            std::map<std::string, std::string> service_specific_arguments) {

        std::shared_ptr<FailureCause> failure_cause;
        bool there_is_a_grid_appropriate_service = false;

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
            if (((is_grid_universe and (std::dynamic_pointer_cast<BatchComputeService>(cs))) or
                 ((not is_grid_universe) and (std::dynamic_pointer_cast<BareMetalComputeService>(cs))))) {
                there_is_a_grid_appropriate_service = true;
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
                num_hosts = BatchComputeService::parseUnsignedLongServiceSpecificArgument(
                        "-N", service_specific_arguments);
                num_cores_per_host = BatchComputeService::parseUnsignedLongServiceSpecificArgument(
                        "-c", service_specific_arguments);
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

        if (not there_is_a_grid_appropriate_service) {
            std::string error_message;
            if (is_grid_universe) {
                error_message = "Grid universe jobs not supported";
            } else {
                error_message = "Non-grid universe jobs not supported";
            }
            return std::make_shared<NotAllowed>(this->getSharedPtr<Service>(), error_message);
        } else if (not found_one) {
            return std::make_shared<NotEnoughResources>(job, this->getSharedPtr<Service>());
        } else {
            return nullptr;
        }
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
