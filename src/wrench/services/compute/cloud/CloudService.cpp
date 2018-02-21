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
#include "wrench/services/compute/cloud/CloudService.h"
#include "wrench/services/compute/multihost_multicore/MultihostMulticoreComputeService.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"


XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_service, "Log category for Cloud Service");

namespace wrench {

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
     * @throw std::invalid_argument
     */
    CloudService::CloudService(const std::string &hostname,
                               bool supports_standard_jobs,
                               bool supports_pilot_jobs,
                               std::vector<std::string> &execution_hosts,
                               StorageService *default_storage_service,
                               std::map<std::string, std::string> plist) :
            ComputeService(hostname, "cloud_service", "cloud_service", supports_standard_jobs, supports_pilot_jobs,
                           default_storage_service) {

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
    CloudService::~CloudService() {
      this->default_property_values.clear();
    }

    /**
     * @brief Get a list of execution hosts to run VMs
     *
     * @return The list of execution hosts
     *
     * @throw WorkflowExecutionException
     */
    std::vector<std::string> CloudService::getExecutionHosts() {

      serviceSanityCheck();

      // send a "get execution hosts" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_execution_hosts");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new CloudServiceGetExecutionHostsRequestMessage(
                                        answer_mailbox,
                                        this->getPropertyValueAsDouble(
                                                CloudServiceProperty::GET_EXECUTION_HOSTS_REQUEST_MESSAGE_PAYLOAD)));
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

