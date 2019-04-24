/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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

XBT_LOG_NEW_DEFAULT_CATEGORY(htcondor_negotiator, "Log category for HTCondorNegotiator");

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
            std::map<ComputeService *, unsigned long> &compute_resources,
            std::map<StandardJob *, ComputeService *> &running_jobs,
            std::vector<StandardJob *> &pending_jobs,
            std::string &reply_mailbox)
            : Service(hostname, "htcondor_negotiator", "htcondor_negotiator"), reply_mailbox(reply_mailbox),
              compute_resources(&compute_resources), running_jobs(&running_jobs), pending_jobs(pending_jobs) {

      this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
    }

    /**
     * @brief Destructor
     */
    HTCondorNegotiatorService::~HTCondorNegotiatorService() {
      this->pending_jobs.clear();
    }

    /**
     * @brief Compare the priority between two workflow standard job
     *
     * @param lhs: pointer to a standard job
     * @param rhs: pointer to a standard job
     *
     * @return whether the priority of the left-hand-side standard job is higher
     */
    bool HTCondorNegotiatorService::JobPriorityComparator::operator()(StandardJob *&lhs, StandardJob *&rhs) {
      long lhs_max_priority = 0;
      long rhs_max_priority = 0;

      for (auto task : lhs->getTasks()) {
        if (task->getPriority() > lhs_max_priority) {
          lhs_max_priority = task->getPriority();
        }
      }
      for (auto task : rhs->getTasks()) {
        if (task->getPriority() > rhs_max_priority) {
          rhs_max_priority = task->getPriority();
        }
      }

      return lhs_max_priority > rhs_max_priority;
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

      // TODO: check how to pass the service specific arguments
      std::map<std::string, std::string> specific_args;
      std::vector<StandardJob *> scheduled_jobs;

      // sort tasks by priority
      std::sort(this->pending_jobs.begin(), this->pending_jobs.end(), JobPriorityComparator());

      for (auto job : this->pending_jobs) {
        for (auto &item : *this->compute_resources) {
          if (item.second >= job->getMinimumRequiredNumCores()) {
            WRENCH_INFO("Dispatching job %s with %ld tasks", job->getName().c_str(), job->getTasks().size());
            for (auto task : job->getTasks()) {
              WRENCH_INFO("    Task ID: %s", task->getID().c_str());
            }

            job->pushCallbackMailbox(this->reply_mailbox);
            item.first->submitStandardJob(job, specific_args);
            this->running_jobs->insert(std::make_pair(job, item.first));
            scheduled_jobs.push_back(job);
            item.second -= job->getMinimumRequiredNumCores();
            WRENCH_INFO("Dispatched job %s with %ld tasks", job->getName().c_str(), job->getTasks().size());
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

      WRENCH_INFO("HTCondorNegotiator Service on host %s cleanly terminating!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

}
