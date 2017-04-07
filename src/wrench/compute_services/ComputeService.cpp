/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::ComputeService is a mostly abstract implementation of a compute service.
 */

#include <exception/WRENCHException.h>
#include <logging/ColorLogging.h>
#include "ComputeService.h"
#include "simulation/Simulation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(compute_service, "Log category for Compute Service");


namespace wrench {



		/*****************************/
		/**	DEVELOPER METHODS BELOW **/
		/*****************************/

		/*! \cond DEVELOPER */


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
		 * @brief Get the state of the compute service
		 * @return the state
		 */
		bool ComputeService::isUp() {
			return (this->state == ComputeService::UP);
		}

		/**
		 * @brief Run a standard job
		 * @param job the job
		 */
		void ComputeService::runJob(WorkflowJob *job) {

			if (this->state == ComputeService::DOWN) {
				throw new WRENCHException("Compute Service is Down");
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
		 * @param job is a pointer to a workflow job
		 * @return true is able, false otherwise
		 */
		bool ComputeService::canRunJob(WorkflowJob::Type job_type,
																	 unsigned long min_num_cores,
																	 double flops) {
			bool can_run = true;

			// If the service isn't up, forget it
			if (this->state != ComputeService::UP) {
				return false;
			}

			// Check if the job type works
			switch (job_type) {
				case WorkflowJob::STANDARD: {
					if (this->getProperty(ComputeService::SUPPORTS_STANDARD_JOBS) != "yes") {
						return false;
					}
					break;
				}
				case WorkflowJob::PILOT: {
					if (this->getProperty(ComputeService::SUPPORTS_PILOT_JOBS) != "yes") {
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

		/*! \endcond */


		/****************************/
		/**	INTERNAL METHODS BELOW **/
		/****************************/

		/*! \cond INTERNAL */

		/**
		 * @brief Constructor, which links back the ComputeService
		 *        to a Simulation (i.e.g, "registering" the ComputeService).
		 *        This means that the Simulation can provide access to
		 *        the ComputeService when queried.
		 *
		 * @param service_name is the name of the compute service
		 * @param simulation is a pointer to a WRENCH simulation
		 */
		ComputeService::ComputeService(std::string service_name, Simulation *simulation) {
			this->service_name = service_name;
			this->simulation = simulation;
			this->state = ComputeService::UP;
		}

		/**
		 * @brief Constructor
		 *
		 * @param service_name is the name of the compute service
		 */
		ComputeService::ComputeService(std::string service_name) {
			this->service_name = service_name;
			this->simulation = nullptr;
			this->state = ComputeService::UP;
		}

		/**
		 * @brief Check whether a property is set
		 * @param property_name
		 * @return true or false
		 */
		bool ComputeService::hasProperty(ComputeService::Property property_name) {
			return (this->property_list.find(property_name) != this->property_list.end());
		}

		/**
		 * @brief Return a property value
		 * @param property_name the property_name
		 * @return a property value, or nullptr if the property does not exist
		 */
		std::string ComputeService::getProperty(ComputeService::Property property_name) {
			if (hasProperty(property_name)) {
				return this->property_list[property_name];
			} else {
				return nullptr;
			}
		}

		/**
		 * @brief Set a property value
		 * @param property_name is the property name
		 * @param value is the property value
		 */
		void ComputeService::setProperty(ComputeService::Property property_name, std::string value) {
			this->property_list[property_name] = value;
		}

		/**
		 * @brief Run a standard job
		 * @param the job
		 * @return
		 */
		void ComputeService::runStandardJob(StandardJob *job) {
			throw WRENCHException("The compute service does not implement runStandardJob(StandardJob *)");
		}

		/**
		 * @brief Run a pilot job
		 * @param the job
		 * @return
		 */
		void ComputeService::runPilotJob(PilotJob *job) {
			throw WRENCHException("The compute service does not implement runPilotJob(PilotJob *)");
		}

		/**
		 * @brief set the state of the compute service to DOWN
		 */
		void ComputeService::setStateToDown() {
			this->state = ComputeService::DOWN;
		}

		/*! \endcond */

};