      if (auto *msg = dynamic_cast<CloudServiceGetExecutionHostsAnswerMessage *>(message.get())) {
        return msg->execution_hosts;
      } else {
        throw std::runtime_error("CloudService::createVM(): Unexpected [" + msg->getName() + "] message");
      }
    }

    /**
     * @brief Create a multicore executor VM in a physical machine
     *
     * @param pm_hostname: the name of the physical machine host
     * @param vm_hostname: the name of the new VM host
     * @param num_cores: the number of cores the service can use (0 means "use as many as there are cores on the host")
     * @param ram_memory: the VM RAM memory capacity (0 means "use all memory available on the host", this can be lead to out of memory issue)
     * @param plist: a property list ({} means "use all defaults")
     *
     * @return Whether the VM creation succeeded
     *
     * @throw WorkflowExecutionException
     */
    bool CloudService::createVM(const std::string &pm_hostname,
                                const std::string &vm_hostname,
                                unsigned long num_cores,
                                double ram_memory,
                                std::map<std::string, std::string> plist) {

      serviceSanityCheck();

      // send a "create vm" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("create_vm");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name,
                                new CloudServiceCreateVMRequestMessage(
                                        answer_mailbox, pm_hostname, vm_hostname, supports_standard_jobs,
                                        supports_pilot_jobs, num_cores, ram_memory, plist,
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
     * @brief Submit a standard job to the cloud service
     *
     * @param job: a standard job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void CloudService::submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_args) {

      serviceSanityCheck();

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_standard_job");

      //  send a "run a standard job" message to the daemon's mailbox
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
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto *msg = dynamic_cast<ComputeServiceSubmitStandardJobAnswerMessage *>(message.get())) {
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
     * @brief Asynchronously submit a pilot job to the cloud service
     *
     * @param job: a pilot job
     * @param service_specific_args: service specific arguments
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void CloudService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_args) {

      serviceSanityCheck();

      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("submit_pilot_job");

      // Send a "run a pilot job" message to the daemon's mailbox
      try {
        S4U_Mailbox::putMessage(
                this->mailbox_name,
                new ComputeServiceSubmitPilotJobRequestMessage(
                        answer_mailbox, job, this->getPropertyValueAsDouble(
                                CloudServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD)));
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

      if (auto *msg = dynamic_cast<ComputeServiceSubmitPilotJobAnswerMessage *>(message.get())) {
        // If no success, throw an exception
        if (not msg->success) {
          throw WorkflowExecutionException(msg->failure_cause);
        } else {
          return;
        }

      } else {
        throw std::runtime_error(
                "CloudService::submitPilotJob(): Received an unexpected [" + message->getName() + "] message!");
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

      } else if (auto *msg = dynamic_cast<ComputeServiceResourceInformationRequestMessage *>(message.get())) {
        processGetResourceInformation(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<CloudServiceGetExecutionHostsRequestMessage *>(message.get())) {
        processGetExecutionHosts(msg->answer_mailbox);
        return true;

      } else if (auto *msg = dynamic_cast<CloudServiceCreateVMRequestMessage *>(message.get())) {
        processCreateVM(msg->answer_mailbox, msg->pm_hostname, msg->vm_hostname, msg->supports_standard_jobs,
                        msg->supports_pilot_jobs, msg->num_cores, msg->ram_memory, msg->plist);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceSubmitStandardJobRequestMessage *>(message.get())) {
        processSubmitStandardJob(msg->answer_mailbox, msg->job, msg->service_specific_args);
        return true;

      } else if (auto *msg = dynamic_cast<ComputeServiceSubmitPilotJobRequestMessage *>(message.get())) {
        processSubmitPilotJob(msg->answer_mailbox, msg->job);
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
     * @brief Get a list of execution hosts to run VMs
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     */
    void CloudService::processGetExecutionHosts(const std::string &answer_mailbox) {

      try {
        S4U_Mailbox::dputMessage(
                answer_mailbox,
                new CloudServiceGetExecutionHostsAnswerMessage(
                        this->execution_hosts,
                        this->getPropertyValueAsDouble(
                                CloudServiceProperty::GET_EXECUTION_HOSTS_ANSWER_MESSAGE_PAYLOAD)));
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
    void CloudService::processCreateVM(const std::string &answer_mailbox,
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
          if (this->cs_available_ram.find(pm_hostname) == this->cs_available_ram.end()) {
            this->cs_available_ram[pm_hostname] = S4U_Simulation::getMemoryCapacity(pm_hostname);
          }
          if (ram_memory <= 0 || ram_memory == ComputeService::ALL_RAM) {
            ram_memory = this->cs_available_ram[pm_hostname];
          }
          if (this->cs_available_ram[pm_hostname] == 0 || this->cs_available_ram[pm_hostname] < ram_memory) {
            throw std::runtime_error("Requested memory is below available memory");
          }

          // create a VM on the provided physical machine
          simgrid::s4u::VirtualMachine *vm = new simgrid::s4u::VirtualMachine(
                  vm_hostname.c_str(), simgrid::s4u::Host::by_name(pm_hostname), num_cores);

          // create a multihost multicore computer service for the VM
          std::set<std::tuple<std::string, unsigned long, double>> compute_resources = {
                  std::make_tuple(vm_hostname, num_cores, ram_memory)};

          std::unique_ptr<ComputeService> cs(
                  new MultihostMulticoreComputeService(vm_hostname,
                                                       supports_standard_jobs,
                                                       supports_pilot_jobs,
                                                       compute_resources,
                                                       default_storage_service, plist));

          this->cs_available_ram[pm_hostname] -= ram_memory;

          cs->setSimulation(this->simulation);

          // start the service
          try {
            cs->start();
          } catch (std::runtime_error &e) {
            throw;
          }

          this->vm_list[vm_hostname] = std::make_tuple(vm, std::move(cs), num_cores);

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
    void CloudService::processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job) {

      WRENCH_INFO("Asked to run a pilot job with %ld hosts and %ld cores per host for %lf seconds",
                  job->getNumHosts(), job->getNumCoresPerHost(), job->getDuration());

      if (not this->supportsPilotJobs()) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new JobTypeNotSupported(job, this)),
                          this->getPropertyValueAsDouble(
                                  CloudServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
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
      // currently, the cloud service does not support
      if (job->getNumHosts() > 1 || not available_host) {
        try {
          S4U_Mailbox::dputMessage(
                  answer_mailbox, new ComputeServiceSubmitPilotJobAnswerMessage(
                          job, this, false, std::shared_ptr<FailureCause>(new NotEnoughComputeResources(job, this)),
                          this->getPropertyValueAsDouble(
                                  CloudServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
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
                                CloudServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD)));
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
      std::map<std::string, std::vector<double>> dict;

      // Num cores per vm
      std::vector<double> num_cores;
      for (auto &vm : this->vm_list) {
        num_cores.push_back(std::get<2>(vm.second));
      }

      dict.insert(std::make_pair("num_cores", num_cores));

      // Num idle cores per vm
      std::vector<double> num_idle_cores;
      for (auto &vm : this->vm_list) {
        std::vector<unsigned long> idle_core_counts = std::get<1>(vm.second)->getNumIdleCores();
        num_idle_cores.push_back(std::accumulate(idle_core_counts.begin(), idle_core_counts.end(), 0));
      }

      dict.insert(std::make_pair("num_idle_cores", num_idle_cores));

      // Flop rate per vm
      std::vector<double> flop_rates;
      for (auto &vm : this->vm_list) {
        // TODO: This is calling a S4U thing directly, without going though our
        // TODO: S4U_Simulation class (perhaps change later, but then the CloudService
        // TODO: implementation will have to change since it uses the simgrid::s4u stuff
        // TODO: directly
        flop_rates.push_back((std::get<0>(vm.second))->getPstateSpeed(0));
      }
      dict.insert(std::make_pair("flop_rates", flop_rates));

      // RAM capacity per host
      std::vector<double> ram_capacities;
      for (auto &vm : this->vm_list) {
        ram_capacities.push_back(S4U_Simulation::getMemoryCapacity(std::get<0>(vm)));
      }
      dict.insert(std::make_pair("ram_capacities", ram_capacities));

      // RAM availability per host
      std::vector<double> ram_availabilities;
      for (auto &vm : this->vm_list) {
        ram_availabilities.push_back(
                ComputeService::ALL_RAM);  // TODO FOR RAFAEL : What about VM memory availabilities???
      }
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
    void CloudService::terminate() {
      this->setStateToDown();

      WRENCH_INFO("Stopping VMs Compute Service");
      for (auto &vm : this->vm_list) {
        this->cs_available_ram[(std::get<0>(vm.second))->getPm()->getName()] += S4U_Simulation::getMemoryCapacity(
                std::get<0>(vm));
        std::get<1>(vm.second)->stop();
      }
    }
}
