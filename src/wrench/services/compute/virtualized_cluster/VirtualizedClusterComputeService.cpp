/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <cfloat>
#include <numeric>
#include <utility>

#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeServiceMessage.h"
#include "wrench/services/compute/cloud/CloudComputeServiceMessage.h"
#include <wrench/services/compute/virtualized_cluster/VirtualizedClusterComputeService.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>


WRENCH_LOG_CATEGORY(wrench_core_virtualized_cluster_service, "Log category for Virtualized Cluster Service");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the hostcreate on which to start the service
     * @param execution_hosts: the hosts available for running virtual machines
     * @param scratch_space_mount_point: the mount of of the scratch space of the cloud service ("" means none)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     */
    VirtualizedClusterComputeService::VirtualizedClusterComputeService(const std::string &hostname,
                                                                       std::vector<std::string> &execution_hosts,
                                                                       const std::string &scratch_space_mount_point,
                                                                       WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                                       WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list)
        : CloudComputeService(hostname, execution_hosts, scratch_space_mount_point) {
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
     * @param num_cores: the desired number of cores in the VM
     * @param ram_memory: the desired memory size of the VM
     * @param physical_host: the physical host on which to create the VM
     * @param property_list: the property list for the underlying BareMetalComputeService
     * @param messagepayload_list: the message payload list for the underlying BareMetalComputeService
     *
     * @return A VM name
     */
    std::string
    VirtualizedClusterComputeService::createVM(unsigned long num_cores,
                                               double ram_memory,
                                               const std::string &physical_host,
                                               WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                               WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) {
        if (num_cores == ComputeService::ALL_CORES) {
            throw std::invalid_argument(
                    "CloudComputeService::createVM(): the VM's number of cores cannot be ComputeService::ALL_CORES");
        }
        if (ram_memory == ComputeService::ALL_RAM) {
            throw std::invalid_argument(
                    "CloudComputeService::createVM(): the VM's memory_manager_service requirement cannot be ComputeService::ALL_RAM");
        }

        assertServiceIsUp();

        if ((not physical_host.empty()) and (std::find(this->execution_hosts.begin(),
                                                       this->execution_hosts.end(),
                                                       physical_host) ==
                                             this->execution_hosts.end())) {
            throw std::invalid_argument("Trying to start a VM on an unknown (at least to this service) physical host");
        }

        // send a "create vm" message to the daemon's mailbox_name
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceCreateVMRequestMessage(
                        answer_mailbox,
                        num_cores, ram_memory, physical_host,
                        std::move(property_list), std::move(messagepayload_list),
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudComputeServiceCreateVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            } else {
                return msg->vm_name;
            }
        } else {
            throw std::runtime_error(
                    "CloudComputeService::createVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Synchronously migrate a VM to another physical host
     *
     * @param vm_name: virtual machine name
     * @param dest_pm_hostname: the name of the destination physical machine host
     *
     */
    void VirtualizedClusterComputeService::migrateVM(const std::string &vm_name, const std::string &dest_pm_hostname) {
        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument(
                    "VirtualizedClusterComputeService::migrateVM(): Unknown VM name '" + vm_name + "'");
        }

        // send a "migrate vm" message to the daemon's mailbox_name
        auto answer_mailbox = S4U_Daemon::getRunningActorRecvMailbox();

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new VirtualizedClusterComputeServiceMigrateVMRequestMessage(
                        answer_mailbox, vm_name, dest_pm_hostname,
                        this->getMessagePayloadValue(
                                VirtualizedClusterComputeServiceMessagePayload::MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<VirtualizedClusterComputeServiceMigrateVMAnswerMessage *>(
                    answer_message.get())) {
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "VirtualizedClusterComputeService::migrateVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
    * @brief Main method of the daemon
    *
    * @return 0 on termination
    */
    int VirtualizedClusterComputeService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);
        WRENCH_INFO(
                "Virtualized Cluster Service starting on host %s listening on mailbox_name %s",
                this->hostname.c_str(),
                this->mailbox->get_cname());

        // Start the Scratch Storage Service
        this->startScratchStorageService();

        /** Main loop **/
        while (this->processNextMessage()) {
            // no specific action
        }

        WRENCH_INFO("Virtualized Cluster Service on host %s cleanly terminating!",
                    S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     */
    bool VirtualizedClusterComputeService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox);
        } catch (ExecutionException &e) {
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->stopAllVMs(msg->send_failure_notifications, (ComputeService::TerminationCause)(msg->termination_cause));
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                CloudComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {
                return false;
            }
            return false;

        } else if (auto msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
            processGetResourceInformation(msg->answer_mailbox, msg->key);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceGetExecutionHostsRequestMessage *>(message.get())) {
            processGetExecutionHosts(msg->answer_mailbox);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceCreateVMRequestMessage *>(message.get())) {
            processCreateVM(msg->answer_mailbox, msg->num_cores, msg->ram_memory, msg->physical_host,
                            msg->property_list,
                            msg->messagepayload_list);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceShutdownVMRequestMessage *>(message.get())) {
            processShutdownVM(msg->answer_mailbox, msg->vm_name, msg->send_failure_notifications, msg->termination_cause);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceDestroyVMRequestMessage *>(message.get())) {
            processDestroyVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceStartVMRequestMessage *>(message.get())) {
            processStartVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceSuspendVMRequestMessage *>(message.get())) {
            processSuspendVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceResumeVMRequestMessage *>(message.get())) {
            processResumeVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<VirtualizedClusterComputeServiceMigrateVMRequestMessage *>(
                           message.get())) {
            processMigrateVM(msg->answer_mailbox, msg->vm_name, msg->dest_pm_hostname);
            return true;

        } else if (auto msg = dynamic_cast<ServiceHasTerminatedMessage *>(message.get())) {
            if (auto bmcs = std::dynamic_pointer_cast<BareMetalComputeService>(msg->service)) {
                processBareMetalComputeServiceTermination(bmcs, msg->exit_code);
            } else {
                throw std::runtime_error(
                        "VirtualizedClusterComputeService::processNextMessage(): Received a service termination message for an unknown BareMetalComputeService!");
            }
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
     */
    void
    VirtualizedClusterComputeService::processMigrateVM(simgrid::s4u::Mailbox *answer_mailbox, const std::string &vm_name,
                                                       const std::string &dest_pm_hostname) {
        WRENCH_INFO("Asked to migrate the VM %s to PM %s", vm_name.c_str(), dest_pm_hostname.c_str());

        VirtualizedClusterComputeServiceMigrateVMAnswerMessage *msg_to_send_back;

        auto vm_tuple = vm_list[vm_name];

        auto vm = std::get<0>(vm_tuple);

        // Check that the target host has sufficient resources
        double dest_available_ram = Simulation::getHostMemoryCapacity(dest_pm_hostname) -
                                    this->used_ram_per_execution_host[dest_pm_hostname];
        double dest_available_cores =
                (double) Simulation::getHostNumCores(dest_pm_hostname) - (double) this->used_cores_per_execution_host[dest_pm_hostname];
        if ((dest_available_ram < vm->getMemory()) or (dest_available_cores < (double) vm->getNumCores())) {
            msg_to_send_back = new VirtualizedClusterComputeServiceMigrateVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(
                            new NotEnoughResources(nullptr, this->getSharedPtr<VirtualizedClusterComputeService>())),
                    this->getMessagePayloadValue(
                            VirtualizedClusterComputeServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            // Do the migration
            vm->migrate(dest_pm_hostname);
            msg_to_send_back = new VirtualizedClusterComputeServiceMigrateVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValue(
                            VirtualizedClusterComputeServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        //        try {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                msg_to_send_back);
        //        } catch (ExecutionException &e) {
        //            // ignore
        //        }
    }

}// namespace wrench
