/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/simple/SimpleStorageService.h>
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
//    StorageService *ComputeService::SCRATCH = (StorageService *) ULONG_MAX;
    StorageService *ComputeService::SCRATCH = (StorageService *)ULONG_MAX;

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void ComputeService::stop() {
      Service::stop();
    }


    /**
     * @brief Submit a job to the compute service
     * @param job: the job
     * @param service_specific_args: arguments specific to compute services when needed:
     *      - to a MultihostMulticoreComputeService: {}
     *      - to a BatchService: {"-t":"<int>","-N":"<int>","-c":"<int>"} (SLURM-like)
     *         - "-t": number of requested job duration in minutes
     *         - "-N": number of requested compute hosts
     *         - "-c": number of requested cores per compute host
     *      - to a CloudService: {}
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void ComputeService::submitJob(WorkflowJob *job, std::map<std::string, std::string> service_specific_args) {

      if (job == nullptr) {
        throw std::invalid_argument("ComputeService::submitJob(): invalid argument");
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
     * @brief Terminate a previously-submitted job (which may or may not be running yet)
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
     * @param scratch_size: the size for the scratch space of the compute service
     */
    ComputeService::ComputeService(const std::string &hostname,
                                   const std::string service_name,
                                   const std::string mailbox_name_prefix,
                                   bool supports_standard_jobs,
                                   bool supports_pilot_jobs,
                                   double scratch_size) :
            Service(hostname, service_name, mailbox_name_prefix),
            supports_pilot_jobs(supports_pilot_jobs),
            supports_standard_jobs(supports_standard_jobs) {

      this->state = ComputeService::UP;
      if (scratch_size > 0) {
        try {
          this->scratch_space_storage_service =
                  new SimpleStorageService(hostname, scratch_size);
        } catch (std::runtime_error &e) {
          throw;
        }
      } else {
        this->scratch_space_storage_service = nullptr;
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
     * @param scratch_space: scratch space of the compute service
     */
    ComputeService::ComputeService(const std::string &hostname,
                                   const std::string service_name,
                                   const std::string mailbox_name_prefix,
                                   bool supports_standard_jobs,
                                   bool supports_pilot_jobs,
                                   StorageService *scratch_space) :
            Service(hostname, service_name, mailbox_name_prefix),
            supports_pilot_jobs(supports_pilot_jobs),
            supports_standard_jobs(supports_standard_jobs) {

      this->state = ComputeService::UP;
      this->scratch_space_storage_service = scratch_space;
    }

    /**
     * @brief Get whether the compute service supports standard jobs or not
     * @return true or false
     */
    bool ComputeService::supportsStandardJobs() {
      return this->supports_standard_jobs;
    }

    /**
     * @brief Get whether the compute service supports pilot jobs or not
     * @return true or false
     */
    bool ComputeService::supportsPilotJobs() {
      return this->supports_pilot_jobs;
    }

    /**
     * @brief Get the number of hosts that the compute service manages
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
        return (unsigned long) (*(dict["num_hosts"].begin()));
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
          to_return.push_back((unsigned long) x);
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
          to_return.push_back((unsigned long) x);
        }
      }

      return to_return;
    }

    /**
    * @brief Get the per-core fop ratea of the compute service's hosts
    * @return flop rates in flop/sec
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
    * @brief Get the RAM capacities of the compute service's hosts
    * @return a vector of RAM capacities
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
     * @brief Get the time-to-live of the compute service
     * @return the ttl in seconds
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
     * @brief Get information about the compute service as a dictionary of vectors
     * @return service information
     */
    std::map<std::string, std::vector<double>> ComputeService::getServiceResourceInformation() {

      if (this->state == Service::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      // send a "info request" message to the daemon's mailbox_name
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
        message = S4U_Mailbox::getMessage(answer_mailbox, this->network_timeout);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      } catch (std::shared_ptr<NetworkTimeout> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto msg = dynamic_cast<ComputeServiceResourceInformationAnswerMessage *>(message.get())) {
        return msg->info;

      } else {
        throw std::runtime_error(
                "MultihostMulticoreComputeService::getServiceResourceInformation(): unexpected [" + msg->getName() +
                "] message");
      }
    }

    /**
     * @brief Get the total size of the scratch space (not the remaining free space on the scratch space)
     * @return return a size (in bytes)
     */
    double ComputeService::getScratchSize() {
      return this->scratch_space_storage_service ? this->scratch_space_storage_service->getTotalSpace() : 0.0;
    }

    /**
     * @brief Get the free space of the scratch service
     * @return returns a size (in bytes)
     */
    double ComputeService::getFreeRemainingScratchSpace() {
      return this->scratch_space_storage_service ? this->scratch_space_storage_service->getFreeSpace() : 0.0;
    }

    /**
    * @brief Get the compute service's scratch storage space
    * @return returns a pointer to the shared scratch space
    */
    StorageService *ComputeService::getScratch() {
      return this->scratch_space_storage_service;
    }

    /**
    * @brief Checks if the compute service has a scratch space
    * @return returns TRUE/FALSE (compute service has some scratch space or not)
    */
    bool ComputeService::hasScratch() {
      return (this->scratch_space_storage_service != nullptr);
    }

};
