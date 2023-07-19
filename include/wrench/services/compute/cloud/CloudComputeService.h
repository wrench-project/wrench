/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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
#include "wrench/services/compute/cloud/CloudComputeServiceProperty.h"
#include "wrench/services/compute/cloud/CloudComputeServiceMessagePayload.h"
#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"
#include "wrench/job/PilotJob.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"


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
        WRENCH_PROPERTY_COLLECTION_TYPE default_property_values = {
                {CloudComputeServiceProperty::VM_BOOT_OVERHEAD, "0"},
                {CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM, "best-fit-ram-first"},
                {CloudComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE, "0"}};

        WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
                {CloudComputeServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::START_VM_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SUSPEND_VM_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::RESUME_VM_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::DESTROY_VM_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::DESTROY_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD, 1024},
                {CloudComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD, 1024},
        };

    public:
        CloudComputeService(const std::string &hostname,
                            const std::vector<std::string> &execution_hosts,
                            const std::string &scratch_space_mount_point,
                            WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});


        bool supportsStandardJobs() override;
        bool supportsCompoundJobs() override;
        bool supportsPilotJobs() override;

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        virtual std::string createVM(unsigned long num_cores,
                                     double ram_memory,
                                     WRENCH_PROPERTY_COLLECTION_TYPE property_list = {},
                                     WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list = {});

        virtual void shutdownVM(const std::string &vm_name);

        virtual void shutdownVM(const std::string &vm_name, bool send_failure_notifications, ComputeService::TerminationCause termination_cause);

        virtual std::shared_ptr<BareMetalComputeService> startVM(const std::string &vm_name);

        virtual std::shared_ptr<BareMetalComputeService> getVMComputeService(const std::string &vm_name);

        virtual std::string getVMPhysicalHostname(const std::string &vm_name);

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

        void submitCompoundJob(std::shared_ptr<CompoundJob> job,
                               const std::map<std::string, std::string> &service_specific_args) override{};

        void terminateCompoundJob(std::shared_ptr<CompoundJob> job) override{};

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

        template<class TMessageType>

        /**
         * @brief Send a message request
         *
         * @param answer_mailbox: the mailbox to which the answer message should be sent
         * @param tosend: message to be sent
         * @return a simulation message
         *
         * @throw std::runtime_error
         */
        std::shared_ptr<TMessageType> sendRequestAndWaitForAnswer(simgrid::s4u::Mailbox *answer_mailbox, ComputeServiceMessage *tosend) {
            serviceSanityCheck();
            S4U_Mailbox::putMessage(this->mailbox, tosend);

            // Wait for a reply
            return S4U_Mailbox::getMessage<TMessageType>(answer_mailbox, this->network_timeout, "CloudComputeService::sendRequestAndWaitForAnswer(): received an");
        }

        virtual bool processNextMessage();

        virtual void processGetResourceInformation(simgrid::s4u::Mailbox *answer_mailbox,
                                                   const std::string &key);

        virtual void processGetExecutionHosts(simgrid::s4u::Mailbox *answer_mailbox);

        virtual void processCreateVM(simgrid::s4u::Mailbox *answer_mailbox,
                                     unsigned long requested_num_cores,
                                     double requested_ram,
                                     const std::string &physical_host,
                                     WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                     WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list);

        virtual void
        processStartVM(simgrid::s4u::Mailbox *answer_mailbox, const std::string &vm_name);

        virtual void processShutdownVM(simgrid::s4u::Mailbox *answer_mailbox,
                                       const std::string &vm_name,
                                       bool send_failure_notifications,
                                       ComputeService::TerminationCause termination_cause);

        virtual void processSuspendVM(simgrid::s4u::Mailbox *answer_mailbox, const std::string &vm_name);

        virtual void processResumeVM(simgrid::s4u::Mailbox *answer_mailbox, const std::string &vm_name);

        virtual void processDestroyVM(simgrid::s4u::Mailbox *answer_mailbox, const std::string &vm_name);

        //        virtual void processSubmitStandardJob(const std::string &answer_mailbox, std::shared_ptr<StandardJob> job,
        //                                              std::map<std::string, std::string> &service_specific_args);
        //
        //        virtual void processSubmitPilotJob(const std::string &answer_mailbox, std::shared_ptr<PilotJob> job,
        //                                           std::map<std::string, std::string> &service_specific_args);

        virtual void
        processBareMetalComputeServiceTermination(const std::shared_ptr<BareMetalComputeService> &cs, int exit_code);

        virtual void processIsThereAtLeastOneHostWithAvailableResources(
                simgrid::s4u::Mailbox *answer_mailbox, unsigned long num_cores, double ram);


        void stopAllVMs(bool send_failure_notifications, ComputeService::TerminationCause termination_cause);

        /** \cond */
        static unsigned long VM_ID;
        /** \endcond */

        /** @brief List of execution host names */
        std::vector<std::string> execution_hosts;

        /** @brief Map of used RAM at the hosts */
        std::unordered_map<std::string, double> used_ram_per_execution_host;

        /** @brief Map of number of used cores at the hosts */
        std::unordered_map<std::string, unsigned long> used_cores_per_execution_host;

        /** @brief A map of VMs */
        std::unordered_map<std::string, std::tuple<std::shared_ptr<S4U_VirtualMachine>, std::string, std::shared_ptr<BareMetalComputeService>>> vm_list;

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::string findHost(unsigned long desired_num_cores, double desired_ram, const std::string &desired_host);
    };
}// namespace wrench

#endif//WRENCH_CLOUDSERVICE_H
