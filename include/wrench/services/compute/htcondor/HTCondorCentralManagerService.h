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
    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A HTCondor central manager service implementation
     */
    class HTCondorCentralManagerService : public ComputeService {
    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {};

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {HTCondorCentralManagerServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                //                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                //                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                //                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                //                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {HTCondorCentralManagerServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorCentralManagerServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024},
                //                {HTCondorCentralManagerServiceMessagePayload::STANDARD_JOB_DONE_MESSAGE_PAYLOAD, 1024},
                {HTCondorCentralManagerServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD, 1024},
                {HTCondorCentralManagerServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD, 1024}
                //                {HTCondorCentralManagerServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD, 1024},
                //                {HTCondorCentralManagerServiceMessagePayload::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD, 1024}
        };

    public:
        HTCondorCentralManagerService(const std::string &hostname,
                                      double negotiator_startup_overhead,
                                      std::set<std::shared_ptr<ComputeService>> compute_services,
                                      WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                      WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;

        void addComputeService(std::shared_ptr<ComputeService> compute_service);

        //        void submitStandardJob(std::shared_ptr<StandardJob> job,
        //                               const std::map<std::string, std::string> &service_specific_arguments);

        void submitCompoundJob(std::shared_ptr<CompoundJob> job,
                               const std::map<std::string, std::string> &service_specific_arguments) override;

        //        void submitPilotJob(std::shared_ptr<PilotJob> job, const std::map<std::string, std::string> &service_specific_arguments) override;


        ~HTCondorCentralManagerService() override;

        //        void terminateStandardJob(std::shared_ptr<StandardJob> job) override;

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override{};

        //        void terminatePilotJob(std::shared_ptr<PilotJob> job) override;

        bool jobKindIsSupported(const std::shared_ptr<Job> &job, std::map<std::string, std::string> service_specific_arguments);

        bool jobCanRunSomewhere(std::shared_ptr<CompoundJob> job, std::map<std::string, std::string> service_specific_arguments);

    private:
        friend class HTCondorComputeService;

        int main() override;

        bool processNextMessage();

        void processSubmitCompoundJob(simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<CompoundJob> job,
                                      std::map<std::string, std::string> &service_specific_args);

        void processCompoundJobCompletion(std::shared_ptr<CompoundJob> job);

        void processCompoundJobFailure(std::shared_ptr<CompoundJob> job);

        void processNegotiatorCompletion(std::vector<std::shared_ptr<Job>> &pending_jobs);

        void terminate();


        /** set of compute resources **/
        std::set<std::shared_ptr<ComputeService>> compute_services;
        /** queue of pending jobs **/
        std::vector<std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>>> pending_jobs;
        /** running workflow jobs **/
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>> running_jobs;
        /** whether a negotiator is dispatching jobs **/
        bool dispatching_jobs = false;
        /** whether a negotiator could not dispatch jobs **/
        bool resources_unavailable = false;
        /** negotiator startup overhead in seconds **/
        double negotiator_startup_overhead = 0.0;
    };

    /***********************/
    /** \endcond          **/
    /***********************/

}// namespace wrench

#endif//WRENCH_HTCONDORCENTRALMANAGERSERVICE_H
