/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HTCONDORNEGOTIATORSERVICE_H
#define WRENCH_HTCONDORNEGOTIATORSERVICE_H

#include "wrench/services/Service.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessagePayload.h"
#include "wrench/workflow/job/WorkflowJob.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/
    /**
     * @brief A HTCondor negotiator service
     */
    class HTCondorNegotiatorService : public Service {
    private:
        std::map<std::string, double> default_messagepayload_values = {
                {HTCondorCentralManagerServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,              1024},
                {HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,           1024},
                {HTCondorCentralManagerServiceMessagePayload::HTCONDOR_NEGOTIATOR_DONE_MESSAGE_PAYLOAD, 1024},
        };

    public:

        HTCondorNegotiatorService(std::string &hostname,
                                  std::map<std::shared_ptr<ComputeService>, unsigned long> &compute_resources,
                                  std::map<WorkflowJob *, std::shared_ptr<ComputeService>> &running_jobs,
                                  std::vector<std::tuple<WorkflowJob *, std::map<std::string, std::string>>> &pending_jobs,
                                  std::string &reply_mailbox,
                                  std::shared_ptr<ComputeService> grid_universe_batch_service);

        ~HTCondorNegotiatorService();

    private:
        int main() override;

        struct JobPriorityComparator {
            bool operator()(std::tuple<WorkflowJob *, std::map<std::string, std::string>> &lhs,
                            std::tuple<WorkflowJob *, std::map<std::string, std::string>> &rhs);
        };

        /** mailbox to reply **/
        std::string reply_mailbox;
        /** set of compute resources **/
        std::map<std::shared_ptr<ComputeService>, unsigned long> *compute_resources;
        std::shared_ptr<ComputeService> grid_universe_batch_service;
        std::map<WorkflowJob *, std::shared_ptr<ComputeService>> *running_jobs;
        /** queue of pending jobs **/
        std::vector<std::tuple<WorkflowJob *, std::map<std::string, std::string>>> pending_jobs;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_HTCONDORNEGOTIATORSERVICE_H
