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

#include "CloudComputeServiceMessage.h"
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>
#include <wrench/services/compute/cloud/CloudComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>


WRENCH_LOG_CATEGORY(wrench_core_cloud_service, "Log category for Cloud Service");

namespace wrench {
    /** @brief VM ID sequence number */
    unsigned long CloudComputeService::VM_ID = 1;

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param execution_hosts: the list of the names of the hosts available for running virtual machines
     * @param scratch_space_mount_point: the mount point for the cloud sercvice's strach space ("" means none)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    CloudComputeService::CloudComputeService(const std::string &hostname,
                                             std::vector<std::string> execution_hosts,
                                             std::string scratch_space_mount_point,
                                             std::map<std::string, std::string> property_list,
                                             std::map<std::string, double> messagepayload_list) :
            ComputeService(hostname, "cloud_service", "cloud_service",
                           scratch_space_mount_point) {
        if (execution_hosts.empty()) {
            throw std::invalid_argument(
                    "CloudComputeService::CloudComputeService(): At least one execution host should be provided");
        }

        // Set default and specified properties
        this->setProperties(this->default_property_values, std::move(property_list));

        // Validate Properties
        validateProperties();

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        // Initialize internal data structures
        this->execution_hosts = execution_hosts;
        for (auto const &h : this->execution_hosts) {
            this->used_ram_per_execution_host[h] = 0;
            this->used_cores_per_execution_host[h] = 0;
        }
    }

    /**
     * @brief Destructor
     */
    CloudComputeService::~CloudComputeService() {
        this->default_property_values.clear();
        this->vm_list.clear();
    }

    /**
     * @brief Get the list of execution hosts available to run VMs
     *
     * @return a list of hostnames
     *
     * @throw ExecutionException
     */
    std::vector<std::string> CloudComputeService::getExecutionHosts() {
        assertServiceIsUp();

        // send a "get execution hosts" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_execution_hosts");

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceGetExecutionHostsRequestMessage(
                        answer_mailbox,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudComputeServiceGetExecutionHostsAnswerMessage *>(answer_message.get())) {
            return msg->execution_hosts;
        } else {
            throw std::runtime_error(
                    "CloudComputeService::sendRequest(): Received an unexpected [" + answer_message->getName() +
                    "] message!");
        }
    }

    /**
     * @brief Create a BareMetalComputeService VM (balances load on execution hosts)
     *
     * @param num_cores: the number of cores for the VM
     * @param ram_memory: the VM's RAM memory_manager_service capacity
     * @param property_list: a property list for the BareMetalComputeService that will run on the VM ({} means "use all defaults")
     * @param messagepayload_list: a message payload list for the BareMetalComputeService that will run on the VM ({} means "use all defaults")
     *
     * @return A VM name
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    std::string CloudComputeService::createVM(unsigned long num_cores,
                                              double ram_memory,
                                              std::map<std::string, std::string> property_list,
                                              std::map<std::string, double> messagepayload_list) {
        return this->createVM(num_cores, ram_memory, "", property_list, messagepayload_list);
    }

    /**
     * @brief Create a BareMetalComputeService VM (balances load on execution hosts)
     *
     * @param num_cores: the number of cores for the VM
     * @param ram_memory: the VM's RAM memory_manager_service capacity
     * @param desired_vm_name: the VM's desired name ("" means "pick a name for me")
     * @param property_list: a property list for the BareMetalComputeService that will run on the VM ({} means "use all defaults")
     * @param messagepayload_list: a message payload list for the BareMetalComputeService that will run on the VM ({} means "use all defaults")
     *
     * @return A VM name
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    std::string CloudComputeService::createVM(unsigned long num_cores,
                                              double ram_memory,
                                              std::string desired_vm_name,
                                              std::map<std::string, std::string> property_list,
                                              std::map<std::string, double> messagepayload_list) {
        if (num_cores == ComputeService::ALL_CORES) {
            throw std::invalid_argument(
                    "CloudComputeService::createVM(): the VM's number of cores cannot be ComputeService::ALL_CORES");
        }
        if (ram_memory == ComputeService::ALL_RAM) {
            throw std::invalid_argument(
                    "CloudComputeService::createVM(): the VM's memory_manager_service requirement cannot be ComputeService::ALL_RAM");
        }

        assertServiceIsUp();

        // send a "create vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("create_vm");

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceCreateVMRequestMessage(
                        answer_mailbox,
                        num_cores, ram_memory, desired_vm_name, property_list, messagepayload_list,
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
     * @brief Shutdown an active VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    void CloudComputeService::shutdownVM(const std::string &vm_name) {
        this->shutdownVM(vm_name, false, ComputeService::TerminationCause::TERMINATION_NONE);
    }


    /**
     * @brief Shutdown an active VM
     *
     * @param vm_name: the name of the VM
     * @param send_failure_notifications: whether to send the failure notifications
     * @param termination_cause: the termination cause (if failure notifications are sent)
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    void CloudComputeService::shutdownVM(const std::string &vm_name, bool send_failure_notifications, ComputeService::TerminationCause termination_cause) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::shutdownVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("shutdown_vm");

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceShutdownVMRequestMessage(
                        answer_mailbox, vm_name, send_failure_notifications, termination_cause,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudComputeServiceShutdownVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "CloudComputeService::shutdownVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Start a VM
     *
     * @param vm_name: the name of the VM
     *
     * @return A BareMetalComputeService that runs on the VM
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    std::shared_ptr<BareMetalComputeService> CloudComputeService::startVM(const std::string &vm_name) {
        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::startVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("start_vm");

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceStartVMRequestMessage(
                        answer_mailbox, vm_name, "",
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::START_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudComputeServiceStartVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
            return msg->cs;
        } else {
            throw std::runtime_error(
                    "CloudComputeService::startVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Get the compute service running on a VM, if any
     *
     * @param vm_name: the name of the VM
     *
     * @return A BareMetalComputeService that runs on the VM, or nullptr if none
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    std::shared_ptr<BareMetalComputeService> CloudComputeService::getVMComputeService(const std::string &vm_name) {
        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::startVM(): Unknown VM name '" + vm_name + "'");
        }
        return this->vm_list.at(vm_name).second;
    }

    /**
     * @brief Get the name of the physical host on which a VM is running
     *
     * @param vm_name: the name of the VM
     *
     * @return physical host name
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    std::string CloudComputeService::getVMPhysicalHostname(const std::string &vm_name) {
        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::startVM(): Unknown VM name '" + vm_name + "'");
        }
        return this->vm_list.at(vm_name).first->getPhysicalHostname();
    }

    /**
     * @brief Suspend a running VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    void CloudComputeService::suspendVM(const std::string &vm_name) {
        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::suspendVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("suspend_vm");

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceSuspendVMRequestMessage(
                        answer_mailbox, vm_name,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::SUSPEND_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudComputeServiceSuspendVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "CloudComputeService::suspendVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Resume a suspended VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    void CloudComputeService::resumeVM(const std::string &vm_name) {
        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::resumeVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("resume_vm");

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceResumeVMRequestMessage(
                        answer_mailbox, vm_name,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::RESUME_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudComputeServiceResumeVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
            // Got the expected reply
        } else {
            throw std::runtime_error(
                    "CloudComputeService::resumeVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Destroy a VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw ExecutionException
     * @throw std::invalid_argument
     */
    void CloudComputeService::destroyVM(const std::string &vm_name) {
        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::destroyVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("destroy_vm");

        std::shared_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudComputeServiceDestroyVMRequestMessage(
                        answer_mailbox, vm_name,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::DESTROY_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudComputeServiceDestroyVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "CloudComputeService::destroyVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Method to check whether a VM is currently running
     * @param vm_name: the name of the VM
     * @return true or false
     * @throw std::invalid_argument
     */
    bool CloudComputeService::isVMRunning(const std::string &vm_name) {
        auto vm_pair_it = this->vm_list.find(vm_name);

        if (vm_pair_it == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::isVMRunning(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        return (vm_pair_it->second.first->getState() == S4U_VirtualMachine::State::RUNNING);
    }

    /**
     * @brief Method to check whether a VM is currently down
     * @param vm_name: the name of the VM
     * @return true or false
     * @throw std::invalid_argument
     */
    bool CloudComputeService::isVMDown(const std::string &vm_name) {
        auto vm_pair_it = this->vm_list.find(vm_name);

        if (vm_pair_it == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::isVMDown(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        return (vm_pair_it->second.first->getState() == S4U_VirtualMachine::State::DOWN);
    }

    /**
     * @brief Method to check whether a VM is currently running
     * @param vm_name: the name of the VM
     * @return true or false
     * @throw std::invalid_argument
     */
    bool CloudComputeService::isVMSuspended(const std::string &vm_name) {
        auto vm_pair_it = this->vm_list.find(vm_name);

        if (vm_pair_it == this->vm_list.end()) {
            throw std::invalid_argument("CloudComputeService::isVMSuspended(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        return (vm_pair_it->second.first->getState() == S4U_VirtualMachine::State::SUSPENDED);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int CloudComputeService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

        WRENCH_INFO("Cloud Service starting on host %s listening on mailbox_name %s",
                    this->hostname.c_str(),
                    this->mailbox_name.c_str());

        /** Main loop **/
        while (this->processNextMessage()) {
            // no specific action
        }

        WRENCH_INFO("Cloud Service on host %s terminating cleanly!", S4U_Simulation::getHostName().c_str());
        return 0;
    }

    /**
     * @brief Send a message request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param message: message to be sent
     * @return a simulation message
     *
     * @throw std::runtime_error
     */
    std::shared_ptr<SimulationMessage> CloudComputeService::sendRequest(std::string &answer_mailbox,
                                                                        ComputeServiceMessage *message) {
        serviceSanityCheck();

        try {
            S4U_Mailbox::putMessage(this->mailbox_name, message);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        // Wait for a reply
        std::shared_ptr<SimulationMessage> answer_message = nullptr;

        try {
            answer_message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw ExecutionException(cause);
        }

        return answer_message;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool CloudComputeService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        if (message == nullptr) { WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting");
            return false;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
            this->stopAllVMs(msg->send_failure_notifications, (ComputeService::TerminationCause)(msg->termination_cause));
            this->vm_list.clear();
            // This is Synchronous
            try {
                S4U_Mailbox::putMessage(msg->ack_mailbox,
                                        new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                                                CloudComputeServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
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
            processCreateVM(msg->answer_mailbox, msg->num_cores, msg->ram_memory, msg->desired_vm_name,
                            msg->property_list,
                            msg->messagepayload_list);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceShutdownVMRequestMessage *>(message.get())) {
            processShutdownVM(msg->answer_mailbox, msg->vm_name, msg->send_failure_notifications, msg->termination_cause);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceStartVMRequestMessage *>(message.get())) {
            processStartVM(msg->answer_mailbox, msg->vm_name, msg->pm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceSuspendVMRequestMessage *>(message.get())) {
            processSuspendVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceResumeVMRequestMessage *>(message.get())) {
            processResumeVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<CloudComputeServiceDestroyVMRequestMessage *>(message.get())) {
            processDestroyVM(msg->answer_mailbox, msg->vm_name);
            return true;

        } else if (auto msg = dynamic_cast<ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage *>(message.get())) {
            processIsThereAtLeastOneHostWithAvailableResources(msg->answer_mailbox, msg->num_cores, msg->ram);
            return true;

        } else if (auto msg = dynamic_cast<ServiceHasTerminatedMessage *>(message.get())) {
            if (auto bmcs = std::dynamic_pointer_cast<BareMetalComputeService>(msg->service)) {
                processBareMetalComputeServiceTermination(bmcs, msg->exit_code);
            } else {
                throw std::runtime_error(
                        "CloudComputeService::processNextMessage(): Received a service termination message for "
                        "an unknown BareMetalComputeService!");
            }
            return true;

        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Process a execution host list request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void CloudComputeService::processGetExecutionHosts(const std::string &answer_mailbox) {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new CloudComputeServiceGetExecutionHostsAnswerMessage(
                        this->execution_hosts,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Create a BareMetalComputeService VM on a physical machine
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param requested_num_cores: the number of cores the service can use
     * @param requested_ram: the VM's RAM memory_manager_service capacity
     * @param desired_vm_name: the desired VM name ("" means "pick a name for me")
     * @param property_list: a property list for the BareMetalComputeService that will run on the VM ({} means "use all defaults")
     * @param messagepayload_list: a message payload list for the BareMetalComputeService that will run on the VM ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    void CloudComputeService::processCreateVM(const std::string &answer_mailbox,
                                              unsigned long requested_num_cores,
                                              double requested_ram,
                                              std::string desired_vm_name,
                                              std::map<std::string, std::string> property_list,
                                              std::map<std::string, double> messagepayload_list) {
        WRENCH_INFO("Asked to create a VM with %s cores and %s RAM",
                    (requested_num_cores == ComputeService::ALL_CORES ? "max" : std::to_string(
                            requested_num_cores)).c_str(),
                    (requested_ram == ComputeService::ALL_RAM ? "max" : std::to_string(requested_ram)).c_str());

        CloudComputeServiceCreateVMAnswerMessage *msg_to_send_back;

        // Check that there is at least one physical host that could support the VM
        bool found_a_host = false;
        for (auto const &host : this->execution_hosts) {
            auto total_num_cores = Simulation::getHostNumCores(host);
            auto total_ram = Simulation::getHostMemoryCapacity(host);
            if ((requested_num_cores <= total_num_cores) and (requested_ram <= total_ram)) {
                found_a_host = true;
                break;
            }
        }

        if (not found_a_host) { WRENCH_INFO("Not host on this service can accommodate this VM");
            std::string empty = std::string();
            msg_to_send_back =
                    new CloudComputeServiceCreateVMAnswerMessage(
                            false,
                            empty,
                            std::shared_ptr<FailureCause>(
                                    new NotEnoughResources(nullptr, this->getSharedPtr<CloudComputeService>())),
                            this->getMessagePayloadValue(
                                    CloudComputeServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            // Pick a VM name (and being paranoid about mistakenly picking an actual hostname!)
            std::string vm_name;
            std::string error_msg;

            if (desired_vm_name.empty()) {
                do {
                    vm_name = this->getName() + "_vm" + std::to_string(CloudComputeService::VM_ID++);
                } while (S4U_Simulation::hostExists(vm_name));
            } else {
                if (this->vm_list.find(desired_vm_name) == this->vm_list.end()) {
                    vm_name = desired_vm_name;
                } else {
                    error_msg = "Desired VM name already in use";
                }
            }

            if (vm_name.empty()) {
                std::string empty = std::string();

                msg_to_send_back = new CloudComputeServiceCreateVMAnswerMessage(
                        false,
                        empty,
                        std::shared_ptr<FailureCause>(
                                new NotAllowed(this->getSharedPtr<CloudComputeService>(), error_msg)),
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD));

            } else {
                // Create the VM
                auto vm = std::shared_ptr<S4U_VirtualMachine>(
                        new S4U_VirtualMachine(vm_name, requested_num_cores, requested_ram, property_list,
                                               messagepayload_list));

                WRENCH_INFO("Created a VM called %s with %lu cores and %lf RAM",
                            vm_name.c_str(), requested_num_cores, requested_ram);

                // Add the VM to the list of VMs, with (for now) a nullptr compute service
                this->vm_list[vm_name] = std::make_pair(vm, nullptr);

                msg_to_send_back = new CloudComputeServiceCreateVMAnswerMessage(
                        true,
                        vm_name,
                        nullptr,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD));
            }
        }

        // Send reply
        S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
    }

    /**
     * @brief: Process a VM shutdown request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     * @param send_failure_notifications: whether to send failure notifications
     * @param termination_cause: termination cause (if failure notifications are sent)
     */
    void CloudComputeService::processShutdownVM(const std::string &answer_mailbox,
                                                const std::string &vm_name,
                                                bool send_failure_notifications,
                                                ComputeService::TerminationCause termination_cause) {
        WRENCH_INFO("Asked to shutdown VM %s", vm_name.c_str());

        CloudComputeServiceShutdownVMAnswerMessage *msg_to_send_back;

        auto vm_pair = *(this->vm_list.find(vm_name));
        auto vm = vm_pair.second.first;
        auto cs = vm_pair.second.second;

        if (vm->getState() != S4U_VirtualMachine::State::RUNNING) {
            std::string error_message("Cannot shutdown a VM that is not running");
            msg_to_send_back = new CloudComputeServiceShutdownVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(
                            new NotAllowed(this->getSharedPtr<CloudComputeService>(), error_message)),
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            std::string pm = vm->getPhysicalHostname();
            // Stop the Compute Service
            cs->stop(send_failure_notifications, termination_cause);
            // We do not shut down the VM. This will be done when the CloudComputeService is notified
            // of the BareMetalComputeService completion.
            vm->shutdown();

            // Update internal data structures
            this->used_ram_per_execution_host[pm] -= vm->getMemory();
            this->used_cores_per_execution_host[pm] -= vm->getNumCores();

            msg_to_send_back = new CloudComputeServiceShutdownVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
    }

    /**
     * @brief A helper method that finds a host with a given number of available cores and a given amount of
     *        available RAM using one of the several resource allocation algorithms.
     * @param desired_num_cores: desired number of cores
     * @param desired_ram: desired amount of RAM
     * @oaram desired_host: name of a desired host ("" if none)
     * @return
     */
    std::string CloudComputeService::findHost(unsigned long desired_num_cores,
                                              double desired_ram,
                                              std::string desired_host) {
        // Find a physical host to start the VM
        std::vector<std::string> possible_hosts;
        for (auto const &host : this->execution_hosts) {
            if ((not desired_host.empty()) and (host != desired_host)) {
                continue;
            }

            // Check that host is up
            if (not Simulation::isHostOn(host)) {
                continue;
            }

            // Check that host has a non-zero compute speed
            if (Simulation::getHostFlopRate(host) <= 0) {
                continue;
            }

            // Check for RAM
            auto total_ram = Simulation::getHostMemoryCapacity(host);
            auto available_ram = total_ram - this->used_ram_per_execution_host[host];
            if (desired_ram > available_ram) {
                continue;
            }

            // Check for cores
            auto total_num_cores = Simulation::getHostNumCores(host);
            auto num_available_cores = total_num_cores - this->used_cores_per_execution_host[host];
            if (desired_num_cores > num_available_cores) {
                continue;
            }

            possible_hosts.push_back(host);
        }

        // Did we find a viable host?
        if (possible_hosts.empty()) {
            return "";
        }

        std::string vm_resource_allocation_algorithm = this->getPropertyValueAsString(
                CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM);
        if (vm_resource_allocation_algorithm == "first-fit") {
            //don't sort the possible hosts

        } else if (vm_resource_allocation_algorithm == "best-fit-ram-first") {
            // Sort the possible hosts to implement best fit (using RAM first)
            std::sort(possible_hosts.begin(), possible_hosts.end(),
                      [](std::string const &a, std::string const &b) {
                          unsigned long a_num_cores = Simulation::getHostNumCores(a);
                          double a_ram = Simulation::getHostMemoryCapacity(a);
                          unsigned long b_num_cores = Simulation::getHostNumCores(b);
                          double b_ram = Simulation::getHostMemoryCapacity(b);

                          if (a_ram != b_ram) {
                              return a_ram < b_ram;
                          } else if (a_num_cores < b_num_cores) {
                              return a_num_cores < b_num_cores;
                          } else {
                              return a < b;  // string order
                          }
                      });

        } else if (vm_resource_allocation_algorithm == "best-fit-cores-first") {
            // Sort the possible hosts to implement best fit (using cores first)
            std::sort(possible_hosts.begin(), possible_hosts.end(),
                      [](std::string const &a, std::string const &b) {
                          unsigned long a_num_cores = Simulation::getHostNumCores(a);
                          double a_ram = Simulation::getHostMemoryCapacity(a);
                          unsigned long b_num_cores = Simulation::getHostNumCores(b);
                          double b_ram = Simulation::getHostMemoryCapacity(b);

                          if (a_num_cores != b_num_cores) {
                              return a_num_cores < b_num_cores;
                          } else if (a_ram < b_ram) {
                              return a_ram < b_ram;
                          } else {
                              return a < b;  // string order
                          }
                      });
        }

        auto picked_host = *(possible_hosts.begin());
        return picked_host;
    }

    /**
     * @brief: Process a VM start request by startibnng a VM on a host (using best fit for RAM first, and then for cores)
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     * @param pm_name: the name of the physical host on which to start the VM (empty string if up to the service to pick a host)
     */
    void CloudComputeService::processStartVM(const std::string &answer_mailbox,
                                             const std::string &vm_name,
                                             const std::string &pm_name) {
        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudComputeServiceStartVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::DOWN) {
            std::string error_message("Cannot start a VM that is not down");
            msg_to_send_back = new CloudComputeServiceStartVMAnswerMessage(
                    false,
                    nullptr,
                    std::shared_ptr<FailureCause>(
                            new NotAllowed(this->getSharedPtr<CloudComputeService>(), error_message)),
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            std::string picked_host = this->findHost(vm->getNumCores(), vm->getMemory(), pm_name);

            // Did we find a viable host?
            if (picked_host.empty()) { WRENCH_INFO("Not enough resources to start the VM");
                msg_to_send_back =
                        new CloudComputeServiceStartVMAnswerMessage(
                                false,
                                nullptr,
                                std::shared_ptr<FailureCause>(
                                        new NotEnoughResources(nullptr, this->getSharedPtr<CloudComputeService>())),
                                this->getMessagePayloadValue(
                                        CloudComputeServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD));

            } else { WRENCH_INFO("Starting VM %s on host %s", vm_name.c_str(), picked_host.c_str());

                // Sleep for the VM booting overhead
                Simulation::sleep(
                        this->getPropertyValueAsDouble(CloudComputeServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS));

                // Start the actual vm
                vm->start(picked_host);

                // Create the Compute Service if needed
                if ((vm_pair.second == nullptr) or (not vm_pair.second->isUp())){

                    // Create the resource set for the BareMetalComputeService
                    std::map<std::string, std::tuple<unsigned long, double>> compute_resources = {
                            std::make_pair(vm_name, std::make_tuple(vm->getNumCores(), vm->getMemory()))};

                    // Create the BareMetal service, whose main daemon is on this (stable) host
                    auto plist = vm->getPropertyList();
                    plist.insert(std::make_pair(BareMetalComputeServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH,"true"));
                    plist.insert(std::make_pair(BareMetalComputeServiceProperty::TERMINATE_WHENEVER_ALL_RESOURCES_ARE_DOWN,"true"));

                    std::shared_ptr<BareMetalComputeService> cs = std::shared_ptr<BareMetalComputeService>(
                            new BareMetalComputeService(this->hostname,
                                                        compute_resources,
                                                        plist,
                                                        vm->getMessagePayloadList(),
                                                        getScratch()));
                    cs->simulation = this->simulation;
                    std::get<1>(this->vm_list[vm_name]) = cs;
                }

                // Update internal data structures
                this->used_ram_per_execution_host[picked_host] += vm->getMemory();
                this->used_cores_per_execution_host[picked_host] += vm->getNumCores();

                // Start the service
                try {
                    std::get<1>(this->vm_list[vm_name])->start(std::get<1>(this->vm_list[vm_name]), true,
                                                               false); // Daemonized
                } catch (std::runtime_error &e) {
                    throw; // This shouldn't happen
                }

                // Create a failure detector for the service
                auto termination_detector = std::shared_ptr<ServiceTerminationDetector>(
                        new ServiceTerminationDetector(this->hostname, std::get<1>(this->vm_list[vm_name]),
                                                       this->mailbox_name, false, true));
                termination_detector->setSimulation(this->simulation);
                termination_detector->start(termination_detector, true, false); // Daemonized, no auto-restart

                msg_to_send_back = new CloudComputeServiceStartVMAnswerMessage(
                        true,
                        std::get<1>(this->vm_list[vm_name]),
                        nullptr,
                        this->getMessagePayloadValue(
                                CloudComputeServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD));
            }
        }

        S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
    }

    /**
     * @brief: Process a VM suspend request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     */
    void CloudComputeService::processSuspendVM(const std::string &answer_mailbox,
                                               const std::string &vm_name) {
        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudComputeServiceSuspendVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::RUNNING) {
            std::string error_message("Cannot suspend a VM that is not running");
            msg_to_send_back = new CloudComputeServiceSuspendVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(
                            new NotAllowed(this->getSharedPtr<CloudComputeService>(), error_message)),
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {

            WRENCH_INFO("Asked to suspend VM %s", vm_name.c_str());

            auto vm_tuple = this->vm_list.find(vm_name);

            auto cs = std::get<1>(vm_tuple->second);
            cs->suspend();

            auto vm = std::get<0>(vm_tuple->second);
            vm->suspend();

            msg_to_send_back =
                    new CloudComputeServiceSuspendVMAnswerMessage(
                            true,
                            nullptr,
                            this->getMessagePayloadValue(
                                    CloudComputeServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
    }

    /**
     * @brief: Process a VM resume request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     */
    void CloudComputeService::processResumeVM(const std::string &answer_mailbox,
                                              const std::string &vm_name) {
        WRENCH_INFO("Asked to resume VM %s", vm_name.c_str());
        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudComputeServiceResumeVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::SUSPENDED) {
            std::string error_message("Cannot resume a VM that is not suspended");
            msg_to_send_back = new CloudComputeServiceResumeVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(
                            new NotAllowed(this->getSharedPtr<CloudComputeService>(), error_message)),
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            vm->resume();
            auto cs = vm_pair.second;
            cs->resume();

            msg_to_send_back = new CloudComputeServiceResumeVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
    }

    /**
    * @brief: Process a VM destruction request
    *
    * @param answer_mailbox: the mailbox to which the answer message should be sent
    * @param vm_name: the name of the VM
    */
    void CloudComputeService::processDestroyVM(const std::string &answer_mailbox,
                                               const std::string &vm_name) {
        WRENCH_INFO("Asked to destroy VM %s", vm_name.c_str());
        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudComputeServiceDestroyVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::DOWN) {
            std::string error_message("Cannot destroy a VM that is not down");
            msg_to_send_back = new CloudComputeServiceDestroyVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(
                            new NotAllowed(this->getSharedPtr<CloudComputeService>(), error_message)),
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::DESTROY_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            this->vm_list.erase(vm_name);
            msg_to_send_back = new CloudComputeServiceDestroyVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValue(
                            CloudComputeServiceMessagePayload::DESTROY_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
    }


    /**
     * @brief Process a "get resource information message"
     * @param answer_mailbox: the mailbox to which the description message should be sent
     * @param key: the desired resource information (i.e., dictionary key) that's needed)
     */
    void CloudComputeService::processGetResourceInformation(const std::string &answer_mailbox,
                                                            const std::string &key) {
        // Build a dictionary
        std::map<std::string, double> dict;

        if (key == "num_hosts") {

            // Num hosts
            dict.insert(std::make_pair(this->getName(), (double) (this->vm_list.size())));

        } else if (key == "num_cores") {

            // Total num cores
            for (auto &host : this->execution_hosts) {
                unsigned long total_num_cores = Simulation::getHostNumCores(host);
                dict.insert(std::make_pair(host, (double) total_num_cores));
            }

        } else if (key == "num_idle_cores") {

            for (auto &host : this->execution_hosts) {
                // Total num cores
                unsigned long total_num_cores = Simulation::getHostNumCores(host);
                // Idle cores
                unsigned long num_cores_allocated_to_vms = 0;
                for (auto &vm_pair : this->vm_list) {
                    auto actual_vm = vm_pair.second.first;
                    if ((actual_vm->getState() != S4U_VirtualMachine::DOWN) and
                        (actual_vm->getPhysicalHostname() == host)) {
                        num_cores_allocated_to_vms += actual_vm->getNumCores();
                    }
                }
                dict.insert(std::make_pair(host, (double) (total_num_cores - num_cores_allocated_to_vms)));
            }
        } else if (key == "flop_rates") {

            for (auto &host : this->execution_hosts) {
                double flop_rate = Simulation::getHostFlopRate(host);
                dict.insert(std::make_pair(host, flop_rate));
            }

        } else if (key == "ram_capacities") {

            for (auto &host : this->execution_hosts) {
                double total_ram = Simulation::getHostMemoryCapacity(host);
                dict.insert(std::make_pair(host, total_ram));
            }

        } else if (key == "ram_availabilities") {

            for (auto &host : this->execution_hosts) {
                double total_ram = Simulation::getHostMemoryCapacity(host);
                // Available RAM
                double ram_allocated_to_vms = 0;
                for (auto &vm_pair : this->vm_list) {
                    auto actual_vm = vm_pair.second.first;
                    if ((actual_vm->getState() != S4U_VirtualMachine::DOWN) and
                        (actual_vm->getPhysicalHostname() == host)) {
                        ram_allocated_to_vms += actual_vm->getMemory();
                    }
                }
                dict.insert(std::make_pair(host, total_ram - ram_allocated_to_vms));
            }

        } else if (key == "ttl") {

            dict.insert(std::make_pair(this->getName(), DBL_MAX));

        } else {
            throw std::runtime_error("CloudComputeService::processGetResourceInformation(): unknown key");
        }

        // Send the reply
        auto *answer_message = new ComputeServiceResourceInformationAnswerMessage(
                dict,
                this->getMessagePayloadValue(
                        ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
    }

    /**
     * @brief Terminate all VMs.
     * @param send_failure_notifications: whether to send failure notifications
     * @param termination_cause: the termination cause (if failure notifications are sent)
     */
    void CloudComputeService::stopAllVMs(bool send_failure_notifications, ComputeService::TerminationCause termination_cause) {
        WRENCH_INFO("Stopping Cloud Service");
        for (auto &vm : this->vm_list) {
            auto actual_vm = vm.second.first;
            auto cs = vm.second.second;
            switch (actual_vm->getState()) {
                case S4U_VirtualMachine::State::DOWN:
                    // Nothing to do
                    break;
                case S4U_VirtualMachine::State::SUSPENDED:
                    // Resume the VM
                    actual_vm->resume();
                    cs->resume();
                    // WARNING: No the lack of a "break" below which is a hacl
                case S4U_VirtualMachine::State::RUNNING:
                    // Shut it down
                    std::string pm = actual_vm->getPhysicalHostname();
                    // Stop the Compute Service
                    cs->stop(send_failure_notifications, termination_cause);
                    // Shutdown the VM
                    actual_vm->shutdown();
                    // Update internal data structures
                    this->used_ram_per_execution_host[pm] -= actual_vm->getMemory();
                    this->used_cores_per_execution_host[pm] -= actual_vm->getNumCores();
                    break;
            }
        }
    }

    /**
     * @brief Process a host available resource request
     * @param answer_mailbox: the answer mailbox
     * @param num_cores: the desired number of cores
     * @param ram: the desired RAM
     */
    void CloudComputeService::processIsThereAtLeastOneHostWithAvailableResources(
            const std::string &answer_mailbox, unsigned long num_cores, double ram) {
        throw std::runtime_error(
                "CloudComputeService::processIsThereAtLeastOneHostWithAvailableResources(): A Cloud service does not support this operation");
    }


    /**
     * @brief Process a termination by a previously started BareMetalComputeService on a VM
     * @param cs: the service that has terminated
     * @param exit_code: the service's exit code
     */
    void CloudComputeService::processBareMetalComputeServiceTermination(std::shared_ptr<BareMetalComputeService> cs,
                                                                        int exit_code) {
        std::string vm_name;
        for (auto const &vm_pair : this->vm_list) {
            if (vm_pair.second.second == cs) {
                vm_name = vm_pair.first;
                break;
            }
        }

        if (vm_name.empty()) {
            throw std::runtime_error(
                    "CloudComputeService::processBareMetalComputeServiceTermination(): received a termination notification for an unknown BareMetalComputeService");
        }
        unsigned long used_cores = this->vm_list[vm_name].first->getNumCores();
        double used_ram = this->vm_list[vm_name].first->getMemory();
        std::string pm_name = this->vm_list[vm_name].first->getPhysicalHostname();
//        WRENCH_INFO("GOT A DEATH NOTIFICATION: %s %ld %lf (exit_code = %d)",
//                    pm_name.c_str(), used_cores, used_ram, exit_code);

        if (this->vm_list[vm_name].first->getState() != S4U_VirtualMachine::State::DOWN) {
            this->vm_list[vm_name].first->shutdown();
        }
        this->used_cores_per_execution_host[pm_name] -= used_cores;
        this->used_ram_per_execution_host[pm_name] -= used_ram;
    }

    /**
     * @brief Validate the service's properties
     *
     * @throw std::invalid_argument
     */
    void CloudComputeService::validateProperties() {

        // VM Boot overhead
        bool success = true;
        double vm_boot_overhead = 0;
        try {
            vm_boot_overhead = this->getPropertyValueAsDouble(CloudComputeServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS);
        } catch (std::invalid_argument &e) {
            success = false;
        }

        if ((!success) or (vm_boot_overhead < 0)) {
            throw std::invalid_argument("Invalid VM_BOOT_OVERHEAD_IN_SECONDS property specification: " +
                                        this->getPropertyValueAsString(
                                                CloudComputeServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS));
        }

        // VM resource allocation algorithm
        std::string vm_resource_allocation_algorithm = this->getPropertyValueAsString(
                CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM);
        if (vm_resource_allocation_algorithm != "first-fit" and
            vm_resource_allocation_algorithm != "best-fit-ram-first" and
            vm_resource_allocation_algorithm != "best-fit-cores-first") {
            throw std::invalid_argument("Inalid VM_RESOURCE_ALLOCATION_ALGORITHM property specification: " +
                                        vm_resource_allocation_algorithm);
        }
    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool CloudComputeService::supportsStandardJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool CloudComputeService::supportsCompoundJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool CloudComputeService::supportsPilotJobs() {
        return false;
    }

}
