/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/ComputeServiceProperty.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(compute_service, "Log category for Compute Service");

namespace wrench {

    constexpr unsigned long ComputeService::ALL_CORES;
    constexpr double ComputeService::ALL_RAM;

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void ComputeService::stop() {

      // Call the super class's method
      Service::stop();

    }


    /**
     * @brief Submit a job to the batch service
     * @param job: the job
     * @param service_specific_arguments: arguments specific to a compute service:
     * @param service_specific_args: arguments specific for compute services:
     *      - to a multicore_compute_service: {}
     *      - to a batch service: {"-t":"<int>","-n":"<int>","-N":"<int>","-c":"<int>"}
     *      - to a cloud service: {}
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void ComputeService::runJob(WorkflowJob *job, std::map<std::string, std::string> service_specific_args) {

      if (job == nullptr) {
        throw std::invalid_argument("ComputeService::runJob(): invalid argument");
      }

      if (this->state == ComputeService::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      try {
        switch (job->getType()) {
          case WorkflowJob::STANDARD: {
            this->submitStandardJob((StandardJob *) job, service_specific_args);
            break;
          }
          case WorkflowJob::PILOT: {
            this->submitPilotJob((PilotJob *) job, service_specific_args);
            break;
          }
        }
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }
    }

    /**
     * @brief Terminate a previously-submitted job (which may or may not be running)
     * 
     * @param job: the job to terminate
     * 
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void ComputeService::terminateJob(WorkflowJob *job) {

      if (job == nullptr) {
        throw std::invalid_argument("ComputeService::terminateJob(): invalid argument");
      }

      if (this->state == ComputeService::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      try {
        switch (job->getType()) {
          case WorkflowJob::STANDARD: {
            this->terminateStandardJob((StandardJob *) job);
            break;
          }
          case WorkflowJob::PILOT: {
            this->terminatePilotJob((PilotJob *) job);
            break;
          }
        }
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the service runs
     * @param service_name: the name of the compute service
     * @param mailbox_name_prefix: the mailbox name prefix
     * @param supports_standard_jobs: true if the job executor should support standard jobs
     * @param supports_pilot_jobs: true if the job executor should support pilot jobs
     * @param default_storage_service: a storage service
     */
    ComputeService::ComputeService(std::string hostname,
                                   std::string service_name,
                                   std::string mailbox_name_prefix,
                                   bool supports_standard_jobs,
                                   bool supports_pilot_jobs,
                                   StorageService *default_storage_service) :
            Service(hostname, service_name, mailbox_name_prefix),
            supports_pilot_jobs(supports_pilot_jobs),
            supports_standard_jobs(supports_standard_jobs),
            default_storage_service(default_storage_service) {

      this->simulation = nullptr; // will be filled in via Simulation::add()
      this->state = ComputeService::UP;
    }

    /**
     * @brief Get the "supports standard jobs" property
     * @return true or false
     */
    bool ComputeService::supportsStandardJobs() {
      return this->supports_standard_jobs;
    }

    /**
     * @brief Get the "supports pilot jobs" property
     * @return true or false
     */
    bool ComputeService::supportsPilotJobs() {
      return this->supports_pilot_jobs;
    }

    /**
     * @brief Get host (compute node) counts for the compute service
     * @return the host counts
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    unsigned long ComputeService::getNumHosts() {

      std::map<std::string, std::vector<double>> dict;
      try {
        dict = this->getServiceResourceInformation();
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }


      if (dict.find("num_hosts") != dict.end()) {
        return (unsigned long)(*(dict["num_hosts"].begin()));
      } else {
        return 0;
      }
    }


    /**
      * @brief Get core counts for each of the compute service's host
      * @return the core counts
      *
      * @throw WorkflowExecutionException
      * @throw std::runtime_error
      */
    std::vector<unsigned long> ComputeService::getNumCores() {

      std::map<std::string, std::vector<double>> dict;
      try {
        dict = this->getServiceResourceInformation();
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }

      std::vector<unsigned long> to_return;

      if (dict.find("num_cores") != dict.end()) {
        for (auto x : dict["num_cores"]) {
          to_return.push_back((unsigned long)x);
        }
      }

      return to_return;
    }

    /**
     * @brief Get idle core counts for each of the compute service's host
     * @return the idle core counts (could be empty)
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    std::vector<unsigned long> ComputeService::getNumIdleCores() {

      std::map<std::string, std::vector<double>> dict;
      try {
        dict = this->getServiceResourceInformation();
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }

      std::vector<unsigned long> to_return;

      if (dict.find("num_idle_cores") != dict.end()) {
        for (auto x : dict["num_idle_cores"]) {
          to_return.push_back((unsigned long)x);
        }
      }

      return to_return;
    }

    /**
    * @brief Get the flop/sec rate of one core of the compute service's host
    * @return  the flop rate
    *
    * @throw std::runtime_error
    */
    std::vector<double> ComputeService::getCoreFlopRate() {

      std::map<std::string, std::vector<double>> dict;
      try {
        dict = this->getServiceResourceInformation();
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }

      std::vector<double> to_return;
      if (dict.find("flop_rates") != dict.end()) {
        for (auto x : dict["flop_rates"]) {
          to_return.push_back(x);
        }
      }

      return to_return;
    }

