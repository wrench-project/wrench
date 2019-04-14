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

#include "CloudServiceMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/cloud/CloudService.h"
#include "wrench/services/compute/bare_metal/BareMetalComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_service, "Log category for Cloud Service");

namespace wrench {

    /** @brief VM ID sequence number */
    unsigned long CloudService::VM_ID = 1;

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param execution_hosts: the list of the names of the hosts available for running virtual machines
     * @param scratch_space_size: the size for the scratch storage pace of the cloud service
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    CloudService::CloudService(const std::string &hostname,
                               std::vector<std::string> &execution_hosts,
                               double scratch_space_size,
                               std::map<std::string, std::string> property_list,
                               std::map<std::string, std::string> messagepayload_list) :
            ComputeService(hostname, "cloud_service", "cloud_service",
                           scratch_space_size) {

        if (execution_hosts.empty()) {
            throw std::invalid_argument(
                    "CloudService::CloudService(): At least one execution host should be provided");
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
    CloudService::~CloudService() {
        this->default_property_values.clear();
        this->vm_list.clear();
    }

    /**
     * @brief Get the list of execution hosts available to run VMs
     *
     * @return a list of hostnames
     *
     * @throw WorkflowExecutionException
     */
    std::vector<std::string> CloudService::getExecutionHosts() {

        assertServiceIsUp();

        // send a "get execution hosts" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_execution_hosts");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceGetExecutionHostsRequestMessage(
                        answer_mailbox,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudServiceGetExecutionHostsAnswerMessage *>(answer_message.get())) {
            return msg->execution_hosts;
        } else {
            throw std::runtime_error(
                    "CloudService::sendRequest(): Received an unexpected [" + answer_message->getName() + "] message!");
        }
    }

    /**
     * @brief Create a BareMetalComputeService VM (balances load on execution hosts)
     *
     * @param num_cores: the number of cores for the VM
     * @param ram_memory: the VM's RAM memory capacity
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @return A VM name
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    std::string CloudService::createVM(unsigned long num_cores,
                                       double ram_memory) {


        if (num_cores == ComputeService::ALL_CORES) {
            throw std::invalid_argument("CloudService::createVM(): the VM's number of cores cannot be ComputeService::ALL_CORES");
        }
        if (ram_memory == ComputeService::ALL_RAM) {
            throw std::invalid_argument("CloudService::createVM(): the VM's memory requirement cannot be ComputeService::ALL_RAM");
        }

        assertServiceIsUp();

        // send a "create vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("create_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceCreateVMRequestMessage(
                        answer_mailbox,
                        num_cores, ram_memory,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudServiceCreateVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            } else {
                return msg->vm_name;
            }
        } else {
            throw std::runtime_error("CloudService::createVM(): Unexpected [" + answer_message->getName() + "] message");
        }
    }

    /**
     * @brief Shutdown an active VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void CloudService::shutdownVM(const std::string &vm_name) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudService::shutdownVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("shutdown_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceShutdownVMRequestMessage(
                        answer_mailbox, vm_name,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudServiceShutdownVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error("CloudService::shutdownVM(): Unexpected [" + answer_message->getName() + "] message");
        }
        return;
    }

    /**
     * @brief Start a VM
     *
     * @param vm_name: the name of the VM
     *
     * @return A BareMetalComputeService that runs on the VM
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    std::shared_ptr<BareMetalComputeService> CloudService::startVM(const std::string &vm_name) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudService::startVM(): Unknown VM name '" + vm_name + "'");
        }


        assertServiceIsUp();

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("start_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceStartVMRequestMessage(
                        answer_mailbox, vm_name, "",
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
     * @brief Suspend a running VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void CloudService::suspendVM(const std::string &vm_name) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudService::suspendVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("suspend_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceSuspendVMRequestMessage(
                        answer_mailbox, vm_name,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::SUSPEND_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudServiceSuspendVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error("CloudService::suspendVM(): Unexpected [" + answer_message->getName() + "] message");
        }
        return;
    }

    /**
     * @brief Resume a suspended VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void CloudService::resumeVM(const std::string &vm_name) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudService::resumeVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("resume_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceResumeVMRequestMessage(
                        answer_mailbox, vm_name,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::RESUME_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudServiceResumeVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
            // Got the expected reply
        } else {
            throw std::runtime_error("CloudService::resumeVM(): Unexpected [" + answer_message->getName() + "] message");
        }
        return;
    }

    /**
     * @brief Destroy a VM
     *
     * @param vm_name: the name of the VM
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     */
    void CloudService::destroyVM(const std::string &vm_name) {

        if (this->vm_list.find(vm_name) == this->vm_list.end()) {
            throw std::invalid_argument("CloudService::resumeVM(): Unknown VM name '" + vm_name + "'");
        }

        assertServiceIsUp();

        // send a "shutdown vm" message to the daemon's mailbox_name
        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("destroy_vm");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new CloudServiceDestroyVMRequestMessage(
                        answer_mailbox, vm_name,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::DESTROY_VM_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<CloudServiceDestroyVMAnswerMessage *>(answer_message.get())) {
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error("CloudService::destroyVM(): Unexpected [" + answer_message->getName() + "] message");
        }
        return;
    }

    /**
     * @brief Submit a standard job to the cloud service
     *
     * @param job: a standard job
     * @param service_specific_args: batch-specific arguments
     *      - optional: "-vm": name of vm on which to start the job
     *        (if not provided, the service will pick the vm)
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void CloudService::submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_args) {

        assertServiceIsUp();

        for (auto const &arg : service_specific_args) {
            if (arg.first != "-vm") {
                throw std::invalid_argument("CloudService::submitStandardJob(): Invalid service-specific argument key '" + arg.first + "'");
            }
            if (this->vm_list.find(arg.second) == this->vm_list.end()) {
                throw std::invalid_argument("CloudService::submitStandardJob(): In service-specific argument value: Unknown VM name '" + arg.second + "'");
            }
        }


        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new ComputeServiceSubmitStandardJobRequestMessage(
                        answer_mailbox, job, service_specific_args,
                        this->getMessagePayloadValueAsDouble(
                                ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(answer_message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            }
        } else {
            throw std::runtime_error(
                    "ComputeService::submitStandardJob(): Received an unexpected [" + msg->getName() + "] message!");
        }
    }

    /**
     * @brief Asynchronously submit a pilot job to the cloud service
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments
     *      - optional: "-vm": name of vm on which to start the job
     *        (if not provided, the service will pick the vm)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void CloudService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_args) {

        assertServiceIsUp();

        for (auto const &arg : service_specific_args) {
            if (arg.first != "-vm") {
                throw std::invalid_argument("CloudService::submitPilotJob(): Invalid service-specific argument key '" + arg.first + "'");
            }
            if (this->vm_list.find(arg.second) == this->vm_list.end()) {
                throw std::invalid_argument("CloudService::submitPilotJob(): In service-specific argument value: Unknown VM name '" + arg.second + "'");
            }
        }

        std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

        std::unique_ptr<SimulationMessage> answer_message = sendRequest(
                answer_mailbox,
                new ComputeServiceSubmitPilotJobRequestMessage(
                        answer_mailbox, job, service_specific_args, this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));

        if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(answer_message.get())) {
            // If no success, throw an exception
            if (not msg->success) {
                throw WorkflowExecutionException(msg->failure_cause);
            } else {
                return;
            }
        } else {
            throw std::runtime_error(
                    "CloudService::submitPilotJob(): Received an unexpected [" + msg->getName() + "] message!");
        }
    }

    /**
     * @brief Terminate a standard job to the compute service (virtual)
     * @param job: the standard job
     *
     * @throw std::runtime_error
     */
    void CloudService::terminateStandardJob(StandardJob *job) {
        throw std::runtime_error("CloudService::terminateStandardJob(): Not implemented!");
    }

    /**
     * @brief non-implemented
     * @param job: a pilot job to (supposedly) terminate
     */
    void CloudService::terminatePilotJob(PilotJob *job) {
        throw std::runtime_error(
                "CloudService::terminatePilotJob(): not implemented because CloudService never supports pilot jobs");
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int CloudService::main() {

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
    std::unique_ptr<SimulationMessage>
    CloudService::sendRequest(std::string &answer_mailbox, ComputeServiceMessage *message) {

        serviceSanityCheck();

        try {
            S4U_Mailbox::dputMessage(this->mailbox_name, message);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }

        // Wait for a reply
        std::unique_ptr<SimulationMessage> answer_message = nullptr;

        try {
            answer_message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
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
    bool CloudService::processNextMessage() {

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
            processCreateVM(msg->answer_mailbox, msg->num_cores, msg->ram_memory);
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

        } else if (auto msg = dynamic_cast<CloudServiceDestroyVMRequestMessage *>(message.get())) {
            processDestroyVM(msg->answer_mailbox, msg->vm_name);
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
     * @brief Process a execution host list request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void CloudService::processGetExecutionHosts(const std::string &answer_mailbox) {

        try {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new CloudServiceGetExecutionHostsAnswerMessage(
                            this->execution_hosts,
                            this->getMessagePayloadValueAsDouble(
                                    CloudServiceMessagePayload::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
            return;
        }
    }

    /**
     * @brief Create a BareMetalComputeService VM on a physical machine
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param requested_num_cores: the number of cores the service can use
     * @param requested_ram: the VM's RAM memory capacity
     *
     * @throw std::runtime_error
     */
    void CloudService::processCreateVM(const std::string &answer_mailbox,
                                       unsigned long requested_num_cores,
                                       double requested_ram) {


        WRENCH_INFO("Asked to create a VM with %s cores and %s RAM",
                    (requested_num_cores == ComputeService::ALL_CORES ? "max" : std::to_string(requested_num_cores)).c_str(),
                    (requested_ram == ComputeService::ALL_RAM ? "max" : std::to_string(requested_ram)).c_str());

        CloudServiceCreateVMAnswerMessage *msg_to_send_back;

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

        if (not found_a_host) {
            WRENCH_INFO("Not host on this service can accommodate this VM");
            std::string empty= std::string();
            msg_to_send_back =
                    new CloudServiceCreateVMAnswerMessage(
                            false,
                            empty,
                            std::shared_ptr<FailureCause>(new NotEnoughResources(nullptr, this)),
                            this->getMessagePayloadValueAsDouble(
                                    CloudServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD));
        } else {

            // Pick a VM name (and being paranoid about mistakenly picking an actual hostname!)
            std::string vm_name;
            do {
                vm_name = this->getName() + "_vm" + std::to_string(CloudService::VM_ID++);
            } while (simgrid::s4u::Host::by_name_or_null(vm_name) != nullptr);

            // Create the VM
            auto vm = std::shared_ptr<S4U_VirtualMachine>(new S4U_VirtualMachine(vm_name, requested_num_cores, requested_ram));

            // Add the VM to the list of VMs, with (for now) a nullptr compute service
            this->vm_list[vm_name] = std::make_pair(vm, nullptr);

            msg_to_send_back = new CloudServiceCreateVMAnswerMessage(
                    true,
                    vm_name,
                    nullptr,
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        // Send reply
        try {
            S4U_Mailbox::dputMessage(answer_mailbox,msg_to_send_back);
        } catch (std::shared_ptr<NetworkError> &cause) {
            // Ignore
        }
        return;
    }

    /**
     * @brief: Process a VM shutdown request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     */
    void CloudService::processShutdownVM(const std::string &answer_mailbox, const std::string &vm_name) {

        WRENCH_INFO("Asked to shutdown VM %s", vm_name.c_str());

        CloudServiceShutdownVMAnswerMessage *msg_to_send_back;

        auto vm_pair = *(this->vm_list.find(vm_name));
        auto vm = vm_pair.second.first;
        auto cs = vm_pair.second.second;

        if (vm->getState() != S4U_VirtualMachine::State::RUNNING) {

            std::string error_message("Cannot shutdown a VM that is not running");
            msg_to_send_back =  new CloudServiceShutdownVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(new NotAllowed(this, error_message)),
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD));
        } else {

            // Stop the Compute Service
            cs->stop();
            // Shutdown the VM
            vm->shutdown();

            // Update internal data structures
            std::string pm = vm->getPhysicalHostname();
            this->used_ram_per_execution_host[pm] -= vm->getMemory();
            this->used_cores_per_execution_host[pm] -= vm->getNumCores();

            msg_to_send_back = new CloudServiceShutdownVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        try {
            S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
        } catch (std::shared_ptr<NetworkError> &cause) {
            // ignore
        }
        return;
    }

    /**
     * @brief: Process a VM start request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     * @param pm_name: the name of the physical host on which to start the VM (empty string if up to the service to pick a host)
     */
    void CloudService::processStartVM(const std::string &answer_mailbox, const std::string &vm_name, const std::string &pm_name) {

        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudServiceStartVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::DOWN) {

            std::string error_message("Cannot start a VM that is not down");
            msg_to_send_back =  new CloudServiceStartVMAnswerMessage(
                    false,
                    nullptr,
                    std::shared_ptr<FailureCause>(new NotAllowed(this, error_message)),
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {

            // Find a physical host to start the VM
            std::string picked_host = "";

            for (auto const &host : this->execution_hosts) {

                if ((not pm_name.empty()) and (host != pm_name)) {
                    continue;
                }

                // Check for RAM
                auto total_ram = Simulation::getHostMemoryCapacity(host);
                auto available_ram = total_ram - this->used_ram_per_execution_host[host];
                if (vm->getMemory() > available_ram) {
                    continue;
                }

                // Check for cores
                auto total_num_cores = Simulation::getHostNumCores(host);
                auto num_available_cores = total_num_cores - this->used_cores_per_execution_host[host];
                if (vm->getNumCores() > num_available_cores) {
                    continue;
                }

                picked_host = host;
                break;
            }

            // Did we find a viable host?
            if (picked_host.empty()) {
                WRENCH_INFO("Not enough resources to create the VM");
                msg_to_send_back =
                        new CloudServiceStartVMAnswerMessage(
                                false,
                                nullptr,
                                std::shared_ptr<FailureCause>(new NotEnoughResources(nullptr, this)),
                                this->getMessagePayloadValueAsDouble(
                                        CloudServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD));
            } else {

                // Sleep for the VM booting overhead
                Simulation::sleep(
                        this->getPropertyValueAsDouble(CloudServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS));

                // Start the actual vm
                vm->start(picked_host);


                // Create the Compute Service if needed
                if (vm_pair.second == nullptr) {

                    // Create the resource set for the BareMetalComputeService
                    std::map<std::string, std::tuple<unsigned long, double>> compute_resources = {
                            std::make_pair(vm_name, std::make_tuple(vm->getNumCores(), vm->getMemory()))};

                    // Create the BareMetal service, whose main daemon is on this (stable) host
                    std::shared_ptr<BareMetalComputeService> cs = std::shared_ptr<BareMetalComputeService>(
                            new BareMetalComputeService(this->hostname,
                                                        compute_resources,
                                                        {{ComputeServiceProperty::SUPPORTS_STANDARD_JOBS,"true"}},
                                                        {},
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

                msg_to_send_back = new CloudServiceStartVMAnswerMessage(
                        true,
                        std::get<1>(this->vm_list[vm_name]),
                        nullptr,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD));
            }
        }

        try {
            S4U_Mailbox::dputMessage(
                    answer_mailbox, msg_to_send_back);
        }  catch (std::shared_ptr<NetworkError> &cause) {
            // ignore
        }
        return;
    }

    /**
     * @brief: Process a VM suspend request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     */
    void CloudService::processSuspendVM(const std::string &answer_mailbox, const std::string &vm_name) {

        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudServiceSuspendVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::RUNNING) {

            std::string error_message("Cannot suspend a VM that is not running");
            msg_to_send_back =  new CloudServiceSuspendVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(new NotAllowed(this, error_message)),
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {

            WRENCH_INFO("Asked to suspend VM %s", vm_name.c_str());

            auto vm_tuple = this->vm_list.find(vm_name);

            auto cs = std::get<1>(vm_tuple->second);
            WRENCH_INFO("SUSPENDING THE CS '%s'", cs->getName().c_str());
            cs->suspend();

            WRENCH_INFO("SUSPENDING THE VM");
            auto vm = std::get<0>(vm_tuple->second);
            vm->suspend();

            msg_to_send_back =
                    new CloudServiceSuspendVMAnswerMessage(
                            true,
                            nullptr,
                            this->getMessagePayloadValueAsDouble(
                                    CloudServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        try {
            S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
        } catch (std::shared_ptr<NetworkError> &cause) {
            // ignore
        }
        return;
    }

    /**
     * @brief: Process a VM resume request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM
     */
    void CloudService::processResumeVM(const std::string &answer_mailbox, const std::string &vm_name) {

        WRENCH_INFO("Asked to resume VM %s", vm_name.c_str());
        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudServiceResumeVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::SUSPENDED) {

            std::string error_message("Cannot resume a VM that is not suspended");
            msg_to_send_back =  new CloudServiceResumeVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(new NotAllowed(this, error_message)),
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD));
        } else {

            vm->resume();
            auto cs = vm_pair.second;
            cs->resume();

            msg_to_send_back = new CloudServiceResumeVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        try {
            S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
        } catch (std::shared_ptr<NetworkError> &cause) {
            // ignore
        }
        return;
    }


    /**
    * @brief: Process a VM destruction request
    *
    * @param answer_mailbox: the mailbox to which the answer message should be sent
    * @param vm_name: the name of the VM
    */
    void CloudService::processDestroyVM(const std::string &answer_mailbox, const std::string &vm_name) {

        WRENCH_INFO("Asked to destroy VM %s", vm_name.c_str());
        auto vm_pair = this->vm_list[vm_name];
        auto vm = vm_pair.first;

        CloudServiceDestroyVMAnswerMessage *msg_to_send_back;

        if (vm->getState() != S4U_VirtualMachine::State::DOWN) {
            std::string error_message("Cannot destroy a VM that is not down");
            msg_to_send_back =  new CloudServiceDestroyVMAnswerMessage(
                    false,
                    std::shared_ptr<FailureCause>(new NotAllowed(this, error_message)),
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::DESTROY_VM_ANSWER_MESSAGE_PAYLOAD));

        } else {
            this->vm_list.erase(vm_name);
            msg_to_send_back = new CloudServiceDestroyVMAnswerMessage(
                    true,
                    nullptr,
                    this->getMessagePayloadValueAsDouble(
                            CloudServiceMessagePayload::DESTROY_VM_ANSWER_MESSAGE_PAYLOAD));
        }

        try {
            S4U_Mailbox::dputMessage(answer_mailbox, msg_to_send_back);
        } catch (std::shared_ptr<NetworkError> &cause) {
            // do nothing, just return
        }
    }


    /**
     * @brief Process a submit standard job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     * @param service_specific_args: service specific arguments
     *
     * @throw std::runtime_error
     */
    void CloudService::processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                                std::map<std::string, std::string> &service_specific_args) {

        if (not this->supportsStandardJobs()) {
            try {
                S4U_Mailbox::dputMessage(
                        answer_mailbox, new ComputeServiceSubmitStandardJobAnswerMessage(
                                job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                                this->getMessagePayloadValueAsDouble(
                                        CloudServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
            return;
        } else {
            throw std::runtime_error(
                    "CloudService::processSubmitPilotJob(): A Cloud service should never support standard jobs");
        }

    }

    /**
     * @brief Process a submit pilot job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     * @param service_specific_args: service specific arguments
     *
     * @throw std::runtime_error
     */
    void CloudService::processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job,
                                             std::map<std::string, std::string> &service_specific_args) {

        if (not this->supportsPilotJobs()) {
            try {
                S4U_Mailbox::dputMessage(
                        answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                                job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                                this->getMessagePayloadValueAsDouble(
                                        CloudServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
            } catch (std::shared_ptr<NetworkError> &cause) {
                return;
            }
            return;
        } else {
            throw std::runtime_error(
                    "CloudService::processSubmitPilotJob(): A Cloud service should never support pilot jobs");
        }

    }

    /**
     * @brief Process a "get resource information message"
     * @param answer_mailbox: the mailbox to which the description message should be sent
     */
    void CloudService::processGetResourceInformation(const std::string &answer_mailbox) {
        // Build a dictionary
        std::map<std::string, std::map<std::string, double>> dict;

        // Num hosts
        std::map<std::string, double> num_hosts;
        num_hosts.insert(std::make_pair(this->getName(), (double) (this->vm_list.size())));
        dict.insert(std::make_pair("num_hosts", num_hosts));

        std::map<std::string, double> num_cores;
        std::map<std::string, double> num_idle_cores;
        std::map<std::string, double> flop_rates;
        std::map<std::string, double> ram_capacities;
        std::map<std::string, double> ram_availabilities;

        for (auto &vm : this->vm_list) {

            // Num cores per vm
            num_cores.insert(std::make_pair(vm.first, (double)(std::get<0>(vm.second)->getNumCores())));

            // Num idle cores per vm compute service
            std::map<std::string, unsigned long> idle_core_counts = std::get<1>(vm.second)->getNumIdleCores();
            unsigned long total_count = 0;
            for (auto &c : idle_core_counts) {
                total_count += c.second;
            }
            num_idle_cores.insert(std::make_pair(vm.first, (double) total_count));

            // Flop rate per vm
            flop_rates.insert(std::make_pair(vm.first, S4U_Simulation::getHostFlopRate(vm.first)));

            // RAM capacity per host
            ram_capacities.insert(std::make_pair(vm.first, S4U_Simulation::getHostMemoryCapacity(vm.first)));

            // RAM availability per
            ram_availabilities.insert(std::make_pair(vm.first, S4U_Simulation::getHostMemoryCapacity(vm.first)));
        }

        dict.insert(std::make_pair("num_cores", num_cores));
        dict.insert(std::make_pair("num_idle_cores", num_idle_cores));
        dict.insert(std::make_pair("flop_rates", flop_rates));
        dict.insert(std::make_pair("ram_capacities", ram_capacities));
        dict.insert(std::make_pair("ram_availabilities", ram_availabilities));

        std::map<std::string, double> ttl;
        ttl.insert(std::make_pair(this->getName(), DBL_MAX));
        dict.insert(std::make_pair("ttl", ttl));

        // Send the reply
        ComputeServiceResourceInformationAnswerMessage *answer_message = new ComputeServiceResourceInformationAnswerMessage(
                dict,
                this->getMessagePayloadValueAsDouble(
                        ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
        try {
            S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return;
        }
    }

    /**
    * @brief Terminate all VMs.
    */
    void CloudService::stopAllVMs() {

        WRENCH_INFO("Stopping Cloud Service");
        for (auto &vm : this->vm_list) {
            this->used_ram_per_execution_host[(std::get<0>(vm.second))->getPhysicalHostname()] -= S4U_Simulation::getHostMemoryCapacity(
                    std::get<0>(vm));
            // Deal with the compute service (if it hasn't been stopped before)
            if (std::get<1>(vm.second)) {
                std::get<1>(vm.second)->stop();
            }
            // Deal with the VM
            std::get<0>(vm.second)->shutdown();
        }
        this->vm_list.clear();
    }

    /**
     * @brief Validate the service's properties
     *
     * @throw std::invalid_argument
     */
    void CloudService::validateProperties() {

        // Supporting pilot jobs
        if (this->getPropertyValueAsBoolean(CloudServiceProperty::SUPPORTS_PILOT_JOBS)) {
            throw std::invalid_argument(
                    "Invalid SUPPORTS_PILOT_JOBS property specification: a CloudService cannot support pilot jobs");
        }

        // Supporting standard jobs
        if (this->getPropertyValueAsBoolean(CloudServiceProperty::SUPPORTS_STANDARD_JOBS)) {
            throw std::invalid_argument(
                    "Invalid SUPPORTS_STANDARD_JOBS property specification: a CloudService cannot support standard jobs (instead, it allows for creating VM instances to which standard jobs can be submitted)");
        }

        // VM Boot overhead
        bool success = true;
        double vm_boot_overhead = 0;
        try {
            vm_boot_overhead = this->getPropertyValueAsDouble(CloudServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS);
        } catch (std::invalid_argument &e) {
            success = false;
        }

        if ((!success) or (vm_boot_overhead < 0)) {
            throw std::invalid_argument("Invalid THREAD_STARTUP_OVERHEAD property specification: " +
                                        this->getPropertyValueAsString(CloudServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS));
        }
    }



}
