/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h>
#include <wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessagePayload.h>
#include <wrench/services/compute/htcondor/HTCondorNegotiatorService.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simgrid_S4U_util/S4U_Simulation.h>
#include <wrench/job/PilotJob.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/failure_causes/NetworkError.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/compute/htcondor/HTCondorComputeService.h>

WRENCH_LOG_CATEGORY(wrench_core_htcondor_negotiator, "Log category for HTCondorNegotiator");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param startup_overhead: a startup overhead, in seconds
     * @param grid_pre_overhead: a pre-job overhead for grid jobs, in seconds
     * @param non_grid_pre_overhead: a pre-job overhead for non-grid jobs, in seconds
     * @param compute_services: a set of 'child' compute services available to and via the HTCondor pool
     * @param running_jobs: a list of currently running jobs
     * @param pending_jobs: a list of pending jobs
     * @param reply_mailbox: the mailbox to which the "done/failed" message should be sent
     */
    HTCondorNegotiatorService::HTCondorNegotiatorService(
            std::string &hostname,
            double startup_overhead,
            double grid_pre_overhead,
            double non_grid_pre_overhead,
            std::set<std::shared_ptr<ComputeService>> &compute_services,
            std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>> &running_jobs,
            std::vector<std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>>> &pending_jobs,
            simgrid::s4u::Mailbox *reply_mailbox)
            : Service(hostname, "htcondor_negotiator"), reply_mailbox(reply_mailbox),
              compute_services(compute_services), running_jobs(running_jobs), pending_jobs(pending_jobs) {
        this->startup_overhead = startup_overhead;
        this->grid_pre_overhead = grid_pre_overhead;
        this->non_grid_pre_overhead = non_grid_pre_overhead;
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
    }

    /**
     * @brief Destructor
     */
    HTCondorNegotiatorService::~HTCondorNegotiatorService() {
        this->pending_jobs.clear();
    }

    /**
     * @brief Compare the priority between two jobs
     *
     * @param lhs: pointer to a job
     * @param rhs: pointer to a job
     *
     * @return whether the priority of the left-hand-side workflow job is lower
     */
    bool HTCondorNegotiatorService::JobPriorityComparator::operator()(
            std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>> &lhs,
            std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>> &rhs) {
        return std::get<0>(lhs)->getPriority() < std::get<0>(rhs)->getPriority();
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int HTCondorNegotiatorService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);

        WRENCH_INFO("HTCondor Negotiator Service starting on host %s listening on mailbox_name %s",
                    this->hostname.c_str(), this->mailbox->get_cname());

        std::vector<std::shared_ptr<Job>> scheduled_jobs;

        // Simulate some overhead
        S4U_Simulation::sleep(this->startup_overhead);

        // sort jobs by priority
        std::sort(this->pending_jobs.begin(), this->pending_jobs.end(), JobPriorityComparator());

        // Go through the jobs and schedule them if possible
        for (auto entry: this->pending_jobs) {
            auto job = std::get<0>(entry);
            auto service_specific_arguments = std::get<1>(entry);
            bool is_standard_job = (std::dynamic_pointer_cast<CompoundJob>(job) != nullptr);

            auto target_compute_service = pickTargetComputeService(job, service_specific_arguments);

            if (target_compute_service) {
                job->pushCallbackMailbox(this->reply_mailbox);

                if (HTCondorComputeService::isJobGridUniverse(job)) {
                    S4U_Simulation::sleep(this->grid_pre_overhead);
                } else {
                    S4U_Simulation::sleep(this->non_grid_pre_overhead);
                }
                target_compute_service->submitCompoundJob(job, service_specific_arguments);
                this->running_jobs.insert(std::make_pair(job, target_compute_service));
                scheduled_jobs.push_back(job);
            }
        }

        // Send the callback to the originator
        try {
            S4U_Mailbox::putMessage(
                    this->reply_mailbox, new NegotiatorCompletionMessage(
                            scheduled_jobs, this->getMessagePayloadValue(
                                    HTCondorCentralManagerServiceMessagePayload::HTCONDOR_NEGOTIATOR_DONE_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            return 1;
        }

        WRENCH_INFO("HTCondorNegotiator Service on host %s cleanly terminating!",
                    S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Helper method to pick a target compute service for a job
     * @param job
     * @param service_specific_arguments
     * @return
     */
    std::shared_ptr<ComputeService> HTCondorNegotiatorService::pickTargetComputeService(
            std::shared_ptr<CompoundJob> job,
            std::map<std::string, std::string> service_specific_arguments) {

        if (HTCondorComputeService::isJobGridUniverse(job)) {
            return pickTargetComputeServiceGridUniverse(job, service_specific_arguments);
        } else {
            return pickTargetComputeServiceNonGridUniverse(job, service_specific_arguments);
        }
    }

    /**
 * @brief Helper method to pick a target compute service for a job for a Grid universe job
 * @param job: job to run
 * @param service_specific_arguments: service-specific arguments
 * @return
 */
    std::shared_ptr<ComputeService> HTCondorNegotiatorService::pickTargetComputeServiceGridUniverse(
            std::shared_ptr<CompoundJob> job, std::map<std::string,
            std::string>
    service_specific_arguments) {
        std::set<std::shared_ptr<BatchComputeService>> available_batch_compute_services;

        // Figure out which BatchComputeService compute services are available
        for (auto const &cs: this->compute_services) {
            if (auto batch_cs = std::dynamic_pointer_cast<BatchComputeService>(cs)) {
                available_batch_compute_services.insert(batch_cs);
            }
        }

        // If none, then grid universe jobs are not allowed
        if (available_batch_compute_services.empty()) {
            throw std::invalid_argument(
                    "HTCondorNegotiatorService::pickTargetComputeServiceGridUniverse(): A grid universe job was submitted, "
                    "but no BatchComputeService is available to the HTCondorComputeService");
        }

        // -N and -c service-specific arguments are required
        if ((service_specific_arguments.find("-N") == service_specific_arguments.end()) or
            (service_specific_arguments.find("-c") == service_specific_arguments.end())) {
            throw std::invalid_argument(
                    "HTCondorNegotiatorService::pickTargetComputeServiceGridUniverse(): A grid universe job must provide -N and -c service-specific arguments");
        }

        // -service service-specific arguments may be required and should point to an existing servuce
        if (service_specific_arguments.find("-service") == service_specific_arguments.end()) {
            if (available_batch_compute_services.size() == 1) {
                service_specific_arguments["-service"] = (*available_batch_compute_services.begin())->getName();
            } else {
                throw std::invalid_argument(
                        "HTCondorNegotiatorService::pickTargetComputeServiceGridUniverse(): a grid universe job must provide a -service service-specific argument since "
                        "the HTCondorComputeService has more than one available BatchComputeService");
            }
        }

        // Find the target BatchComputeService compute service
        std::shared_ptr<BatchComputeService> target_batch_cs = nullptr;
        for (auto const &batch_cs: available_batch_compute_services) {
            if (batch_cs->getName() == service_specific_arguments["-service"]) {
                target_batch_cs = batch_cs;
                break;
            }
        }
        if (target_batch_cs == nullptr) {
            throw std::invalid_argument("HTCondorNegotiatorService::pickTargetComputeServiceGridUniverse(): "
                                        "-service service-specific argument specifies a BatchComputeService compute service named '" +
                                        service_specific_arguments["-service"] +
                                        "', but no such service is known to the HTCondorComputeService");
        }

        return target_batch_cs;
    }


    /**
 * @brief Helper method to pick a target compute service for a job for a Non-Grid universe job
 * @param job: job to run
 * @param service_specific_arguments: service-specific arguments
 * @return
 */
    std::shared_ptr<ComputeService> HTCondorNegotiatorService::pickTargetComputeServiceNonGridUniverse(
            std::shared_ptr<CompoundJob> job, std::map<std::string,
            std::string>
    service_specific_arguments) {
        std::shared_ptr<BareMetalComputeService> target_cs = nullptr;

        // Figure out which BatchComputeService compute services are available
        for (auto const &cs: this->compute_services) {
            // Only BareMetalComputeServices can be used
            if (not std::dynamic_pointer_cast<BareMetalComputeService>(cs)) {
                continue;
            }
            // If job type is not supported, nevermind (shouldn't happen really)
            if (std::dynamic_pointer_cast<CompoundJob>(job) and (not cs->supportsCompoundJobs())) {
                continue;
            }

            // If service-specific arguments are provided, for now reject them
            if (not service_specific_arguments.empty()) {
                throw std::invalid_argument("HTCondorNegotiatorService::pickTargetComputeServiceNonGridUniverse(): "
                                            "service-specific arguments for Non-Grid universe jobs are currently not supported");
            }

            unsigned long min_required_num_cores = job->getMinimumRequiredNumCores();
            double min_required_memory = job->getMinimumRequiredMemory();

            bool enough_idle_resources = cs->isThereAtLeastOneHostWithIdleResources(min_required_num_cores,
                                                                                    min_required_memory);
#if 0
            // Check on RAM constraints
            auto ram_resources = cs->getPerHostAvailableMemoryCapacity();
            unsigned long max_available_ram_capacity = 0;
            for (auto const &entry: ram_resources) {
                max_available_ram_capacity = std::max<unsigned long>(max_available_ram_capacity, entry.second);
            }
            if (max_available_ram_capacity < sjob->getMinimumRequiredMemory()) {
                continue;
            }

            // Check on idle resources
            auto idle_core_resources = cs->getPerHostNumIdleCores();
            unsigned long max_num_idle_cores = 0;
            for (auto const &entry : idle_core_resources) {
                max_num_idle_cores = std::max<unsigned long>(max_num_idle_cores, entry.second);
            }
            if (max_num_idle_cores < sjob->getMinimumRequiredNumCores()) {
                continue;
            }
#endif
            if (enough_idle_resources) {
                // Return the first appropriate CS we found
                return cs;
            }
        }

        return nullptr;
    }


}// namespace wrench