    /**
    * @brief Get the  RAM capacities of the compute service's hosts
    * @return the RAM capacities
    *
    * @throw std::runtime_error
    */
    std::vector<double> ComputeService::getMemoryCapacity() {

      std::map<std::string, std::vector<double>> dict;
      try {
        dict = this->getServiceResourceInformation();
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }

      std::vector<double> to_return;

      if (dict.find("ram_capacities") != dict.end()) {
        for (auto x : dict["ram_capacities"]) {
          to_return.push_back(x);
        }
      }

      return to_return;
    }

    /**
     * @brief Get the time-to-live, in seconds, of the compute service
     * @return the ttl
     *
     * @throw std::runtime_error
     */
    double ComputeService::getTTL() {

      std::map<std::string, std::vector<double>> dict;
      try {
        dict = this->getServiceResourceInformation();
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        throw;
      }

      return dict["ttl"][0];
    }


    /**
     * @brief Submit a standard job to a batch service (virtual)
     * @param job: the job
     * @param service_specific_arguments: arguments specific for the compute service
     *
     * @throw std::runtime_error
     */
    void ComputeService::submitStandardJob(StandardJob *job,
                                           std::map<std::string, std::string> &service_specific_arguments) {
      throw std::runtime_error("ComputeService::submitStandardJob(): Not implemented here");
    }

    /**
    * @brief Submit a pilot job to a batch service (virtual)
    * @param job: the job
    * @param service_specific_arguments: arguments specific for the compute service
    *
    * @throw std::runtime_error
    */
    void ComputeService::submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_arguments) {
      throw std::runtime_error("ComputeService::submitPilotJob(): Not implemented here");
    }

    /**
     * @brief Process a submit standard job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     * @param service_specific_arguments: arguments specific for the compute service
     *
     * @throw std::runtime_error
     */
    void ComputeService::processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                                  std::map<std::string, std::string> &service_specific_args) {
      throw std::runtime_error("ComputeService::processSubmitStandardJob(): Not implemented here");
    }

    /**
     * @brief Process a submit pilot job request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void ComputeService::processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job) {
      throw std::runtime_error("ComputeService::processSubmitPilotJob(): Not implemented here");
    }

    /**
     * @brief Process a get resource description request
     *
     * @param answer_mailbox: the mailbox to which the answer message should be sent
     *
     * @throw std::runtime_error
     */
    void ComputeService::processGetResourceInformation(const std::string &answer_mailbox) {
      throw std::runtime_error("ComputeService::processGetResourceInformation(): Not implemented here");
    }

    /**
    * @brief Terminate a standard job to the compute service (virtual)
    * @param job: the job
    *
    * @throw std::runtime_error
    */
    void ComputeService::terminateStandardJob(StandardJob *job) {
      throw std::runtime_error("ComputeService::terminateStandardJob(): Not implemented here");
    }

    /**
     * @brief Terminate a pilot job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void ComputeService::terminatePilotJob(PilotJob *job) {
      throw std::runtime_error("ComputeService::terminatePilotJob(): Not implemented here");
    }


    /**
     * @brief Set the default StorageService for the ComputeService
     * @param storage_service: a storage service
     */
    void ComputeService::setDefaultStorageService(StorageService *storage_service) {
      this->default_storage_service = storage_service;
    }

    /**
    * @brief Get the default StorageService for the ComputeService
    * @return a storage service
    */
    StorageService *ComputeService::getDefaultStorageService() {
      return this->default_storage_service;
    }

    /**
     * @brief Get information about the compute service as a dictionary of vectors
     * @return service information
     */
    std::map<std::string, std::vector<double>> ComputeService::getServiceResourceInformation() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "info request" message to the daemon's mailbox
      std::string answer_mailbox = S4U_Mailbox::generateUniqueMailboxName("get_service_info");

      try {
        S4U_Mailbox::putMessage(this->mailbox_name, new ComputeServiceResourceInformationRequestMessage(
                answer_mailbox,
                this->getPropertyValueAsDouble(
                        ComputeServiceProperty::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD)));
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      // Get the reply
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(answer_mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto *msg = dynamic_cast<ComputeServiceResourceInformationAnswerMessage *>(message.get())) {
        return msg->info;

      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::getResourceDescription(): unexpected [" + msg->getName() + "] message");
      }
    }
};
