/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <exceptions/ComputeServiceIsDownException.h>
#include <logging/Logging.h>
#include "ComputeService.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(compute_service, "Log category for Compute Service");

namespace wrench {

		/**
		 * @brief Sets the simulation the compute service belongs to
		 * @param simulation: pointer to a Simulation object
		 */
		void ComputeService::setSimulation(Simulation *simulation) {
			this->simulation = simulation;
		}

		/**
		 * @brief Stop the compute service - must be called by the stop()
		 *        method of derived classes
		 */
		void ComputeService::stop() {
			this->state = ComputeService::DOWN;
			// Notify the simulation that the service is terminated, if that
			// service was registered with the simulation
			if (this->simulation) {
				this->simulation->mark_compute_service_as_terminated(this);
			}
		}

		/**
		 * @brief Get the name of the compute service
		 * @return the compute service name
		 */
		std::string ComputeService::getName() {
			return this->service_name;
		}

		/**
		* @brief Find out whether the compute service is UP
		* @return true if the compute service is UP, false otherwise
		*/
		bool ComputeService::isUp() {
			return (this->state == ComputeService::UP);
		}

		/**
		 * @brief Submit a job to the compute service
		 * @param job: a pointer to the job
		 *
		 * @throw ComputeServiceIsDownException
		 */
		void ComputeService::runJob(WorkflowJob *job) {

			if (this->state == ComputeService::DOWN) {
				throw ComputeServiceIsDownException(this->getName());
			}

			switch (job->getType()) {
				case WorkflowJob::STANDARD: {
					this->runStandardJob((StandardJob *) job);
					break;
				}
				case WorkflowJob::PILOT: {
					this->runPilotJob((PilotJob *) job);
					break;
				}
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
			unsigned long num_idle_cores = this->getNumIdleCores();
			WRENCH_INFO("The compute service says it has %ld idle cores", num_idle_cores);
			if (num_idle_cores < min_num_cores) {
				return false;
			}

			// Check that the TTL is ok (does a communication with the daemons)
			double ttl = this->getTTL();
			double duration = flops / this->getCoreFlopRate();
			if ((ttl > 0) && (ttl < duration)) {
				return false;
			}

			// Everything checks out
			return true;
		}


		/**
		 * @brief Constructor
		 *
		 * @param service_name: the name of the compute service
		 */
		ComputeService::ComputeService(std::string service_name) {
			this->service_name = service_name;
			this->simulation = nullptr; // will be filled in via Simulation::add()
			this->state = ComputeService::UP;
		}

		/**
		 * @brief Set the "supports standard jobs" property
		 * @param v: true or false
		 */
		void ComputeService::setSupportStandardJobs(bool v) {
			this->supports_standard_jobs = v;
		}

		/**
		 * @brief Set the "supports pilot jobs" property
		 * @param v: true or false
		 */
		void ComputeService::setSupportPilotJobs(bool v) {
			this->supports_pilot_jobs = v;
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
		void ComputeService::runStandardJob(StandardJob *job) {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement runStandardJob(StandardJob *)");
		}

		/**
		 * @brief Submit a pilot job to the compute service (virtual)
		 * @param job: a pointer ot the job
		 *
		 * @throw std::runtime_error
		 */
		void ComputeService::runPilotJob(PilotJob *job) {
			throw std::runtime_error("Compute service '"+this->getName()+"' does not implement runPilotJob(StandardJob *)");
		}

		/**
		 * @brief Set the state of the compute service to DOWN
		 */
		void ComputeService::setStateToDown() {
			this->state = ComputeService::DOWN;
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

};
