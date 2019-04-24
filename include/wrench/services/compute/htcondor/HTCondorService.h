/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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
#include "wrench/services/compute/htcondor/HTCondorServiceProperty.h"
#include "wrench/services/compute/htcondor/HTCondorServiceMessagePayload.h"
#include "wrench/workflow/job/StandardJob.h"

namespace wrench {

    /**
     * @brief A workload management framework compute service
     *
     */
    class HTCondorService : public ComputeService {
    private:
        std::map<std::string, std::string> default_property_values = {
                {HTCondorServiceProperty::SUPPORTS_PILOT_JOBS,    "true"},
                {HTCondorServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}
        };

        std::map<std::string, double> default_messagepayload_values = {
                {HTCondorServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                  1024},
                {HTCondorServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,               1024},
                {HTCondorServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {HTCondorServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,  1024},
                {HTCondorServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  512000000},
                {HTCondorServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,   1024},
                {HTCondorServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,     1024},
                {HTCondorServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,      1024}
        };

    public:
        HTCondorService(const std::string &hostname,
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

        StorageService *getLocalStorageService() const;

        void setLocalStorageService(StorageService *local_storage_service);

        /***********************/
        /** \endcond          **/
        /***********************/


        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~HTCondorService() override;

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

        void terminate();

        std::string pool_name;
        StorageService *local_storage_service;
        std::shared_ptr <HTCondorCentralManagerService> central_manager;
    };
}

#endif //WRENCH_HTCONDOR_H
