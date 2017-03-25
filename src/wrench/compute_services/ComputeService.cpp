/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::ComputeService is a mostly abstract implementation of a compute service.
 */

#include <exception/WRENCHException.h>
#include "ComputeService.h"
#include "simulation/Simulation.h"


namespace wrench {

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
		 * @return the  name
		 */
		std::string ComputeService::getName() {
			return this->service_name;
		}

		/**
		 * @brief Get the state of the compute service
		 * @return the state
		 */
		ComputeService::State ComputeService::getState() {
			return this->state;
		}

		/**
		 * @brief Run a job
		 * @param the job
		 */
		void ComputeService::runJob(WorkflowJob *job) {
			std::cerr << "XXXX " << (unsigned long)job << std::endl;

			switch(job->getType()) {
				case WorkflowJob::STANDARD: {
					this->runStandardJob((StandardJob *)job);
					break;
				}
				case WorkflowJob::PILOT: {
					this->runPilotJob((PilotJob *)job);
					break;
				}
			}

		}



		/**
		 * @brief Check whether a property is set
		 * @param property_name
		 * @return true or false
		 */
		bool ComputeService::hasProperty(ComputeService::Property property) {
			return (this->property_list.find(property) != this->property_list.end());
		}

		/**
		 * @brief Return a property value
		 * @param the property_name
		 * @return a property value, or nullptr if the property does not exist
		 */
		std::string ComputeService::getProperty(ComputeService::Property property) {
			if (this->property_list.find(property) != this->property_list.end()) {
				return this->property_list[property];
			} else {
				return nullptr;
			}
		}

		/**
		 * @brief Set a property value
		 * @param property is the property
		 * @param value is the value
		 */
		void ComputeService::setProperty(ComputeService::Property property, std::string value) {
			this->property_list[property] = value;
		}

		/**
		 * @brief Check whether the service is able to run a job
		 * @param job is a pointer to a workflow job
		 * @return true is able, false otherwise
		 */
		bool ComputeService::canRunJob(WorkflowJob *job) {
			bool can_run = true;

			// Check if the job type works
			switch (job->getType()) {
				case WorkflowJob::STANDARD: {
					can_run = can_run &&  (this->getProperty(ComputeService::SUPPORTS_STANDARD_JOBS) == "yes");
				}
				case WorkflowJob::PILOT: {
					can_run = can_run &&  (this->getProperty(ComputeService::SUPPORTS_PILOT_JOBS) == "yes");
				}
			}

			return can_run;
		}

		/***********************************************************/
		/**	UNDOCUMENTED PUBLIC/PRIVATE  METHODS AFTER THIS POINT **/
		/***********************************************************/

		/*! \cond PRIVATE */

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

		/*! \endcod */
};