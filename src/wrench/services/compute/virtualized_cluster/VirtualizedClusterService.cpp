/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <cfloat>
#include <numeric>

#include "VirtualizedClusterServiceMessage.h"
#include "../cloud/CloudServiceMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(virtualized_cluster_service, "Log category for Virtualized Cluster Service");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the hostcreate on which to start the service
     * @param execution_hosts: the hosts available for running virtual machines
     * @param scratch_space_size: the size for the scratch space of the cloud service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    VirtualizedClusterService::VirtualizedClusterService(const std::string &hostname,
                                                         std::vector<std::string> &execution_hosts,
                                                         double scratch_space_size,
                                                         std::map<std::string, std::string> property_list,
                                                         std::map<std::string, std::string> messagepayload_list) :
            CloudService(hostname, execution_hosts, scratch_space_size) {

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));
        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
        // Validate Properties (the method from the super class)
        validateProperties();
    }

    /**
     * @brief Start a VM
     *
     * @param vm_name: the name of the VM
     * @param pm_name: the physical host on which to start the VM
     *
     * @return The compute service running on the VM
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    std::shared_ptr<BareMetalComputeService> VirtualizedClusterService::startVM(const std::string &vm_name, const std::string &pm_name) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudService::startVM(): Unknown VM name '" + vm_name + "'");
        }

        if ((not pm_name.empty()) and (std::find(this->execution_hosts.begin(), this->execution_hosts.end(), pm_name) == this->execution_hosts.end())) {
            throw std::invalid_argument("Trying to start a VM on an unknown (at least to this service) physical host");
        }

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("start_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceStartVMRequestMessage(
                        answer_mailbox, vm_name, pm_name,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::START_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudServiceStartVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
            return msg->cs;
        } else {
            throw std::runtime_error("CloudService::startVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Synchronously migrate a VM to another physical host
     *
     * @param vm_name: virtual machine hostname
     * @param dest_pm_hostname: the name of the destination physical machine host
     *
     * @throw std::invalid_argument
     */
    void VirtualizedClusterService::migrateVM(const std::string &vm_name, const std::string &dest_pm_hostname) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("VirtualizedClusterService::migrateVM(): Unknown VM name '" + vm_name + "'");
        }

        // send a "migrate vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("migrate_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new VirtualizedClusterServiceMigrateVMRequestMessage(
                        answer_mailbox, vm_name, dest_pm_hostname,
                        this->getMessagePayloadValueAsDouble(
                                VirtualizedClusterServiceMessagePayload::MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<VirtualizedClusterServiceMigrateVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error("VirtualizedClusterService::migrateVM(): Unexpected [" + msg->getName() + "] message");
        }
        return;
    }

    /**
    * @brief Main method of the daemon
    *
    * @return 0 on termination
    */
    int VirtualizedClusterService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);
        WRENCH_INFO("Virtualized Cluster Service starting on host %s listening on mailbox_name %s",
                    this->hostname.c_str(),
                    this->mailbox_name.c_str());

        /** Main loop **/
        while (this->processNextMessage()) {
            // no specific action
        }

        WRENCH_INFO("Virtualized Cluster Service on host %s cleanly terminating!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool VirtualizedClusterService::processNextMessage() {

        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::unique_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->stopAllVMs();
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValueAsDouble(
                                                CloudServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
            processGetResourceInformation(msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<CloudServiceGetExecutionHostsRequestMessage *>(message.get())) {
            processGetExecutionHosts(msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<CloudServiceCreateVMRequestMessage *>(message.get())) {
            processCreateVM(msg->answer_mailbox, msg->num_cores, msg->ram_memory, msg->property_list, msg->messagepayload_list);
            return true;

        } else if (auto msg = dynamic_cast<CloudServiceShutdownVMRequestMessage *>(message.get())) {
            processShutdownVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudServiceStartVMRequestMessage *>(message.get())) {
            processStartVM(msg->answer_mailbox, msg->vm_name, msg->pm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudServiceSuspendVMRequestMessage *>(message.get())) {
            processSuspendVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudServiceResumeVMRequestMessage *>(message.get())) {
            processResumeVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<VirtualizedClusterServiceMigrateVMRequestMessage *>(message.get())) {
            processMigrateVM(msg->answer_mailbox, msg->vm_name, msg->dest_pm_hostname);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
            processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
            processSubmitPilotJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
            return true;

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Process a VM migration request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM host
     * @param dest_pm_hostname: the name of the destination physical machine host
     *
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::processMigrateVM(const std::string &answer_mailbox, const std::string &vm_name,
                                                     const std::string &dest_pm_hostname) {


        WRENCH_INFO("Asked to migrate the VM %s to PM %s", vm_name.c_str(), dest_pm_hostname.c_str());

        VirtualizedClusterServiceMigrateVMAnswerMessage *msg_to_send_back;

        auto vm_pair = vm_list[vm_name];

        auto vm = vm_pair.first;

        // Check that the target host has sufficient resources
        double dest_available_ram = Simulation::getHostMemoryCapacity(dest_pm_hostname) - this->used_ram_per_execution_host[dest_pm_hostname];
        double dest_available_cores = Simulation::getHostNumCores(dest_pm_hostname) - this->used_cores_per_execution_host[dest_pm_hostname];
        if ((dest_available_ram < vm->getMemory()) or (dest_available_cores < vm->getNumCores())) {
            msg_to_send_back = new VirtualizedClusterServiceMigrateVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(new NotEnoughResources(nullptr, this)),
                    this->getMessagePayloadValueAsDouble(
                            VirtualizedClusterServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            // Do the migrationa
            vm->migrate(dest_pm_hostname);
            msg_to_send_back = new VirtualizedClusterServiceMigrateVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValueAsDouble(
                            VirtualizedClusterServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        try {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    msg_to_send_back);
        } catch (std::shared_ptr<NetworkError> &cause) {
            // ignore
        }
        return;
    }

}
