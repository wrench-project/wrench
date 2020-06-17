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

#include "wrench/simulation/Simulation.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
//#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/cloud/CloudComputeServiceProperty.h"
#include "wrench/services/compute/cloud/CloudComputeServiceMessagePayload.h"
#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"
#include "wrench/workflow/job/PilotJob.h"


namespace wrench {

    class Simulation;

    class BareMetalComputeService;

    /**
     * @brief A cloud-based compute service that manages a set of physical
     *        hosts and controls access to their resources by (transparently) executing jobs
     *        in VM instances.
     */
    class CloudComputeService : public ComputeService {

    private:
        std::map<std::string, std::string> default_property_values = {
                {CloudComputeServiceProperty::SUPPORTS_PILOT_JOBS,         "false"},
                {CloudComputeServiceProperty::SUPPORTS_STANDARD_JOBS,      "false"},
                {CloudComputeServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS, "0.0"},
                {CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM, "best-fit-ram-first"}
        };

        std::map<std::string, double> default_messagepayload_values = {
                {CloudComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD,                  1024},
                {CloudComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD,               1024},
                {CloudComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD,  1024},
                {CloudComputeServiceMessagePayload::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD,  1024},
                {CloudComputeServiceMessagePayload::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD,   1024},
                {CloudComputeServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD,            1024},
                {CloudComputeServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD,             1024},
                {CloudComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD,          1024},
                {CloudComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD,           1024},
                {CloudComputeServiceMessagePayload::START_VM_REQUEST_MESSAGE_PAYLOAD,             1024},
                {CloudComputeServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD,              1024},
                {CloudComputeServiceMessagePayload::SUSPEND_VM_REQUEST_MESSAGE_PAYLOAD,           1024},
                {CloudComputeServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD,            1024},
                {CloudComputeServiceMessagePayload::RESUME_VM_REQUEST_MESSAGE_PAYLOAD,            1024},
                {CloudComputeServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD,             1024},
                {CloudComputeServiceMessagePayload::DESTROY_VM_REQUEST_MESSAGE_PAYLOAD,           1024},
                {CloudComputeServiceMessagePayload::DESTROY_VM_ANSWER_MESSAGE_PAYLOAD,           1024},
                {CloudComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,  1024},
                {CloudComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,   1024},
                {CloudComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,     1024},
                {CloudComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,      1024}
        };

    public:
        CloudComputeService(const std::string &hostname,
                     std::vector<std::string> &execution_hosts,
                     std::string scratch_space_mount_point,
                     std::map<std::string, std::string> property_list = {},
                     std::map<std::string, double> messagepayload_list = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        virtual std::string createVM(unsigned long num_cores,
                                     double ram_memory,
                                     std::map<std::string, std::string> property_list = {},
                                     std::map<std::string, double> messagepayload_list = {});

        virtual std::string createVM(unsigned long num_cores,
                                     double ram_memory,
                                     std::string desired_vm_name,
                                     std::map<std::string, std::string> property_list = {},
                                     std::map<std::string, double> messagepayload_list = {});

        virtual void shutdownVM(const std::string &vm_name);

        virtual std::shared_ptr<BareMetalComputeService> startVM(const std::string &vm_name);

        virtual void suspendVM(const std::string &vm_name);

        virtual void resumeVM(const std::string &vm_name);

        virtual void destroyVM(const std::string &vm_name);

        virtual bool isVMRunning(const std::string &vm_name);
        virtual bool isVMSuspended(const std::string &vm_name);
        virtual bool isVMDown(const std::string &vm_name);



        std::vector<std::string> getExecutionHosts();

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/
        void submitStandardJob(StandardJob *job, const std::map<std::string, std::string> &service_specific_args) override;

        void submitPilotJob(PilotJob *job, const std::map<std::string, std::string> &service_specific_args) override;

        void terminateStandardJob(StandardJob *job) override;
        void terminatePilotJob(PilotJob *job) override;

        void validateProperties();

        ~CloudComputeService();

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:
        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        friend class Simulation;

        int main() override;

        std::shared_ptr<SimulationMessage> sendRequest(std::string &answer_mailbox, ComputeServiceMessage *message);

        virtual bool processNextMessage();

        virtual void processGetResourceInformation(const std::string &answer_mailbox);

        virtual void processGetExecutionHosts(const std::string &answer_mailbox);

        virtual void processCreateVM(const std::string &answer_mailbox,
                                     unsigned long requested_num_cores,
                                     double requested_ram,
                                     std::string desired_vm_name,
                                     std::map<std::string, std::string> property_list,
                                     std::map<std::string, double> messagepayload_list
        );


        virtual void processStartVM(const std::string &answer_mailbox, const std::string &vm_name, const std::string &pm_name);

        virtual void processShutdownVM(const std::string &answer_mailbox, const std::string &vm_name);

        virtual void processSuspendVM(const std::string &answer_mailbox, const std::string &vm_name);

        virtual void processResumeVM(const std::string &answer_mailbox, const std::string &vm_name);

        virtual void processDestroyVM(const std::string &answer_mailbox, const std::string &vm_name);

        virtual void processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                              std::map<std::string, std::string> &service_specific_args);

        virtual void processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job,
                                           std::map<std::string, std::string> &service_specific_args);

        virtual void processBareMetalComputeServiceTermination(std::shared_ptr<BareMetalComputeService> cs, int exit_code);

        void stopAllVMs();

        /** \cond */
        static unsigned long VM_ID;
        /** \endcond */

        /** @brief List of execution host names */
        std::vector<std::string> execution_hosts;

        /** @brief Map of used RAM at the hosts */
        std::map<std::string, double> used_ram_per_execution_host;

        /** @brief Map of number of used cores at the hosts */
        std::map<std::string, unsigned long> used_cores_per_execution_host;

        /** @brief A map of VMs */
        std::map<std::string, std::pair<std::shared_ptr<S4U_VirtualMachine>, std::shared_ptr<BareMetalComputeService>>> vm_list;

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::string findHost(unsigned long desired_num_cores, double desired_ram, std::string desired_host);

    };
}

#endif //WRENCH_CLOUDSERVICE_H
