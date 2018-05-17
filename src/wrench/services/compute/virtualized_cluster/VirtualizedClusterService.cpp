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
#include "wrench/services/compute/virtualized_cluster/VirtualizedClusterService.h"
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(virtualized_cluster_service, "Log category for Virtualized Cluster Service");

namespace wrench {

    static unsigned long VM_ID = 1;

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which to start the service
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param execution_hosts: the hosts available for running virtual machines
     * @param default_storage_service: a storage service (or nullptr)
     * @param plist: a property list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    VirtualizedClusterService::VirtualizedClusterService(const std::string &hostname,
                                                         bool supports_standard_jobs,
                                                         bool supports_pilot_jobs,
                                                         std::vector<std::string> &execution_hosts,
                                                         std::map<std::string, std::string> plist,
                                                         int scratch_size) :
            ComputeService(hostname, "virtualized_cluster_service", "virtualized_cluster_service",
                           supports_standard_jobs, supports_pilot_jobs,
                           scratch_size) {

      if (execution_hosts.empty()) {
        throw std::runtime_error("At least one execution host should be provided");
      }
      this->execution_hosts = execution_hosts;

      // Set default and specified properties
      this->setProperties(this->default_property_values, plist);
    }

    /**
     * @brief Destructor
     */
    VirtualizedClusterService::~VirtualizedClusterService() {
      this->default_property_values.clear();
    }

