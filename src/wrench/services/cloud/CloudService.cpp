/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "wrench/services/cloud/CloudService.h"
#include "wrench/services/compute/MulticoreComputeService.h"
#include "wrench/services/storage/SimpleStorageService.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "services/ServiceMessage.h"
#include "services/cloud/CloudServiceMessage.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_service, "Log category for Cloud Service");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param default_storage_service: a storage service
     * @param plist: a property list ({} means "use all defaults")
     *
     * @throw std::invalid_argument
     */
    CloudService::CloudService(std::string &hostname,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               StorageService *default_storage_service,
                               std::map<std::string, std::string> plist = {}) :
            ComputeService("cloud_service", "cloud_service", supports_standard_jobs, supports_pilot_jobs,
                           default_storage_service) {

      this->hostname = hostname;

      // Set default and specified properties
      this->setProperties(this->default_property_values, plist);

      // Start the daemon on the same host
      try {
        this->start(hostname);
      } catch (std::invalid_argument &e) {
        throw e;
      }
    }

    /**
     * @brief Create a multicore executor VM in a physical machine
     *
     * @param pm_hostname: the name of the physical machine host
     * @param vm_hostname: the name of the new VM host
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param plist: a property list ({} means "use all defaults")
     *
     * @return Whether the VM creation succeeded
     *
     * @throw WorkflowExecutionException
     */
    bool CloudService::createVM(const std::string &pm_hostname,
                                const std::string &vm_hostname,
                                int num_cores,
                                std::map<std::string, std::string> plist) {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "create vm" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("create_vm");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new CloudServiceCreateVMRequestMessage(
                                        answer_mailbox, pm_hostname, vm_hostname, num_cores, supports_standard_jobs,
                                        supports_pilot_jobs, plist,
                                        this->getPropertyValueAsDouble(
                                                CloudServiceProperty::CREATE_VM_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto *msg = dynamic_cast<CloudServiceCreateVMAnswerMessage *>(message.get())) {
        return msg->success;
      } else {
        throw std::runtime_error("CloudService::createVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
    * @brief Main method of the daemon
    *
    * @return 0 on termination
    */
    int CloudService::main() {

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_RED);
      WRENCH_INFO("Cloud Service starting on host %s listening on mailbox %s", this->hostname.c_str(),
                  this->mailbox_name.c_str());

      /** Main loop **/
      while (this->processNextMessage()) {
        // no specific action
      }

      WRENCH_INFO("Cloud Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
      return 0;
    }

    /**
     * @brief Wait for and react to any incoming message
     *
     * @return false if the daemon should terminate, true otherwise
     *
     * @throw std::runtime_error
     */
    bool CloudService::processNextMessage() {
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

      if (auto *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        this->terminate();
        // This is Synchronous
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          CloudServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return false;
        }
        return false;

      } else if (auto *msg = dynamic_cast<ComputeServiceNumCoresRequestMessage *>(message.get())) {
        processGetNumCores(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceNumIdleCoresRequestMessage *>(message.get())) {
        processGetNumIdleCores(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<CloudServiceCreateVMRequestMessage *>(message.get())) {
        processCreateVM(msg->answer_mailbox, msg->pm_hostname, msg->vm_hostname, msg->num_cores,
                        msg->supports_standard_jobs, msg->supports_pilot_jobs, msg->plist);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        processSubmitStandardJob(msg->answer_mailbox, msg->job);
        return true;

      } else if (auto *msg = dynamic_cast<ServiceStopDaemonMessage *>(message.get())) {
        this->terminate();
        // This is Synchronous
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          CloudServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return false;
        }
        return false;

      } else {
        throw std::runtime_error("Unexpected [" + message->getName() + "] message");
      }
    }

    /**
     * @brief Create a multicore executor VM in a physical machine
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param pm_hostname: the name of the physical machine host
     * @param vm_hostname: the name of the VM host
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param plist: a property list ({} means "use all defaults")
     *
     * @return A multicore executor VM in a physical machine
     */
    void CloudService::processCreateVM(const std::string &answer_mailbox,
                                       const std::string &pm_hostname,
                                       const std::string &vm_hostname,
                                       int num_cores,
                                       bool supports_standard_jobs,
                                       bool supports_pilot_jobs,
                                       std::map<std::string, std::string> plist = {}) {

      WRENCH_INFO("Asked to create a VM on %s with %d cores", pm_hostname.c_str(), num_cores);

      try {

        if (simgrid::s4u::Host::by_name_or_null(vm_hostname) == nullptr) {
          if (num_cores <= 0) {
            num_cores = S4U_Simulation::getNumCores(hostname);
          }
          this->vm_list[vm_hostname] = new simgrid::s4u::VirtualMachine(vm_hostname.c_str(),
                                                                        simgrid::s4u::Host::by_name(
                                                                                pm_hostname), num_cores);
          // create a multicore executor for the VM
          this->cs_list[vm_hostname] = std::unique_ptr<ComputeService>(
                  new MulticoreComputeService(vm_hostname, supports_standard_jobs, supports_pilot_jobs,
                                              {std::pair<std::string, unsigned long>(vm_hostname, num_cores)},
                                              default_storage_service, plist));
          this->cs_list[vm_hostname]->setSimulation(this->simulation);

          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new CloudServiceCreateVMAnswerMessage(
                          true,
                          this->getPropertyValueAsDouble(CloudServiceProperty::CREATE_VM_ANSWER_MESSAGE_PAYLOAD)));
        } else {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new CloudServiceCreateVMAnswerMessage(
                          false,
                          this->getPropertyValueAsDouble(CloudServiceProperty::CREATE_VM_ANSWER_MESSAGE_PAYLOAD)));
        }
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Process a get number of cores request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     *
     * @throw std::runtime_error
     */
    void CloudService::processGetNumCores(std::string &answer_mailbox) {
      unsigned int total_num_cores = 0;
      for (auto &cs : this->cs_list) {
        total_num_cores += cs.second->getNumCores();
      }

      ComputeServiceNumCoresAnswerMessage *answer_message = new ComputeServiceNumCoresAnswerMessage(
              total_num_cores,
              this->getPropertyValueAsDouble(
                      ComputeServiceProperty::NUM_CORES_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Process a get number of idle cores request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     *
     * @throw std::runtime_error
     */
    void CloudService::processGetNumIdleCores(std::string &answer_mailbox) {
      unsigned int total_num_available_cores = 0;
      for (auto &cs : this->cs_list) {
        total_num_available_cores += cs.second->getNumIdleCores();
      }

      ComputeServiceNumIdleCoresAnswerMessage *answer_message = new ComputeServiceNumIdleCoresAnswerMessage(
              total_num_available_cores,
              this->getPropertyValueAsDouble(
                      MulticoreComputeServiceProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Process a submit standard job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void CloudService::processSubmitStandardJob(std::string &answer_mailbox, StandardJob *job) {
      WRENCH_INFO("Asked to run a standard job with %ld tasks", job->getNumTasks());
      if (not this->supportsStandardJobs()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new ComputeServiceSubmitStandardJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                          this->getPropertyValueAsDouble(
                                  ComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      for (auto &cs : cs_list) {
        if (cs.second->getNumIdleCores() >= job->getMinimumRequiredNumCores()) {
          cs.second->submitStandardJob(job);
          try {
            S4U_Mailbox::dputMessage(
                    answer_mailbox,
                    new ComputeServiceSubmitStandardJobAnswerMessage(
                            job, this, true, nullptr, this->getPropertyValueAsDouble(
                                    ComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
            return;
          } catch (std::shared_ptr<NetworkError> &cause) {
            return;
          }
        }
      }

      // could not find a suitable resource
      try {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new ComputeServiceSubmitStandardJobAnswerMessage(
                        job, this, false, std::shared_ptr<FailureCause>(new NotEnoughComputeResources(job, this)),
                        this->getPropertyValueAsDouble(
                                ComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
    * @brief Terminate the daemon.
    */
    void CloudService::terminate() {
      this->setStateToDown();

      WRENCH_INFO("Stopping VMs Compute Service");
      for (auto &cs : this->cs_list) {
        cs.second->stop();
      }
    }
}
