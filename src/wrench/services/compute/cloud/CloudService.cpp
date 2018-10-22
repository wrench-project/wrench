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
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_service, "Log category for Cloud Service");

namespace wrench {

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
      this->execution_hosts = execution_hosts;

      // Set default and specified properties
      this->setProperties(this->default_property_values, std::move(property_list));
      // Set default and specified message payloads
      this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));
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
                "CloudService::sendRequest(): Received an unexpected [" + msg->getName() + "] message!");
      }
    }

    /**
     * @brief Create a MultihostMulticoreComputeService VM (balances load on execution hosts)
     *
     * @param num_cores: the number of cores the service can use (use ComputeService::ALL_CORES to use all cores
     *                   available on the host)
     * @param ram_memory: the VM's RAM memory capacity (use ComputeService::ALL_RAM to use all RAM available on the
     *                    host, this can be lead to an out of memory issue)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @return Virtual machine name
     *
     * @throw WorkflowExecutionException
     */
    std::string CloudService::createVM(unsigned long num_cores,
                                       double ram_memory,
                                       std::map<std::string, std::string> property_list,
                                       std::map<std::string, std::string> messagepayload_list) {

      // determine physical machine hostname
      std::string pm_hostname;
      unsigned long min_cores = ULONG_MAX;
      for (auto &host : this->execution_hosts) {
        if (this->used_cores_per_execution_host.find(host) == this->used_cores_per_execution_host.end()) {
          pm_hostname = host;
          break;
        }
        if (this->used_cores_per_execution_host[host] < min_cores) {
          pm_hostname = host;
          min_cores = this->used_cores_per_execution_host[host];
        }
      }

      // vm host name
      std::string vm_name = "vm" + std::to_string(CloudService::VM_ID++) + "_" + pm_hostname;

      // send a "create vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("create_vm");

      std::unique_ptr<SimulationMessage> answer_message = sendRequest(
              answer_mailbox,
              new CloudServiceCreateVMRequestMessage(
                      answer_mailbox, pm_hostname, vm_name,
                      num_cores, ram_memory, property_list, messagepayload_list,
                      this->getMessagePayloadValueAsDouble(
                              CloudServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD)));

      if (auto msg = dynamic_cast<CloudServiceCreateVMAnswerMessage *>(answer_message.get())) {
        if (msg->success) {
          return vm_name;
        }
        return nullptr;
      } else {
        throw std::runtime_error("CloudService::createVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Shutdown an active VM
     *
     * @param vm_hostname: the name of the VM host
     *
     * @return Whether the VM shutdown succeeded
     *
     * @throw WorkflowExecutionException
     */
    bool CloudService::shutdownVM(const std::string &vm_hostname) {

      // send a "shutdown vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("shutdown_vm");

      std::unique_ptr<SimulationMessage> answer_message = sendRequest(
              answer_mailbox,
              new CloudServiceShutdownVMRequestMessage(
                      answer_mailbox, vm_hostname,
                      this->getMessagePayloadValueAsDouble(
                              CloudServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD)));

      if (auto msg = dynamic_cast<CloudServiceShutdownVMAnswerMessage *>(answer_message.get())) {
        return msg->success;
      } else {
        throw std::runtime_error("CloudService::shutdownVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Start a VM
     *
     * @param vm_hostname: the name of the VM host
     *
     * @return Whether the VM start succeeded
     *
     * @throw WorkflowExecutionException
     */
    bool CloudService::startVM(const std::string &vm_hostname) {

      // send a "shutdown vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("start_vm");

      std::unique_ptr<SimulationMessage> answer_message = sendRequest(
              answer_mailbox,
              new CloudServiceStartVMRequestMessage(
                      answer_mailbox, vm_hostname,
                      this->getMessagePayloadValueAsDouble(
                              CloudServiceMessagePayload::START_VM_REQUEST_MESSAGE_PAYLOAD)));

      if (auto msg = dynamic_cast<CloudServiceStartVMAnswerMessage *>(answer_message.get())) {
        return msg->success;
      } else {
        throw std::runtime_error("CloudService::startVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Suspend a running VM
     *
     * @param vm_hostname: the name of the VM host
     *
     * @return Whether the VM suspend succeeded
     *
     * @throw WorkflowExecutionException
     */
    bool CloudService::suspendVM(const std::string &vm_hostname) {

      // send a "shutdown vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("suspend_vm");

      std::unique_ptr<SimulationMessage> answer_message = sendRequest(
              answer_mailbox,
              new CloudServiceSuspendVMRequestMessage(
                      answer_mailbox, vm_hostname,
                      this->getMessagePayloadValueAsDouble(
                              CloudServiceMessagePayload::SUSPEND_VM_REQUEST_MESSAGE_PAYLOAD)));

      if (auto msg = dynamic_cast<CloudServiceSuspendVMAnswerMessage *>(answer_message.get())) {
        return msg->success;
      } else {
        throw std::runtime_error("CloudService::suspendVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Resume a suspended VM
     *
     * @param vm_hostname: the name of the VM host
     *
     * @return Whether the VM resume succeeded
     *
     * @throw WorkflowExecutionException
     */
    bool CloudService::resumeVM(const std::string &vm_hostname) {

      // send a "shutdown vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("resume_vm");

      std::unique_ptr<SimulationMessage> answer_message = sendRequest(
              answer_mailbox,
              new CloudServiceResumeVMRequestMessage(
                      answer_mailbox, vm_hostname,
                      this->getMessagePayloadValueAsDouble(
                              CloudServiceMessagePayload::START_VM_REQUEST_MESSAGE_PAYLOAD)));

      if (auto msg = dynamic_cast<CloudServiceResumeVMAnswerMessage *>(answer_message.get())) {
        return msg->success;
      } else {
        throw std::runtime_error("CloudService::resumeVM(): Unexpected [" + msg->getName() + "] message");
      }
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
     * @throw std::runtime_error
     */
    void CloudService::submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_args) {

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
      throw std::runtime_error("CloudService::terminateStandardJob(): Not implemented yet!");
    }

    /**
     * @brief Terminate a pilot job to the compute service (virtual)
     * @param job: the pilot job
     *
     * @throw std::runtime_error
     */
    void CloudService::terminatePilotJob(PilotJob *job) {
      throw std::runtime_error("CloudService::terminatePilotJob(): Not implemented yet!");
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

      WRENCH_INFO("Cloud Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    /**
     * @brief Send a message request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param message: message to be sent
     *
     * @throw std::runtime_error
     */
    std::unique_ptr<SimulationMessage>
    CloudService::sendRequest(std::string &answer_mailbox, ComputeServiceMessage *message) {

      serviceSanityCheck();

      // Send a "run a pilot job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name, message);
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
        processCreateVM(msg->answer_mailbox, msg->pm_hostname, msg->vm_hostname, msg->num_cores, msg->ram_memory,
                        msg->property_list, msg->messagepayload_list);
        return true;

      } else if (auto msg = dynamic_cast<CloudServiceShutdownVMRequestMessage *>(message.get())) {
        processShutdownVM(msg->answer_mailbox, msg->vm_hostname);
        return true;

      } else if (auto msg = dynamic_cast<CloudServiceStartVMRequestMessage *>(message.get())) {
        processStartVM(msg->answer_mailbox, msg->vm_hostname);
        return true;

      } else if (auto msg = dynamic_cast<CloudServiceSuspendVMRequestMessage *>(message.get())) {
        processSuspendVM(msg->answer_mailbox, msg->vm_hostname);
        return true;

      } else if (auto msg = dynamic_cast<CloudServiceResumeVMRequestMessage *>(message.get())) {
        processResumeVM(msg->answer_mailbox, msg->vm_hostname);
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
     * @brief Create a MultihostMulticoreComputeService VM on a physical machine
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param pm_hostname: the name of the physical machine host
     * @param vm_name: the name of the VM host
     * @param num_cores: the number of cores the service can use (use ComputeService::ALL_CORES to use all cores available on the host)
     * @param ram_memory: the VM's RAM memory capacity (use ComputeService::ALL_RAM to use all RAM available on the host, this can be lead to out of memory issue)
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    void CloudService::processCreateVM(const std::string &answer_mailbox,
                                       const std::string &pm_hostname,
                                       const std::string &vm_name,
                                       unsigned long num_cores,
                                       double ram_memory,
                                       std::map<std::string, std::string> &property_list,
                                       std::map<std::string, std::string> &messagepayload_list) {

      try {
        WRENCH_INFO("Asked to create a VM on %s with %d cores", pm_hostname.c_str(), (int) num_cores);

        if (simgrid::s4u::Host::by_name_or_null(vm_name) == nullptr) {
          if (num_cores <= 0) {
            num_cores = S4U_Simulation::getHostNumCores(pm_hostname);
          }

          // RAM memory management
          if (ram_memory != 0) { // RAM is a requirement for creating the VM
            if (this->cs_available_ram.find(pm_hostname) == this->cs_available_ram.end()) {
              this->cs_available_ram[pm_hostname] = S4U_Simulation::getHostMemoryCapacity(pm_hostname);
            }
            if (ram_memory < 0 || ram_memory == ComputeService::ALL_RAM) {
              ram_memory = this->cs_available_ram[pm_hostname];
            }
            if (this->cs_available_ram[pm_hostname] < ram_memory) {
              throw std::runtime_error("Requested memory is below available memory");
            }
          }

          // create a VM on the provided physical machine
          Simulation::sleep(
                  this->getPropertyValueAsDouble(CloudServiceProperty::VM_BOOT_OVERHEAD_IN_SECONDS));
          auto vm = std::make_shared<S4U_VirtualMachine>(vm_name, pm_hostname, num_cores, ram_memory);

          // create a multihost multicore compute service for the VM
          std::map<std::string, std::tuple<unsigned long, double>> compute_resources = {
                  std::make_pair(vm_name, std::make_tuple(num_cores, ram_memory))};

          // Merge the compute service property and message payload lists
          property_list.insert(this->property_list.begin(), this->property_list.end());
          messagepayload_list.insert(this->messagepayload_list.begin(), this->messagepayload_list.end());

          std::shared_ptr<ComputeService> cs = std::shared_ptr<ComputeService>(
                  new MultihostMulticoreComputeService(vm_name,
                                                       compute_resources,
                                                       property_list,
                                                       messagepayload_list,
                                                       getScratch()));
          cs->simulation = this->simulation;

          this->cs_available_ram[pm_hostname] -= ram_memory;

          // starting the service
          try {
            cs->start(cs, true); // Daemonize!
          } catch (std::runtime_error &e) {
            throw;
          }

          this->vm_list[vm_name] = std::make_tuple(vm, cs, num_cores, ram_memory);

          if (this->used_cores_per_execution_host.find(pm_hostname) == this->used_cores_per_execution_host.end()) {
            this->used_cores_per_execution_host.insert(std::make_pair(pm_hostname, num_cores));
          } else {
            this->used_cores_per_execution_host[pm_hostname] += num_cores;
          }

          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new CloudServiceCreateVMAnswerMessage(
                          true,
                          this->getMessagePayloadValueAsDouble(
                                  CloudServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD)));
        } else {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new CloudServiceCreateVMAnswerMessage(
                          false,
                          this->getMessagePayloadValueAsDouble(
                                  CloudServiceMessagePayload::CREATE_VM_ANSWER_MESSAGE_PAYLOAD)));
        }
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief: Process a VM shutdown request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_hostname: the name of the VM host
     */
    void CloudService::processShutdownVM(const std::string &answer_mailbox, const std::string &vm_hostname) {

      try {
        WRENCH_INFO("Asked to shutdown VM %s", vm_hostname.c_str());

        auto vm_tuple = this->vm_list.find(vm_hostname);

        if (vm_tuple == this->vm_list.end()) {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new CloudServiceShutdownVMAnswerMessage(
                          false,
                          this->getMessagePayloadValueAsDouble(
                                  CloudServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD)));
          return;
        }

        auto cs = std::get<1>(vm_tuple->second);
        cs->stop();
        auto vm = std::get<0>(vm_tuple->second);
        vm->shutdown();
        std::get<1>(vm_tuple->second) = nullptr;

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new CloudServiceShutdownVMAnswerMessage(
                        true,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD)));

      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief: Process a VM start request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_name: the name of the VM host
     */
    void CloudService::processStartVM(const std::string &answer_mailbox, const std::string &vm_name) {

      try {
        WRENCH_INFO("Asked to start VM %s", vm_name.c_str());

        auto vm_tuple = this->vm_list.find(vm_name);

        if (vm_tuple == this->vm_list.end()) {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new CloudServiceStartVMAnswerMessage(
                          false,
                          this->getMessagePayloadValueAsDouble(
                                  CloudServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD)));
          return;
        }

        try {
          auto vm = std::get<0>(vm_tuple->second);
          vm->start();

          // TODO: creating a MHMC would not be necessary once auto_restart will be available
          // create a multihost multicore compute service for the VM
          std::map<std::string, std::tuple<unsigned long, double>> compute_resources = {
                  std::make_pair(vm_name, std::make_tuple(std::get<2>(vm_tuple->second), std::get<3>(vm_tuple->second)))};

          std::shared_ptr<ComputeService> cs = std::shared_ptr<ComputeService>(
                  new MultihostMulticoreComputeService(vm_name,
                                                       compute_resources,
                                                       property_list,
                                                       messagepayload_list,
                                                       getScratch()));
          cs->simulation = this->simulation;
          cs->start(cs, true); // Daemonize!
          std::get<1>(vm_tuple->second) = cs;

        } catch (std::runtime_error &e) {
          throw;
        }

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new CloudServiceStartVMAnswerMessage(
                        true,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::START_VM_ANSWER_MESSAGE_PAYLOAD)));

      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief: Process a VM suspend request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_hostname: the name of the VM host
     */
    void CloudService::processSuspendVM(const std::string &answer_mailbox, const std::string &vm_hostname) {

      try {
        WRENCH_INFO("Asked to suspend VM %s", vm_hostname.c_str());

        auto vm_tuple = this->vm_list.find(vm_hostname);

        if (vm_tuple == this->vm_list.end()) {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new CloudServiceSuspendVMAnswerMessage(
                          false,
                          this->getMessagePayloadValueAsDouble(
                                  CloudServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD)));
          return;
        }

        std::shared_ptr<S4U_VirtualMachine> vm = std::get<0>(vm_tuple->second);
        vm->suspend();

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new CloudServiceSuspendVMAnswerMessage(
                        true,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::SUSPEND_VM_ANSWER_MESSAGE_PAYLOAD)));

      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief: Process a VM resume request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param vm_hostname: the name of the VM host
     */
    void CloudService::processResumeVM(const std::string &answer_mailbox, const std::string &vm_hostname) {

      try {
        WRENCH_INFO("Asked to resume VM %s", vm_hostname.c_str());
        auto vm_tuple = this->vm_list.find(vm_hostname);

        if (vm_tuple != this->vm_list.end()) {
          std::shared_ptr<S4U_VirtualMachine> vm = std::get<0>(vm_tuple->second);
          if (vm->isSuspended()) {
            vm->resume();

            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new CloudServiceResumeVMAnswerMessage(
                            true,
                            this->getMessagePayloadValueAsDouble(
                                    CloudServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD)));
            return;
          }
        }

        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new CloudServiceResumeVMAnswerMessage(
                        false,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::RESUME_VM_ANSWER_MESSAGE_PAYLOAD)));

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

      WRENCH_INFO("Asked to run a standard job with %ld tasks", job->getNumTasks());
      if (not this->supportsStandardJobs()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new ComputeServiceSubmitStandardJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                          this->getMessagePayloadValueAsDouble(
                                  ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      std::map<std::string, std::tuple<std::shared_ptr<S4U_VirtualMachine>, std::shared_ptr<ComputeService>,
              unsigned long, unsigned long>> vm_local_list = this->vm_list;

      // whether the job should be mapped to a particular VM
      auto vm_it = service_specific_args.find("-vm");
      if (vm_it != service_specific_args.end()) {
        vm_local_list.clear();
        auto vm_tuple_it = this->vm_list.find(vm_it->second);
        if (vm_tuple_it != this->vm_list.end()) {
          vm_local_list.insert(std::make_pair(vm_it->second, vm_tuple_it->second));
        }
      }


      for (auto vm_tuple : vm_local_list) {
        std::shared_ptr<S4U_VirtualMachine> vm = std::get<0>(vm_tuple.second);
        if (vm->isRunning()) {

          ComputeService *cs = std::get<1>(vm_tuple.second).get();
          std::map<std::string, unsigned long> num_idle_cores = cs->getNumIdleCores();
          unsigned long sum_idle_cores = 0;
          for (auto h : num_idle_cores) {
            sum_idle_cores += h.second;
          }

          if (std::get<2>(vm_tuple.second) >= job->getMinimumRequiredNumCores() &&
              sum_idle_cores >= job->getMinimumRequiredNumCores()) {
            cs->submitStandardJob(job, service_specific_args);

            try {
              S4U_Mailbox::dputMessage(
                      answer_mailbox,
                      new ComputeServiceSubmitStandardJobAnswerMessage(
                              job, this, true, nullptr, this->getMessagePayloadValueAsDouble(
                                      ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
              return;
            } catch (std::shared_ptr<NetworkError> &cause) {
              return;
            }
          }
        }
      }

      // could not find a suitable resource
      try {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitStandardJobAnswerMessage(
                        job, this, false, std::shared_ptr<FailureCause>(new NotEnoughResources(job, this)),
                        this->getMessagePayloadValueAsDouble(
                                ComputeServiceMessagePayload::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
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

      WRENCH_INFO("Asked to run a pilot job with %ld hosts and %ld cores per host for %lf seconds",
                  job->getNumHosts(), job->getNumCoresPerHost(), job->getDuration());

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
      }

      std::map<std::string, std::tuple<std::shared_ptr<S4U_VirtualMachine>, std::shared_ptr<ComputeService>,
              unsigned long, unsigned long>> vm_local_list = this->vm_list;


      // whether the job should be mapped to a particular VM
      auto vm_it = service_specific_args.find("-vm");
      if (vm_it != service_specific_args.end()) {
        vm_local_list.clear();
        auto vm_tuple_it = this->vm_list.find(vm_it->second);
        if (vm_tuple_it != this->vm_list.end()) {
          vm_local_list.insert(std::make_pair(vm_it->second, vm_tuple_it->second));
        }
      }

      // count the number of hosts that have enough cores
      bool available_host = false;

      for (auto &vm_tuple : vm_local_list) {
        std::shared_ptr<S4U_VirtualMachine> vm = std::get<0>(vm_tuple.second);
        if (vm->isRunning()) {
          if (std::get<2>(vm_tuple.second) >= job->getNumCoresPerHost()) {
            std::map<std::string, std::string> service_specific_arguments;
            ComputeService *cs = std::get<1>(vm_tuple.second).get();
            cs->submitPilotJob(job, service_specific_arguments);
            available_host = true;
            break;
          }
        }
      }

      // Do we have enough hosts?
      // currently, the cloud service cannot support the job
      if (job->getNumHosts() > 1 || not available_host) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new NotEnoughResources(job, this)),
                          this->getMessagePayloadValueAsDouble(
                                  CloudServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      // success
      try {
        S4U_Mailbox::dputMessage(
                answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                        job, this, true, nullptr,
                        this->getMessagePayloadValueAsDouble(
                                CloudServiceMessagePayload::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
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
        num_cores.insert(std::make_pair(vm.first, (double) std::get<2>(vm.second)));

        // Num idle cores per vm
        std::map<std::string, unsigned long> idle_core_counts = std::get<1>(vm.second)->getNumIdleCores();
        unsigned long total_count = 0;
        for (auto & c : idle_core_counts) {
          total_count += c.second;
        }
        num_idle_cores.insert(std::make_pair(vm.first, (double)total_count));

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
        this->cs_available_ram[(std::get<0>(vm.second))->getPm()->get_name()] += S4U_Simulation::getHostMemoryCapacity(
                std::get<0>(vm));
        // Deal with the compute service (if it hasn't been stopped before)
        if (std::get<1>(vm.second)) {
          std::get<1>(vm.second)->stop();
        }
        // Deal with the VM
        std::get<0>(vm.second)->stop();
      }
      this->vm_list.clear();
    }
}
