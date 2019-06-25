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
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/workflow/job/StandardJob.h"

namespace wrench {

    /**
     * @brief A workload management framework compute service
     *
     */
    class HTCondorComputeService : public ComputeService {
    private:
        std::map<std::string, std::string> default_property_values = {
                {HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS,    "true"},
                {HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}
        };

        std::map<std::string, double> default_messagepayload_values = {
                {HTCondorComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                  1024},
                {HTCondorComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,               1024},
                {HTCondorComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,  1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  512000000},
                {HTCondorComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,   1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,     1024},
                {HTCondorComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,      1024}
        };

    public:
        HTCondorComputeService(const std::string &hostname,
                               const std::string &pool_name,
                               std::set<ComputeService *> compute_resources,
                               std::map<std::string, std::string> property_list = {},
                               std::map<std::string, double> messagepayload_list = {});

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        void submitStandardJob(StandardJob *job,
                               std::map<std::string, std::string> &service_specific_arguments) override;

        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_arguments) override;

        void scheduleStandardJobForPilot(StandardJob *job, std::string &job_manager_mailbox_name,
                                         std::map<std::string, std::string> &service_specific_arguments);

        std::shared_ptr<StorageService> getLocalStorageService() const;

        void setLocalStorageService(std::shared_ptr<StorageService> local_storage_service);

        /***********************/
        /** \endcond          **/
        /***********************/


        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~HTCondorComputeService() override;

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

        void terminate();

        std::string pool_name;
        std::shared_ptr<StorageService> local_storage_service;
        std::shared_ptr<HTCondorCentralManagerService> central_manager;
    };
}

#endif //WRENCH_HTCONDOR_H
