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
#include "wrench/job/Job.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/
    /**
     * @brief A HTCondor negotiator service
     */
    class HTCondorNegotiatorService : public Service {
    private:
WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE  default_messagepayload_values = {
                {HTCondorCentralManagerServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,              1024},
                {HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,           1024},
                {HTCondorCentralManagerServiceMessagePayload::HTCONDOR_NEGOTIATOR_DONE_MESSAGE_PAYLOAD, 1024},
        };

    public:

        HTCondorNegotiatorService(std::string &hostname,
                                  double startup_overhead,
                                  std::set<std::shared_ptr<ComputeService>> &compute_services,
                                  std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>> &running_jobs,
                                  std::vector<std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>>> &pending_jobs,
                                  simgrid::s4u::Mailbox *reply_mailbox);

        ~HTCondorNegotiatorService();

    private:
        int main() override;

        struct JobPriorityComparator {
            bool operator()(std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>> &lhs,
                            std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>> &rhs);
        };

        std::shared_ptr<ComputeService> pickTargetComputeService(std::shared_ptr<CompoundJob> job, std::map<std::string, std::string> service_specific_arguments);
        std::shared_ptr<ComputeService> pickTargetComputeServiceGridUniverse(std::shared_ptr<CompoundJob> job, std::map<std::string, std::string> service_specific_arguments);
        std::shared_ptr<ComputeService> pickTargetComputeServiceNonGridUniverse(std::shared_ptr<CompoundJob> job, std::map<std::string, std::string> service_specific_arguments);

        /** startup overhead **/
        double startup_overhead;
        /** mailbox to reply **/
        simgrid::s4u::Mailbox *reply_mailbox;
        /** set of compute resources **/
        std::set<std::shared_ptr<ComputeService>> compute_services;
        /**map of ongoing jobs **/
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>> running_jobs;
        /** queue of pending jobs **/
        std::vector<std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>>> pending_jobs;
    };

    /***********************/
    /** \endcond           */
    /***********************/
}

#endif //WRENCH_HTCONDORNEGOTIATORSERVICE_H
