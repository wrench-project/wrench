/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HTCONDORCENTRALMANAGERSERVICE_H
#define WRENCH_HTCONDORCENTRALMANAGERSERVICE_H

#include <set>
#include <deque>
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/htcondor/HTCondorCentralManagerServiceMessagePayload.h"

namespace wrench {

    /**
     * @brief A HTCondor central manager service implementation
     */
    class HTCondorCentralManagerService : public ComputeService {
    private:
        std::map<std::string, std::string> default_property_values = {};

        std::map<std::string, double> default_messagepayload_values = {
                {HTCondorCentralManagerServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                  1024},
                {HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,               1024},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  256000000},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,   256000000},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,     256000000},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,      256000000},
                {HTCondorCentralManagerServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 196000000},
                {HTCondorCentralManagerServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,  196000000},
                {HTCondorCentralManagerServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,            512000000},
                {HTCondorCentralManagerServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,            1024},
                {HTCondorCentralManagerServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,            1024}
        };

    public:
        HTCondorCentralManagerService(const std::string &hostname,
                                      std::set<ComputeService *> compute_resources,
                                      std::map<std::string, std::string> property_list = {},
                                      std::map<std::string, double> messagepayload_list = {});

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        void submitStandardJob(StandardJob *job,
                               const std::map<std::string, std::string> &service_specific_arguments) override;

        void submitPilotJob(PilotJob *job, const std::map<std::string, std::string> &service_specific_arguments) override;


        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~HTCondorCentralManagerService() override;

        void terminateStandardJob(StandardJob *job) override;

        void terminatePilotJob(PilotJob *job) override;

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        int main() override;

        bool processNextMessage();

        void processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                      std::map<std::string, std::string> &service_specific_args);

        void processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job,
                                   std::map<std::string, std::string> &service_specific_args);

        void processPilotJobStarted(PilotJob *job);

        void processPilotJobCompletion(PilotJob *job);

        void processStandardJobCompletion(StandardJob *job);

        void processNegotiatorCompletion(std::vector<WorkflowJob *> &pending_jobs);

        void terminate();

        /** set of compute resources **/
        std::set<ComputeService *> compute_resources;
        /** queue of pending jobs **/
        std::vector<std::tuple<WorkflowJob *, std::map<std::string, std::string>>> pending_jobs;
        /** whether a negotiator is dispatching jobs **/
        bool dispatching_jobs = false;
        /** whether a negotiator could not dispatch jobs **/
        bool resources_unavailable = false;
        /** **/
        std::map<std::shared_ptr<ComputeService>, unsigned long> compute_resources_map;
        /** running workflow jobs **/
        std::map<WorkflowJob *, std::shared_ptr<ComputeService>> running_jobs;
    };

}

#endif //WRENCH_HTCONDORCENTRALMANAGERSERVICE_H
