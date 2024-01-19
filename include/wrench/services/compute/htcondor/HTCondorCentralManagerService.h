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
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"

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
                {HTCondorCentralManagerServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {HTCondorCentralManagerServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {HTCondorCentralManagerServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {HTCondorCentralManagerServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {HTCondorCentralManagerServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {HTCondorCentralManagerServiceMessagePayload::COMPOUND_JOB_DONE_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size},
                {HTCondorCentralManagerServiceMessagePayload::COMPOUND_JOB_FAILED_MESSAGE_PAYLOAD, S4U_CommPort::default_control_message_size}};

    public:
        HTCondorCentralManagerService(const std::string &hostname,
                                      double negotiator_startup_overhead,
                                      double grid_pre_overhead,
                                      double grid_post_overhead,
                                      double non_grid_pre_overhead,
                                      double non_grid_post_overhead,
                                      bool fast_bmcs_resource_availability,
                                      bool fcfs,
                                      std::set<std::shared_ptr<ComputeService>> compute_services,
                                      WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                      WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;

        void addComputeService(std::shared_ptr<ComputeService> compute_service);

        void submitCompoundJob(std::shared_ptr<CompoundJob> job,
                               const std::map<std::string, std::string> &service_specific_arguments) override;

        ~HTCondorCentralManagerService() override;

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override{};

        //        bool jobKindIsSupported(const std::shared_ptr<Job> &job, std::map<std::string, std::string> service_specific_arguments);

        std::shared_ptr<FailureCause> jobCanRunSomewhere(const std::shared_ptr<CompoundJob> &job, std::map<std::string, std::string> service_specific_arguments);

    private:
        friend class HTCondorComputeService;

        std::map<std::string, double> constructResourceInformation(const std::string &key) override;

        int main() override;

        bool processNextMessage();

        void processSubmitCompoundJob(S4U_CommPort *answer_commport, std::shared_ptr<CompoundJob> job,
                                      std::map<std::string, std::string> &service_specific_args);

        void processCompoundJobCompletion(const std::shared_ptr<CompoundJob> &job);

        void processCompoundJobFailure(const std::shared_ptr<CompoundJob> &job);

        void processNegotiatorCompletion(std::set<std::shared_ptr<Job>> &pending_jobs);

        void terminate();

        /** set of compute resources **/
        std::set<std::shared_ptr<ComputeService>> compute_services;
        /** queue of pending jobs **/
        std::vector<std::tuple<std::shared_ptr<CompoundJob>, std::map<std::string, std::string>>> pending_jobs;
        /** running jobs **/
        std::map<std::shared_ptr<CompoundJob>, std::shared_ptr<ComputeService>> running_jobs;
        /** whether a negotiator is dispatching jobs **/
        bool dispatching_jobs = false;
        /** whether a negotiator could not dispatch jobs **/
        bool resources_unavailable = false;
        /** negotiator startup overhead in seconds **/
        double negotiator_startup_overhead = 0.0;
        /** fast resource availaiblity activated */
        bool fast_bmcs_resource_availability;
        /** FCFS scheduling enforced */
        bool fcfs;

        double grid_pre_overhead = 0.0;
        double grid_post_overhead = 0.0;
        double non_grid_pre_overhead = 0.0;
        double non_grid_post_overhead = 0.0;
    };

    /***********************/
    /** \endcond          **/
    /***********************/

}// namespace wrench

#endif//WRENCH_HTCONDORCENTRALMANAGERSERVICE_H
