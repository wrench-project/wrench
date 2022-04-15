/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HTCONDOR_H
#define WRENCH_HTCONDOR_H

#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/htcondor/HTCondorCentralManagerService.h"
#include "wrench/services/compute/htcondor/HTCondorComputeServiceProperty.h"
#include "wrench/services/compute/htcondor/HTCondorComputeServiceMessagePayload.h"
#include "wrench/job/PilotJob.h"
#include "wrench/job/StandardJob.h"

namespace wrench {

    /**
     * @brief A workload management framework compute service
     *
     */
    class HTCondorComputeService : public ComputeService {
    private:
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {HTCondorComputeServiceProperty::NEGOTIATOR_OVERHEAD, "0.0"},
                {HTCondorComputeServiceProperty::GRID_PRE_EXECUTION_DELAY, "0.0"},
                {HTCondorComputeServiceProperty::GRID_POST_EXECUTION_DELAY, "0.0"},
                {HTCondorComputeServiceProperty::NON_GRID_PRE_EXECUTION_DELAY, "0.0"},
                {HTCondorComputeServiceProperty::NON_GRID_POST_EXECUTION_DELAY, "0.0"},
        };

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {HTCondorComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_COMPOUND_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {BatchComputeServiceMessagePayload::PILOT_JOB_STARTED_MESSAGE_PAYLOAD, 1024},// for forwarding
                {HTCondorComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD, 1024},
        };

    public:
        HTCondorComputeService(const std::string &hostname,
                               std::set<std::shared_ptr<ComputeService>> compute_services,
                               WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        void validateJobsUseOfScratch(std::map<std::string, std::string> &service_specific_args) override;

        void validateServiceSpecificArguments(std::shared_ptr<CompoundJob> compound_job,
                                              std::map<std::string, std::string> &service_specific_args) override;

        void addComputeService(std::shared_ptr<ComputeService> compute_service);

        //        void submitStandardJob(std::shared_ptr<StandardJob> job,
        //                               const std::map<std::string, std::string> &service_specific_arguments);

        void submitCompoundJob(std::shared_ptr<CompoundJob> job,
                               const std::map<std::string, std::string> &service_specific_arguments) override;


        //        void submitPilotJob(std::shared_ptr<PilotJob> job, const std::map<std::string, std::string> &service_specific_arguments) override;

        std::shared_ptr<StorageService> getLocalStorageService() const;

        void setLocalStorageService(std::shared_ptr<StorageService> local_storage_service);

        /***********************/
        /** \endcond          **/
        /***********************/


        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~HTCondorComputeService() override;

        //        void terminateStandardJob(std::shared_ptr<StandardJob> job) override;

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override{};

        //        void terminatePilotJob(std::shared_ptr<PilotJob> job) override;

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        int main() override;


        bool processNextMessage();

        void processSubmitCompoundJob(simgrid::s4u::Mailbox *answer_mailbox, std::shared_ptr<CompoundJob> job,
                                      const std::map<std::string, std::string> &service_specific_args);


        //        void processSubmitStandardJob(const std::string &answer_mailbox, std::shared_ptr<StandardJob>job,
        //                                      const std::map<std::string, std::string> &service_specific_args);
        //
        //        void processSubmitPilotJob(const std::string &answer_mailbox, std::shared_ptr<PilotJob>job,
        //                                   const std::map<std::string, std::string> &service_specific_args);

        void processIsThereAtLeastOneHostWithAvailableResources(simgrid::s4u::Mailbox *answer_mailbox, unsigned long num_cores, double ram);

        void terminate();

        std::string pool_name;
        std::shared_ptr<StorageService> local_storage_service;
        std::shared_ptr<HTCondorCentralManagerService> central_manager;
    };
}// namespace wrench

#endif//WRENCH_HTCONDOR_H
