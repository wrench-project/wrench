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

WRENCH_LOG_CATEGORY(wrench_core_htcondor_negotiator, "Log category for HTCondorNegotiator");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param compute_resources: a set of compute resources available via the HTCondor pool
     * @param running_jobs:
     * @param pending_jobs: a list of pending jobs
     * @param reply_mailbox: the mailbox to which the "done/failed" message should be sent
     */
    HTCondorNegotiatorService::HTCondorNegotiatorService(
            std::string &hostname,
            std::map<std::shared_ptr<ComputeService>, unsigned long> &compute_resources,
            std::map<WorkflowJob *, std::shared_ptr<ComputeService>> &running_jobs,
            std::vector<std::tuple<WorkflowJob *, std::map<std::string, std::string>>> &pending_jobs,
            std::string &reply_mailbox,
            std::shared_ptr<ComputeService> &grid_universe_batch_service)
            : Service(hostname, "htcondor_negotiator", "htcondor_negotiator"), reply_mailbox(reply_mailbox),
              compute_resources(&compute_resources), running_jobs(&running_jobs), pending_jobs(pending_jobs),
              grid_universe_batch_service(grid_universe_batch_service) {

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
            std::tuple<WorkflowJob *, std::map<std::string, std::string>> &lhs,
            std::tuple<WorkflowJob *, std::map<std::string, std::string>> &rhs) {
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

        std::vector<WorkflowJob *> scheduled_jobs;

        // sort tasks by priority
        std::sort(this->pending_jobs.begin(), this->pending_jobs.end(), JobPriorityComparator());

        for (auto entry : this->pending_jobs) {

            auto job = std::get<0>(entry);
            auto service_specific_arguments = std::get<1>(entry);


            //GRID STANDARD JOB
            //Diverts grid jobs to batch service if it has been provided when initializing condor.
            if (auto standard_job = dynamic_cast<StandardJob *>(job)) {
                if (service_specific_arguments.find("universe") == service_specific_arguments.end()) {
                    for (auto &item : *this->compute_resources) {
                        if (not item.first->supportsStandardJobs()) {
                            continue;
                        }

                        if (item.second >= standard_job->getMinimumRequiredNumCores()) {

                            WRENCH_INFO("Dispatching job %s with %ld tasks", standard_job->getName().c_str(),
                                        standard_job->getTasks().size());

                            for (auto task : standard_job->getTasks()) {
                                // temporary printing task IDs
                                WRENCH_INFO("    Task ID: %s", task->getID().c_str());
                            }

                            WRENCH_INFO("---> %lu", service_specific_arguments.size());

                            standard_job->pushCallbackMailbox(this->reply_mailbox);
                            item.first->submitStandardJob(standard_job, service_specific_arguments);
                            this->running_jobs->insert(std::make_pair(job, item.first));
                            scheduled_jobs.push_back(job);
                            item.second -= standard_job->getMinimumRequiredNumCores();

                            WRENCH_INFO("Dispatched job %s with %ld tasks", standard_job->getName().c_str(),
                                        standard_job->getTasks().size());
                            break;
                        }
                    }
                } else {
                    if (service_specific_arguments["universe"].compare("grid") == 0) {

                        //instead of erasing here, going to iterate through.
                        //service_specific_arguments.erase("universe");
                        service_specific_arguments.insert(std::pair<std::string, std::string>("-N", "1"));
                        service_specific_arguments.insert(std::pair<std::string, std::string>("-c", "1"));
                        service_specific_arguments.insert(std::pair<std::string, std::string>("-t", "9999"));

                        std::map<std::string, std::string> service_specs_copy;

                        WRENCH_INFO("Dispatching job %s with %ld tasks", standard_job->getName().c_str(),
                                    standard_job->getTasks().size());

                        for (auto task : standard_job->getTasks()) {
                            // temporary printing task IDs
                            WRENCH_INFO("    Task ID: %s", task->getID().c_str());
                        }

                        //S4U_Simulation::sleep(140.0);

                        standard_job->pushCallbackMailbox(this->reply_mailbox);
                        this->grid_universe_batch_service->submitStandardJob(standard_job, service_specific_arguments);
                        this->running_jobs->insert(std::make_pair(job, this->grid_universe_batch_service));
                        scheduled_jobs.push_back(job);
                        standard_job->getMinimumRequiredNumCores();

                        WRENCH_INFO("Dispatched grid universe job %s with %ld tasks to batch service",
                                    standard_job->getName().c_str(),
                                    standard_job->getTasks().size());
                    }
                }
            } else if (auto pilot_job = dynamic_cast<PilotJob *>(job)) { // PILOT JOB

                for (auto &item : *this->compute_resources) {
                    if (not item.first->supportsPilotJobs()) {
                        continue;
                    }

                    pilot_job->pushCallbackMailbox(this->reply_mailbox);
                    item.first->submitPilotJob(pilot_job, service_specific_arguments);
                    this->running_jobs->insert(std::make_pair(job, item.first));
                    scheduled_jobs.push_back(job);

                    WRENCH_INFO("Dispatched pilot job %s", pilot_job->getName().c_str());
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
