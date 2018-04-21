/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_CLOUDSERVICE_H
#define WRENCH_CLOUDSERVICE_H

#include <map>
#include <simgrid/s4u/VirtualMachine.hpp>

#include "wrench/services/Service.h"
#include "wrench/services/compute/ComputeService.h"
#include "CloudServiceProperty.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/workflow/job/PilotJob.h"

namespace wrench {

    class Simulation;

    class ComputeService;

    /**
     * @brief A simulated cloud-based compute service
     */
    class CloudService : public ComputeService {

    private:
        std::map<std::string, std::string> default_property_values =
                {{CloudServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,                  "1024"},
                 {CloudServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,               "1024"},
                 {CloudServiceProperty::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {CloudServiceProperty::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                 {CloudServiceProperty::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD,  "1024"},
                 {CloudServiceProperty::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD,   "1024"},
                 {CloudServiceProperty::CREATE_VM_REQUEST_MESSAGE_PAYLOAD,            "1024"},
                 {CloudServiceProperty::CREATE_VM_ANSWER_MESSAGE_PAYLOAD,             "1024"},
                 {CloudServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  "1024"},
                 {CloudServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,   "1024"},
                 {CloudServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,     "1024"},
                 {CloudServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,      "1024"}
                };

    public:
        CloudService(const std::string &hostname,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                     std::vector<std::string> &execution_hosts,
                     StorageService *default_storage_service,
                     std::map<std::string, std::string> plist = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        std::string createVM(const std::string &pm_hostname,
                             unsigned long num_cores,
                             double ram_memory = ComputeService::ALL_RAM,
                             std::map<std::string, std::string> plist = {});

        std::vector<std::string> getExecutionHosts();

        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_args) override;

        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_args) override;

        void terminateStandardJob(StandardJob *job) override;

        void terminatePilotJob(PilotJob *job) override;

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        ~CloudService();

        /***********************/
        /** \endcond          **/
        /***********************/


    private:
        friend class Simulation;

        int main() override;

        bool processNextMessage();

        void processGetResourceInformation(const std::string &answer_mailbox);

        void processGetExecutionHosts(const std::string &answer_mailbox);

        void processCreateVM(const std::string &answer_mailbox,
                             const std::string &pm_hostname,
                             const std::string &vm_hostname,
                             bool supports_standard_jobs,
                             bool supports_pilot_jobs,
                             unsigned long num_cores,
                             double ram_memory,
                             std::map<std::string, std::string> plist);

        void processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                      std::map<std::string, std::string> &service_specific_args);

        void processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job);

        void terminate();

        std::vector<std::string> execution_hosts;

        std::map<std::string, double> cs_available_ram;

        /** @brief A map of VMs described by the VM actor, the actual compute service, and the total number of cores */
        std::map<std::string, std::tuple<simgrid::s4u::VirtualMachine *, std::shared_ptr<ComputeService>, unsigned long>> vm_list;
    };

}

#endif //WRENCH_CLOUDSERVICE_H
