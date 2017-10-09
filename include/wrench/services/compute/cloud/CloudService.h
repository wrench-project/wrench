/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_CLOUDSERVICE_H
#define WRENCH_CLOUDSERVICE_H

#include <simgrid/s4u/VirtualMachine.hpp>

#include "wrench/services/Service.h"
#include "wrench/services/compute/ComputeService.h"
#include "CloudServiceProperty.h"
#include "wrench/simulation/Simulation.h"

namespace wrench {

    class Simulation;

    class ComputeService;

    /**
     * @brief A Cloud Service
     */
    class CloudService : public ComputeService {

    private:
        std::map<std::string, std::string> default_property_values =
                {{CloudServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,                 "1024"},
                 {CloudServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,              "1024"},
                 {CloudServiceProperty::NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD,      "1024"},
                 {CloudServiceProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD,       "1024"},
                 {CloudServiceProperty::NUM_CORES_REQUEST_MESSAGE_PAYLOAD,           "1024"},
                 {CloudServiceProperty::NUM_CORES_ANSWER_MESSAGE_PAYLOAD,            "1024"},
                 {CloudServiceProperty::CREATE_VM_REQUEST_MESSAGE_PAYLOAD,           "1024"},
                 {CloudServiceProperty::CREATE_VM_ANSWER_MESSAGE_PAYLOAD,            "1024"},
                 {CloudServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                 {CloudServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"}
                };

    public:
        CloudService(std::string &hostname,
                     bool supports_standard_jobs,
                     bool supports_pilot_jobs,
                     StorageService *default_storage_service,
                     std::map<std::string, std::string> plist);

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        bool createVM(const std::string &pm_hostname,
                      const std::string &vm_hostname,
                      unsigned long num_cores,
                      std::map<std::string, std::string> plist = {});

        // Running jobs
        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_args) override;

        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_args) override;

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        friend class Simulation;

        int main() override;

        bool processNextMessage();

        void processGetNumCores(std::string &answer_mailbox) override;

        void processGetNumIdleCores(std::string &answer_mailbox) override;

        void processCreateVM(const std::string &answer_mailbox,
                             const std::string &pm_hostname,
                             const std::string &vm_hostname,
                             int num_cores,
                             bool supports_standard_jobs,
                             bool supports_pilot_jobs,
                             std::map<std::string, std::string> plist);

        void processSubmitStandardJob(std::string &answer_mailbox, StandardJob *job,
                                      std::map<std::string, std::string> &service_specific_args) override;

        void terminate();

        /** @brief A map of VMs described by the VM actor, the actual compute service, and the total number of cores */
        std::map<std::string, std::tuple<simgrid::s4u::VirtualMachine *, std::unique_ptr<ComputeService>, int>> vm_list;
    };

}

#endif //WRENCH_CLOUDSERVICE_H