    /**
     * @brief Get a list of execution hosts to run VMs
     *
     * @return The list of execution hosts
     *
     * @throw WorkflowExecutionException
     */
    std::vector<std::string> VirtualizedClusterService::getExecutionHosts() {

      serviceSanityCheck();

      // send a "get execution hosts" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_execution_hosts");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new VirtualizedClusterServiceGetExecutionHostsRequestMessage(
                                        answer_mailbox,
                                        this->getPropertyValueAsDouble(
                                                VirtualizedClusterServiceProperty::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<VirtualizedClusterServiceGetExecutionHostsAnswerMessage *>(message.get())) {
        return msg->execution_hosts;
      } else {
        throw std::runtime_error("VirtualizedClusterService::createVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Create a multicore executor VM in a physical machine
     *
     * @param pm_hostname: the name of the physical machine host
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param ram_memory: the VM RAM memory capacity (-1 means "use all memory available on the host", this can be lead to an out of memory issue)
     * @param plist: a property list ({} means "use all defaults")
     *
     * @return Virtual machine hostname
     *
     * @throw WorkflowExecutionException
     */
    std::string VirtualizedClusterService::createVM(const std::string &pm_hostname,
                                                    unsigned long num_cores,
                                                    double ram_memory,
                                                    std::map<std::string, std::string> plist) {

      serviceSanityCheck();

      // vm host name
      std::string vm_hostname = "vm" + std::to_string(VM_ID++) + "_" + pm_hostname;

      // send a "create vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("create_vm");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new VirtualizedClusterServiceCreateVMRequestMessage(
                                        answer_mailbox, pm_hostname, vm_hostname, supports_standard_jobs,
                                        supports_pilot_jobs, num_cores, ram_memory, plist,
                                        this->getPropertyValueAsDouble(
                                                VirtualizedClusterServiceProperty::CREATE_VM_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<VirtualizedClusterServiceCreateVMAnswerMessage *>(message.get())) {
        if (msg->success) {
          return vm_hostname;
        }
        return nullptr;
      } else {
        throw std::runtime_error("VirtualizedClusterService::createVM(): Unexpected [" + msg->getName() + "] message");
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

      serviceSanityCheck();

      // send a "migrate vm" message to the daemon's mailbox_name
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("migrate_vm");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new VirtualizedClusterServiceMigrateVMRequestMessage(
                                        answer_mailbox, vm_hostname, dest_pm_hostname,
                                        this->getPropertyValueAsDouble(
                                                VirtualizedClusterServiceProperty::MIGRATE_VM_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<VirtualizedClusterServiceMigrateVMAnswerMessage *>(message.get())) {
        return msg->success;
      } else {
        throw std::runtime_error("VirtualizedClusterService::migrateVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Submit a standard job to the virtualized cluster service
     *
     * @param job: a standard job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::submitStandardJob(StandardJob *job,
                                                      std::map<std::string, std::string> &service_specific_args) {

      serviceSanityCheck();

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

      //  send a "run a standard job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new ComputeServiceSubmitStandardJobRequestMessage(
                                        answer_mailbox, job, service_specific_args,
                                        this->getPropertyValueAsDouble(
                                                ComputeServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the answer
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        }
      } else {
        throw std::runtime_error(
                "ComputeService::submitStandardJob(): Received an unexpected [" + message->getName() + "] message!");
      }
    }

    /**
     * @brief Asynchronously submit a pilot job to the virtualized cluster service
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::submitPilotJob(PilotJob *job,
                                                   std::map<std::string, std::string> &service_specific_args) {

      serviceSanityCheck();

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

      // Send a "run a pilot job" message to the daemon's mailbox_name
      try {
        S4U_Mailbox::putMessage(
                this->mailbox_name,
                new ComputeServiceSubmitPilotJobRequestMessage(
                        answer_mailbox, job, this->getPropertyValueAsDouble(
                                VirtualizedClusterServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Wait for a reply
      std::unique_ptr<SimulationMessage> message = nullptr;

      try {
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        } else {
          return;
        }

      } else {
        throw std::runtime_error(
                "VirtualizedClusterService::submitPilotJob(): Received an unexpected [" + message->getName() +
                "] message!");
      }
    }

    /**
     * @brief Terminate a standard job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::terminateStandardJob(StandardJob *job) {
      throw std::runtime_error("VirtualizedClusterService::terminateStandardJob(): Not implemented yet!");
    }

    /**
     * @brief Terminate a pilot job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::terminatePilotJob(PilotJob *job) {
      throw std::runtime_error("VirtualizedClusterService::terminatePilotJob(): Not implemented yet!");
    }


    /**
    * @brief Main method of the daemon
    *
    * @return 0 on termination
    */
    int VirtualizedClusterService::main() {

      TerminalOutput::setThisProcessLoggingColor(COLOR_RED);
      WRENCH_INFO("Virtualized Cluster Service starting on host %s listening on mailbox_name %s",
                  this->hostname.c_str(),
                  this->mailbox_name.c_str());

      /** Main loop **/
      while (this->processNextMessage()) {
        // no specific action
      }

      WRENCH_INFO("Virtualized Cluster Service on host %s terminated!", S4U_Simulation::getHostName().c_str());
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
        this->terminate();
        // This is Synchronous
        try {
          S4U_Mailbox::putMessage(msg->ack_mailbox,
                                  new ServiceDaemonStoppedMessage(this->getPropertyValueAsDouble(
                                          VirtualizedClusterServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return false;
        }
        return false;

      } else if (auto msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
        processGetResourceInformation(msg->answer_mailbox);
        return true;

      } else if (auto msg = dynamic_cast<VirtualizedClusterServiceGetExecutionHostsRequestMessage *>(message.get())) {
        processGetExecutionHosts(msg->answer_mailbox);
        return true;

      } else if (auto msg = dynamic_cast<VirtualizedClusterServiceCreateVMRequestMessage *>(message.get())) {
        processCreateVM(msg->answer_mailbox, msg->pm_hostname, msg->vm_hostname, msg->supports_standard_jobs,
                        msg->supports_pilot_jobs, msg->num_cores, msg->ram_memory, msg->plist);
        return true;

      } else if (auto msg = dynamic_cast<VirtualizedClusterServiceMigrateVMRequestMessage *>(message.get())) {
        processMigrateVM(msg->answer_mailbox, msg->vm_hostname, msg->dest_pm_hostname);
        return true;

      } else if (auto msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
        return true;

      } else if (auto msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
        processSubmitPilotJob(msg->answer_mailbox, msg->job);
        return true;

      } else {
        throw std::runtime_error("Unexpected [" + message->getName() + "] message");
      }
    }

    /**
     * @brief Get a list of execution hosts to run VMs
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void VirtualizedClusterService::processGetExecutionHosts(const std::string &answer_mailbox) {

      try {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new VirtualizedClusterServiceGetExecutionHostsAnswerMessage(
                        this->execution_hosts,
                        this->getPropertyValueAsDouble(
                                VirtualizedClusterServiceProperty::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Create a multicore executor VM in a physical machine
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param pm_hostname: the name of the physical machine host
     * @param vm_hostname: the name of the VM host
     * @param supports_standard_jobs: true if the compute service should support standard jobs
     * @param supports_pilot_jobs: true if the compute service should support pilot jobs
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param ram_memory: the VM RAM memory capacity (0 means "use all memory available on the host", this can be lead to out of memory issue)
     * @param plist: a property list ({} means "use all defaults")
     *
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::processCreateVM(const std::string &answer_mailbox,
                                                    const std::string &pm_hostname,
                                                    const std::string &vm_hostname,
                                                    bool supports_standard_jobs,
                                                    bool supports_pilot_jobs,
                                                    unsigned long num_cores,
                                                    double ram_memory,
                                                    std::map<std::string, std::string> plist) {

      WRENCH_INFO("Asked to create a VM on %s with %d cores", pm_hostname.c_str(), (int) num_cores);

      try {

        if (simgrid::s4u::Host::by_name_or_null(vm_hostname) == nullptr) {
          if (num_cores <= 0) {
            num_cores = S4U_Simulation::getNumCores(pm_hostname);
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
          auto vm = std::make_shared<S4U_VirtualMachine>(vm_hostname, pm_hostname, num_cores, ram_memory);

          // create a multihost multicore computer service for the VM
          std::set<std::tuple<std::string, unsigned long, double>> compute_resources = {
                  std::make_tuple(vm_hostname, num_cores, ram_memory)};

          std::shared_ptr<ComputeService> cs = std::shared_ptr<ComputeService>(
                  new MultihostMulticoreComputeService(vm_hostname,
                                                       supports_standard_jobs,
                                                       supports_pilot_jobs,
                                                       compute_resources, plist,
                                                       getScratch()));
          cs->simulation = this->simulation;

          this->cs_available_ram[pm_hostname] -= ram_memory;

          // starting the service
          try {
            cs->start(cs, true); // Daemonize!
          } catch (std::runtime_error &e) {
            throw;
          }

          this->vm_list[vm_hostname] = std::make_tuple(vm, cs, num_cores);

          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new VirtualizedClusterServiceCreateVMAnswerMessage(
                          true,
                          this->getPropertyValueAsDouble(
                                  VirtualizedClusterServiceProperty::CREATE_VM_ANSWER_MESSAGE_PAYLOAD)));
        } else {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new VirtualizedClusterServiceCreateVMAnswerMessage(
                          false,
                          this->getPropertyValueAsDouble(
                                  VirtualizedClusterServiceProperty::CREATE_VM_ANSWER_MESSAGE_PAYLOAD)));
        }
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Synchronously migrate a VM to another physical machine
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

          double mig_sta = simgrid::s4u::Engine::getClock();
          sg_vm_migrate(vm->get(), dest_pm);
          double mig_end = simgrid::s4u::Engine::getClock();
          WRENCH_INFO("%s migrated: %s to %g s", vm_hostname.c_str(), dest_pm->getName().c_str(), mig_end - mig_sta);

          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new VirtualizedClusterServiceMigrateVMAnswerMessage(
                          true,
                          this->getPropertyValueAsDouble(
                                  VirtualizedClusterServiceProperty::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD)));

        } else {
          S4U_Mailbox::dputMessage(
                  answer_mailbox,
                  new VirtualizedClusterServiceMigrateVMAnswerMessage(
                          false,
                          this->getPropertyValueAsDouble(
                                  VirtualizedClusterServiceProperty::MIGRATE_VM_ANSWER_MESSAGE_PAYLOAD)));

        }
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
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
    void VirtualizedClusterService::processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                                             std::map<std::string, std::string> &service_specific_args) {

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

      for (auto &vm : vm_list) {
        ComputeService *cs = std::get<1>(vm.second).get();
        unsigned long sum_num_idle_cores;
        std::vector<unsigned long> num_idle_cores = cs->getNumIdleCores();
        sum_num_idle_cores = (unsigned long) std::accumulate(num_idle_cores.begin(), num_idle_cores.end(), 0);
        if (std::get<2>(vm.second) >= job->getMinimumRequiredNumCores() &&
            sum_num_idle_cores >= job->getMinimumRequiredNumCores()) {
          cs->submitStandardJob(job, service_specific_args);
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
     * @brief Process a submit pilot job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void VirtualizedClusterService::processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job) {

      WRENCH_INFO("Asked to run a pilot job with %ld hosts and %ld cores per host for %lf seconds",
                  job->getNumHosts(), job->getNumCoresPerHost(), job->getDuration());

      if (not this->supportsPilotJobs()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                          this->getPropertyValueAsDouble(
                                  VirtualizedClusterServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
        } catch (std::shared_ptr<NetworkError> &cause) {
          return;
        }
        return;
      }

      // count the number of hosts that have enough cores
      bool available_host = false;

      for (auto &vm : vm_list) {
        if (std::get<2>(vm.second) >= job->getNumCoresPerHost()) {
          ComputeService *cs = std::get<1>(vm.second).get();
          std::map<std::string, std::string> service_specific_arguments;
          cs->submitPilotJob(job, service_specific_arguments);
          available_host = true;
          break;
        }
      }

      // Do we have enough hosts?
      // currently, the virtualized cluster service does not support
      if (job->getNumHosts() > 1 || not available_host) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new NotEnoughComputeResources(job, this)),
                          this->getPropertyValueAsDouble(
                                  VirtualizedClusterServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
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
                        this->getPropertyValueAsDouble(
                                VirtualizedClusterServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
     * @brief Process a "get resource information message"
     * @param answer_mailbox: the mailbox to which the description message should be sent
     */
    void VirtualizedClusterService::processGetResourceInformation(const std::string &answer_mailbox) {
      // Build a dictionary
      std::map<std::string, std::vector<double>> dict;

      // Num hosts
      std::vector<double> num_hosts;
      num_hosts.push_back((double) (this->vm_list.size()));
      dict.insert(std::make_pair("num_hosts", num_hosts));

      std::vector<double> num_cores;
      std::vector<double> num_idle_cores;
      std::vector<double> flop_rates;
      std::vector<double> ram_capacities;
      std::vector<double> ram_availabilities;

      for (auto &vm : this->vm_list) {
        // Num cores per vm
        num_cores.push_back(std::get<2>(vm.second));

        // Num idle cores per vm
        std::vector<unsigned long> idle_core_counts = std::get<1>(vm.second)->getNumIdleCores();
        num_idle_cores.push_back(std::accumulate(idle_core_counts.begin(), idle_core_counts.end(), 0));

        // Flop rate per vm
        flop_rates.push_back(S4U_Simulation::getFlopRate(std::get<0>(vm)));

        // RAM capacity per host
        ram_capacities.push_back(S4U_Simulation::getHostMemoryCapacity(std::get<0>(vm)));

        // RAM availability per
        ram_availabilities.push_back(
                ComputeService::ALL_RAM);  // TODO FOR RAFAEL : What about VM memory availabilities???
      }

      dict.insert(std::make_pair("num_cores", num_cores));
      dict.insert(std::make_pair("num_idle_cores", num_idle_cores));
      dict.insert(std::make_pair("flop_rates", flop_rates));
      dict.insert(std::make_pair("ram_capacities", ram_capacities));
      dict.insert(std::make_pair("ram_availabilities", ram_availabilities));

      std::vector<double> ttl;
      ttl.push_back(ComputeService::ALL_RAM);
      dict.insert(std::make_pair("ttl", ttl));

      // Send the reply
      ComputeServiceResourceInformationAnswerMessage *answer_message = new ComputeServiceResourceInformationAnswerMessage(
              dict,
              this->getPropertyValueAsDouble(
                      ComputeServiceProperty::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD));
      try {
        S4U_Mailbox::dputMessage(answer_mailbox, answer_message);
      } catch (std::shared_ptr<NetworkError> &cause) {
        return;
      }
    }

    /**
    * @brief Terminate the daemon.
    */
    void VirtualizedClusterService::terminate() {
      this->setStateToDown();

      WRENCH_INFO("Stopping Virtualized Cluster Service");
      for (auto &vm : this->vm_list) {
        this->cs_available_ram[(std::get<0>(vm.second))->getPm()->getName()] += S4U_Simulation::getHostMemoryCapacity(
                std::get<0>(vm));
        std::get<0>(vm.second)->stop();
        std::get<1>(vm.second)->stop();
      }
    }
}
