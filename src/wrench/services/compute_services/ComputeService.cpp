/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <exceptions/WorkflowExecutionException.h>
#include <logging/TerminalOutput.h>
#include "ComputeService.h"
#include "simulation/Simulation.h"

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
     * @brief Check whether the service is able to run a job
     *
     * @param job_type: the job type
     * @param min_num_cores: the minimum number of cores required
     * @param flops: the flops
     *
     * @return true if the compute service can run the job, false otherwise
     */
    bool ComputeService::canRunJob(WorkflowJob::Type job_type,
                                   unsigned long min_num_cores,
                                   double flops) {
      // If the service isn't up, forget it
      if (this->state != ComputeService::UP) {
        return false;
      }

      // Check if the job type works
      switch (job_type) {
        case WorkflowJob::STANDARD: {
          if (not this->supportsStandardJobs()) {
            return false;
          }
          break;
        }
        case WorkflowJob::PILOT: {
          if (not this->supportsPilotJobs()) {
            return false;
          }
          break;
        }
      }

      // Check that the number of cores is ok (does a communication with the daemons)
      try {
        unsigned long num_idle_cores = this->getNumIdleCores();
        WRENCH_INFO("The compute service says it has %ld idle cores", num_idle_cores);
        if (num_idle_cores < min_num_cores) {
          return false;
        }
      } catch (WorkflowExecutionException &e) {
        throw;
      }

      try {

        // Check that the TTL is ok (does a communication with the daemons)
        double ttl = this->getTTL();
        // TODO: This duration is really hard to compute because we don't know
        // how many cores will be available, we don't know how the core schedule
        // will work out, etc. So right now, if the service couldn't run the job
        // sequentially, we say it can't run it at all. Something to fix at some point.
        // One option is to ask the user to provide the maximum amount of flop that will
        // be required on ONE core assuming min_num_cores cores are available?
        double duration = (flops / this->getCoreFlopRate());
        if ((ttl > 0) && (ttl < duration)) {
          return false;
        }
      } catch (WorkflowExecutionException &e) {
        throw;
      } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        throw;
      }

      // Everything checks out
      return true;
    }

    /**
     * @brief Constructor
     *
     * @param service_name: the name of the compute service
     * @param mailbox_name_prefix: the mailbox name prefix
     * @param default_storage_service: a storage service
     */
    ComputeService::ComputeService(std::string service_name,
                                   std::string mailbox_name_prefix,
                                   StorageService *default_storage_service) :
            Service(service_name, mailbox_name_prefix),
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
     * @brief Submit a pilot job to the compute service (virtual)
     * @param job: the job
     *
     * @throw std::runtime_error
     */
    void ComputeService::submitPilotJob(PilotJob *job) {
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
};
