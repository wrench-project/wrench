/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessage.h"
#include "wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessagePayload.h"
#include "wrench/services/compute/htcondor/HTCondorNegotiatorService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/workflow/WorkflowTask.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/workflow/job/StandardJob.h"
#include <wrench/workflow/failure_causes/NetworkError.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>

WRENCH_LOG_CATEGORY(wrench_core_htcondor_negotiator, "Log category for HTCondorNegotiator");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param compute_services: a set of 'child' compute services available to and via the HTCondor pool
     * @param running_jobs: a list of currently running jobs
     * @param pending_jobs: a list of pending jobs
     * @param reply_mailbox: the mailbox to which the "done/failed" message should be sent
     */
    HTCondorNegotiatorService::HTCondorNegotiatorService(
            std::string &hostname,
            std::set<std::shared_ptr<ComputeService>> &compute_services,
            std::map<std::shared_ptr<WorkflowJob>, std::shared_ptr<ComputeService>> &running_jobs,
            std::vector<std::tuple<std::shared_ptr<WorkflowJob>, std::map<std::string, std::string>>> &pending_jobs,
            std::string &reply_mailbox)
            : Service(hostname, "htcondor_negotiator", "htcondor_negotiator"), reply_mailbox(reply_mailbox),
              compute_services(compute_services), running_jobs(running_jobs), pending_jobs(pending_jobs) {

        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
    }

    /**
     * @brief Destructor
     */
    HTCondorNegotiatorService::~HTCondorNegotiatorService() {
        this->pending_jobs.clear();
    }

    /**
     * @brief Compare the priority between two workflow jobs
     *
     * @param lhs: pointer to a workflow job
     * @param rhs: pointer to a workflow job
     *
     * @return whether the priority of the left-hand-side workflow job is higher
     */
    bool HTCondorNegotiatorService::JobPriorityComparator::operator()(
            std::tuple<std::shared_ptr<WorkflowJob>, std::map<std::string, std::string>> &lhs,
            std::tuple<std::shared_ptr<WorkflowJob>, std::map<std::string, std::string>> &rhs) {
        return std::get<0>(lhs)->getPriority() > std::get<0>(rhs)->getPriority();
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int HTCondorNegotiatorService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);

        WRENCH_INFO("HTCondor Negotiator Service starting on host %s listening on mailbox_name %s",
                    this->hostname.c_str(), this->mailbox_name.c_str());

        std::vector<std::shared_ptr<WorkflowJob>> scheduled_jobs;


        // sort jobs by priority
        std::sort(this->pending_jobs.begin(), this->pending_jobs.end(), JobPriorityComparator());

        // Go through the jobs and schedule them if possible
        for (auto entry : this->pending_jobs) {

            auto job = std::get<0>(entry);
            auto service_specific_arguments = std::get<1>(entry);
            bool is_standard_job = (std::dynamic_pointer_cast<StandardJob>(job) != nullptr);

            auto target_compute_service = pickTargetComputeService(job, service_specific_arguments);

            if (target_compute_service) {
                job->pushCallbackMailbox(this->reply_mailbox);
                if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
                    target_compute_service->submitStandardJob(sjob, service_specific_arguments);
                } else {
                    auto pjob = std::dynamic_pointer_cast<PilotJob>(job);
                    target_compute_service->submitPilotJob(pjob, service_specific_arguments);
                }
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
            std::shared_ptr<WorkflowJob> job, std::map<std::string,
            std::string> service_specific_arguments) {

        bool is_grid_universe = (service_specific_arguments.find("universe") != service_specific_arguments.end());

        if (is_grid_universe) {
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
            std::shared_ptr<WorkflowJob> job, std::map<std::string,
            std::string> service_specific_arguments) {

        std::set<std::shared_ptr<BatchComputeService>> available_batch_compute_services;

        // Figure out which batch compute services are available
        for (auto const &cs : this->compute_services) {
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

        // Find the target batch compute service
        std::shared_ptr<BatchComputeService> target_batch_cs = nullptr;
        for (auto const &batch_cs : available_batch_compute_services) {
            if (batch_cs->getName() == service_specific_arguments["-service"]) {
                target_batch_cs = batch_cs;
                break;
            }
        }
        if (target_batch_cs == nullptr) {
            throw std::invalid_argument("HTCondorNegotiatorService::pickTargetComputeServiceGridUniverse(): "
                                        "-service service-specific argument specifies a batch compute service named '" +
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
            std::shared_ptr<WorkflowJob> job, std::map<std::string,
            std::string> service_specific_arguments) {

        std::shared_ptr<BareMetalComputeService> target_cs = nullptr;

        if (std::dynamic_pointer_cast<PilotJob>(job)) {
            throw std::invalid_argument("HTCondorNegotiatorService::pickTargetComputeServiceNonGridUniverse(): "
                                        "Non-Grid universe pilot jobs are currently not supported");
        }
        auto sjob = std::dynamic_pointer_cast<StandardJob>(job);

        // Figure out which batch compute services are available
        for (auto const &cs : this->compute_services) {
            // Only BareMetalComputeServices can be used
            if (not std::dynamic_pointer_cast<BareMetalComputeService>(cs)) {
                continue;
            }
            // If job type is not supported, nevermind (shouldn't happen really)
            if (std::dynamic_pointer_cast<StandardJob>(job) and (not cs->supportsStandardJobs())) {
                continue;
            }

            // If service-specific arguments are provided, for now reject them
            if (not service_specific_arguments.empty()) {
                throw std::invalid_argument("HTCondorNegotiatorService::pickTargetComputeServiceNonGridUniverse(): "
                                            "service-specific arguments for Non-Grid universe jobs are currently not supported");
            }
            // Check on idle resources
            auto resources = cs->getPerHostNumIdleCores();
            unsigned long max_num_idle_cores = 0;
            for (auto const &entry : resources) {
                max_num_idle_cores = std::max<unsigned long>(max_num_idle_cores, entry.second);
            }
            if (max_num_idle_cores >= sjob->getMinimumRequiredNumCores()) {
                return cs;
            }
        }

       return nullptr;
    }
}


#if 0
// If the job is not a grid universe

// Determine the compute services that can possibly be used
for (auto const &compute_service : this->compute_services) {
// Check that job can run in principle based on what is supported
if (is_standard_job and not compute_service->supportsStandardJobs()) {
continue;
}
if (not is_standard_job and not compute_service->supportsPilotJobs()) {
continue;
}
if (is_grid_universe and (std::dynamic_pointer_cast<BatchComputeService>(compute_service) == nullptr)) {
continue;
}
if ((not is_grid_universe) and (std::dynamic_pointer_cast<BatchComputeService>(compute_service) != nullptr)) {
continue;
}

// For grid universe jobs, check that -N and -c arguments can work and that there is a service with the appropriate name
if (is_grid_universe) {
unsigned long required_num_hosts;
unsigned long required_num_cores_per_host;
try {
required_num_hosts = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-N", service_specific_arguments);
required_num_cores_per_host = BatchComputeService::parseUnsignedLongServiceSpecificArgument("-c", service_specific_arguments);
} catch (std::invalid_argument &e) {
throw std::invalid_argument(
"HTCondorNegotiatorService::main(): invalid service-specific arguments to grid universe job");
}
bool found_service = false;
for (auto const &cs : this->compute_services) {
auto batch_cs = std::dynamic_pointer_cast<BatchComputeService>(cs);
if (batch_cs and (batch_cs->getName() == service_specific_arguments["-service"])) {

}
}

}

// check that
if (is_standard_job) {
auto min_required_num_cores = std::dynamic_pointer_cast<StandardJob>(job)->getMinimumRequiredNumCores();
auto per_host_num_idle_core = compute_service->getPerHostNumIdleCores();
bool enough_cores = false;
for (auto const &entry : per_host_num_idle_core) {
if (entry.second >= min_required_num_cores) {
enough_cores = true;
break;
}
}
if (not enough_cores) {
continue;
}
}
possible_compute_services.push_back(compute_service);
}


//GRID STANDARD JOB
//Diverts grid jobs to batch service if it has been provided when initializing condor.
if (auto sjob = std::dynamic_pointer_cast<StandardJob>(job)) {
if (service_specific_arguments.find("universe") == service_specific_arguments.end()) {
for (auto &item : *this->compute_resources) {
if (not item.first->supportsStandardJobs()) {
continue;
}

if (item.second >= sjob->getMinimumRequiredNumCores()) {

WRENCH_INFO("Dispatching job %s with %ld tasks", sjob->getName().c_str(),
            sjob->getTasks().size());

for (auto task : sjob->getTasks()) {
// temporary printing task IDs
WRENCH_INFO("    Task ID: %s", task->getID().c_str());
}

WRENCH_INFO("---> %lu", service_specific_arguments.size());

sjob->pushCallbackMailbox(this->reply_mailbox);
item.first->submitStandardJob(sjob, service_specific_arguments);
this->running_jobs->insert(std::make_pair(job, item.first));
scheduled_jobs.push_back(job);
item.second -= sjob->getMinimumRequiredNumCores();

WRENCH_INFO("Dispatched job %s with %ld tasks", sjob->getName().c_str(),
            sjob->getTasks().size());
break;
}
}
} else {
if (service_specific_arguments["universe"].compare("grid") == 0) {
auto num_tasks = std::to_string((int) std::min(sjob->getNumTasks(),this->grid_universe_batch_service->getNumHosts()));

service_specific_arguments.insert(std::pair<std::string, std::string>("-N", num_tasks));
service_specific_arguments.insert(std::pair<std::string, std::string>("-c", "1"));
service_specific_arguments.insert(std::pair<std::string, std::string>("-t", "9999"));

std::map<std::string, std::string> service_specs_copy;


WRENCH_INFO("Dispatching job %s with %ld tasks", sjob->getName().c_str(),
            sjob->getTasks().size());

for (auto task : sjob->getTasks()) {
// temporary printing task IDs
WRENCH_INFO("    Task ID: %s", task->getID().c_str());
}

//S4U_Simulation::sleep(140.0);

sjob->pushCallbackMailbox(this->reply_mailbox);
this->grid_universe_batch_service->submitStandardJob(sjob, service_specific_arguments);
this->running_jobs->insert(std::make_pair(job, this->grid_universe_batch_service));
scheduled_jobs.push_back(job);
sjob->getMinimumRequiredNumCores();

WRENCH_INFO("Dispatched grid universe job %s with %ld tasks to batch service",
            sjob->getName().c_str(),
            sjob->getTasks().size());
}
}
} else if (auto pjob = std::dynamic_pointer_cast<PilotJob>(job)) { // PILOT JOB


for (auto &item : *this->compute_resources) {
if (not item.first->supportsPilotJobs()) {
continue;
}

pjob->pushCallbackMailbox(this->reply_mailbox);
item.first->submitPilotJob(pjob, service_specific_arguments);
this->running_jobs->insert(std::make_pair(job, item.first));
scheduled_jobs.push_back(job);

WRENCH_INFO("Dispatched pilot job %s", pjob->getName().c_str());
break;
}
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
}

#endif
