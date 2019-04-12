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
#include <simgrid/plugins/live_migration.h>

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
     * @brief Create a BareMetalComputeService VM on a physical machine
     *
     * @param pm_hostname: the name of the physical machine host
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
    std::pair<std::string, std::shared_ptr<BareMetalComputeService>> VirtualizedClusterService::createVM(const std::string &pm_hostname,
                                                    unsigned long num_cores,
                                                    double ram_memory,
                                                    std::map<std::string, std::string> property_list,
                                                    std::map<std::string, std::string> messagepayload_list) {

      // vm host name
      std::string vm_name = this->getName() + "_vm" + std::to_string(CloudService::VM_ID++);

      // send a "create vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("create_vm");

      std::unique_ptr<SimulationMessage> answer_message = sendRequest(
              answer_mailbox,
              new CloudServiceCreateVMRequestMessage(
                      answer_mailbox, pm_hostname, vm_name,
                      num_cores, ram_memory, property_list, messagepayload_list,
                      this->getMessagePayloadValueAsDouble(
                              VirtualizedClusterServiceMessagePayload::CREATE_VM_REQUEST_MESSAGE_PAYLOAD)));

      if (auto msg = dynamic_cast<CloudServiceCreateVMAnswerMessage *>(answer_message.get())) {
        if (msg->success) {
          return std::make_pair(vm_name, msg->cs);
        } else {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error("VirtualizedClusterService::createVM(): Unexpected [" + answer_message->getName() + "] message");
      }
    }

    /**
     * @brief Synchronously migrate a VM to another physical host
     *
     * @param vm_hostname: virtual machine hostname
     * @param dest_pm_hostname: the name of the destination physical machine host
     *
     * @return Whether the VM was successfully migrated
     */
    bool VirtualizedClusterService::migrateVM(const std::string &vm_hostname, const std::string &dest_pm_hostname) {

      // send a "migrate vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("migrate_vm");

      std::unique_ptr<SimulationMessage> answer_message = sendRequest(
              answer_mailbox,
              new VirtualizedClusterServiceMigrateVMRequestMessage(
                      answer_mailbox, vm_hostname, dest_pm_hostname,
                      this->getMessagePayloadValueAsDouble(
                              VirtualizedClusterServiceMessagePayload::MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD)));

      if (auto msg = dynamic_cast<VirtualizedClusterServiceMigrateVMAnswerMessage *>(answer_message.get())) {
        return msg->success;
      } else {
        throw std::runtime_error("VirtualizedClusterService::migrateVM(): Unexpected [" + msg->getName() + "] message");
      }
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

      } else if (auto msg = dynamic_cast<VirtualizedClusterServiceMigrateVMRequestMessage *>(message.get())) {
        processMigrateVM(msg->answer_mailbox, msg->vm_hostname, msg->dest_pm_hostname);
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
     * @param vm_hostname: the name of the VM host
     * @param dest_pm_hostname: the name of the destination physical machine host
     *
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::processMigrateVM(const std::string &answer_mailbox, const std::string &vm_hostname,
                                                     const std::string &dest_pm_hostname) {

      WRENCH_INFO("Asked to migrate the VM %s to the PM %s", vm_hostname.c_str(), dest_pm_hostname.c_str());

      try {

        auto it = vm_list.find(vm_hostname);

        if (it != vm_list.end()) {

          std::shared_ptr<S4U_VirtualMachine> vm = std::get<0>((*it).second);
          std::shared_ptr<ComputeService> cs = std::get<1>((*it).second);
          simgrid::s4u::Host *dest_pm = simgrid::s4u::Host::by_name_or_null(dest_pm_hostname);

          double mig_sta = simgrid::s4u::Engine::get_clock();
          sg_vm_migrate(vm->get(), dest_pm);
          double mig_end = simgrid::s4u::Engine::get_clock();
          WRENCH_INFO("%s migrated: %s to %g s", vm_hostname.c_str(), dest_pm->get_name().c_str(), mig_end - mig_sta);

          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new VirtualizedClusterServiceMigrateVMAnswerMessage(
                          true,
                          this->getMessagePayloadValueAsDouble(
                                  VirtualizedClusterServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD)));

        } else {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new VirtualizedClusterServiceMigrateVMAnswerMessage(
                          false,
                          this->getMessagePayloadValueAsDouble(
                                  VirtualizedClusterServiceMessagePayload::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD)));

        }
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }


}
