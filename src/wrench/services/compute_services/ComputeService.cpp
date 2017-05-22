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

			// Notify the simulation that the service is terminated, if that
			// service was registered with the simulation
			if (this->simulation) {
				this->simulation->mark_compute_service_as_terminated(this);
			}

			// Call the super class's method
			Service::stop();

		}


		/**
		 * @brief Submit a job to the compute service
		 * @param job: a pointer to the job
		 *
		 * @throw WorkflowExecutionException
		 */
		void ComputeService::runJob(WorkflowJob *job) {

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
		 * @brief Check whether the service is able to run a job
		 *
		 * @param job_type: the job type
		 * @param min_num_cores: the minimum number of cores required
		 * @param duration: the duration in seconds
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
					if (!this->supportsStandardJobs()) {
						return false;
					}
					break;
				}
				case WorkflowJob::PILOT: {
					if (!this->supportsPilotJobs()) {
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
				double duration = flops / this->getCoreFlopRate();
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
		 * @param default_storage_service: a raw pointer to a StorageService object
		 */
		ComputeService::ComputeService(std::string service_name,
		                               std::string mailbox_name_prefix,
		                               StorageService *default_storage_service) : Service(service_name, mailbox_name_prefix)

		{
			this->default_storage_service = default_storage_service;
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
		 * @param job: a pointer to the job
		 *
		 * @throw std::runtime_error
		 */
		void ComputeService::submitStandardJob(StandardJob *job) {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement submitStandardJob(StandardJob *)");
		}

		/**
		 * @brief Submit a pilot job to the compute service (virtual)
		 * @param job: a pointer ot the job
		 *
		 * @throw std::runtime_error
		 */
		void ComputeService::submitPilotJob(PilotJob *job) {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement submitPilotJob(StandardJob *)");
		}

		/**
		 * @brief Get the flop/sec rate of one core of the compute service's host
		 * @return  the flop rate
		 *
		 * @throw std::runtime_error
		 */
		double ComputeService::getCoreFlopRate() {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement getCoreFlopRate()");
		}

		/**
		 * @brief Get the number of physical cores on the compute service's host
		 * @return the core count
		 *
		 * @throw std::runtime_error
		 */
		unsigned long ComputeService::getNumCores() {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement getNumCores()");
		}

		/**
		 * @brief Get the number of currently idle cores on the compute service's host
		 * @return the idle core count
		 *
		 * @throw std::runtime_error
		 */
		unsigned long ComputeService::getNumIdleCores() {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement getNumIdleCores()");
		}

		/**
		 * @brief Get the time-to-live, in seconds, of the compute service
		 * @return the ttl
		 *
		 * @throw std::runtime_error
		 */
		double ComputeService::getTTL() {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement getTTL()");
		}


    /**
     * @brief Set the default StorageService for the ComputeService
     * @param storage_service: a raw pointer to a StorageService object
     */
    void ComputeService::setDefaultStorageService(StorageService *storage_service) {
      this->default_storage_service = storage_service;
    }

    /**
    * @brief Get the default StorageService for the ComputeService
    * @return a raw pointer to a StorageService object
    */
    StorageService *ComputeService::getDefaultStorageService() {
      return this->default_storage_service;
    }
};
