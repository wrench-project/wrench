/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/exceptions/WorkflowExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include "wrench/services/compute/ComputeService.h"
#include "wrench/simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(compute_service, "Log category for Compute Service");

namespace wrench {

    /**
     * @brief Stop the compute service - must be called by the stop()
     *        method of derived classes
     */
    void ComputeService::stop() {

      // Call the super class's method
      Service::stop();

    }

    /**
     * @brief Submit a job to the compute service
     * @param job: the job
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void ComputeService::runJob(WorkflowJob *job) {

      if (job == nullptr) {
        throw std::invalid_argument("ComputeService::runJob(): invalid argument");
      }

      if (this->state == ComputeService::DOWN) {
        throw WorkflowExecutionException(new ServiceIsDown(this));
      }

      try {
        switch (job->getType()) {
          case WorkflowJob::STANDARD: {
            this->submitStandardJob((StandardJob *) job);
            break;
          }
          case WorkflowJob::PILOT: {
            this->submitPilotJob((PilotJob *) job);
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
     * @brief Submit a job to the batch service
     * @param job: the job
     * @param batch_job_args: arguments to the batch job
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void ComputeService::runJob(WorkflowJob *job,std::map<std::string,unsigned long> batch_job_args) {

        if (job == nullptr) {
            throw std::invalid_argument("ComputeService::runJob(): invalid argument");
        }

        if (this->state == ComputeService::DOWN) {
            throw WorkflowExecutionException(new ServiceIsDown(this));
        }

        try {
            switch (job->getType()) {
                case WorkflowJob::STANDARD: {
                    this->submitStandardJob((StandardJob*) job,batch_job_args);
                    break;
                }
                case WorkflowJob::PILOT: {
                    this->submitPilotJob((PilotJob *) job, batch_job_args);
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
     * @param service_name: the name of the compute service
     * @param mailbox_name_prefix: the mailbox name prefix
     * @param supports_standard_jobs: true if the job executor should support standard jobs
     * @param supports_pilot_jobs: true if the job executor should support pilot jobs
     * @param default_storage_service: a storage service
     */
    ComputeService::ComputeService(std::string service_name,
                                   std::string mailbox_name_prefix,
                                   bool supports_standard_jobs,
                                   bool supports_pilot_jobs,
                                   StorageService *default_storage_service) :
            Service(service_name, mailbox_name_prefix),
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
     * @brief Submit a standard job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void ComputeService::submitStandardJob(StandardJob *job) {
      throw std::runtime_error("ComputeService::submitStandardJob(): Not implemented here");
    }

    /**
    * @brief Submit a standard job to a batch service (virtual)
    * @param job: the job
    * @param batch_job_args: arguments to the batch job
    *
    * @throw std::runtime_error
    */
    unsigned long ComputeService::submitStandardJob(StandardJob *job,std::map<std::string,unsigned long> batch_job_args) {
        throw std::runtime_error("ComputeService::submitStandardJob(): Not implemented here");
    }

    /**
     * @brief Submit a pilot job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void ComputeService::submitPilotJob(PilotJob *job) {
      throw std::runtime_error("ComputeService::submitPilotJob(): Not implemented here");
    }

    /**
    * @brief Submit a pilot job to a batch service (virtual)
    * @param job: the job
    * @param batch_job_args: arguments to the batch job
    *
    * @throw std::runtime_error
    */
    unsigned long ComputeService::submitPilotJob(PilotJob *job,std::map<std::string,unsigned long> batch_job_args) {
        throw std::runtime_error("ComputeService::submitPilotJob(): Not implemented here");
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
     * @brief Get the flop/sec rate of one core of the compute service's host
     * @return  the flop rate
     *
     * @throw std::runtime_error
     */
    double ComputeService::getCoreFlopRate() {
      throw std::runtime_error("ComputeService::getCoreFlopRate(): Not implemented here");
    }

    /**
     * @brief Get the number of physical cores on the compute service's host
     * @return the core count
     *
     * @throw std::runtime_error
     */
    unsigned long ComputeService::getNumCores() {
      throw std::runtime_error("ComputeService::getNumCores(): Not implemented here");
    }

    /**
     * @brief Get the number of currently idle cores on the compute service's host
     * @return the idle core count
     *
     * @throw std::runtime_error
     */
    unsigned long ComputeService::getNumIdleCores() {
      throw std::runtime_error("ComputeService::getNumIdleCores(): Not implemented here");
    }

    /**
     * @brief Get the time-to-live, in seconds, of the compute service
     * @return the ttl
     *
     * @throw std::runtime_error
     */
    double ComputeService::getTTL() {
      throw std::runtime_error("ComputeService::getTTL(): Not implemented");
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
     * @brief Set default and user defined properties
     * @param default_property_values: list of default properties
     * @param plist: user defined list of properties
     */
    void ComputeService::setProperties(std::map<std::string, std::string> default_property_values,
                                       std::map<std::string, std::string> plist) {
      // Set default properties
      for (auto p : default_property_values) {
        this->setProperty(p.first, p.second);
      }

      // Set specified properties
      for (auto p : plist) {
        this->setProperty(p.first, p.second);
      }
    }
};
