/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::Simulation is a top-level class that keeps track of
 *  the simulation state.
 */

#include "compute_services/multicore_job_executor/MulticoreJobExecutor.h"
#include "wms/engine/simple_wms/SimpleWMS.h"
#include "simulation/Simulation.h"
#include "exception/WRENCHException.h"
#include "wms/engine/EngineFactory.h"
#include "wms/scheduler/SchedulerFactory.h"

namespace wrench {

		/**
		 * @brief Default constructor, which creates a simulation
		 */
		Simulation::Simulation() {
			// Customize the logging format
			xbt_log_control_set("root.fmt:[%d][%h:%t(%i)]%e%m%n");

			// Create the S4U simulation wrapper
			this->s4u_simulation = std::unique_ptr<S4U_Simulation>(new S4U_Simulation());

		}

		/**
		 * @brief initializes the simulation
		 *
		 * @param argc
		 * @param argv
		 */
		void Simulation::init(int *argc, char **argv) {
			this->s4u_simulation->initialize(argc, argv);
		}

		/**
		 * @brief Launches the simulation
		 *
		 */
		void Simulation::launch() {
			this->s4u_simulation->runSimulation();
		}

		/**
		 * @brief instantiate a simulated platform
		 *
		 * @param filename is the path to a SimGrid XML platform description file
		 */
		void Simulation::createPlatform(std::string filename) {
			this->s4u_simulation->setupPlatform(filename);
		}


		/**
		 * @brief Instantiate a multicore standard job executor on a host
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createMulticoreStandardJobExecutor(std::string hostname) {
			this->createMulticoreJobExecutor(hostname, "yes", "no");
		}

		/**
		 * @brief Instantiate a multicore pilot job executor on a host
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createMulticorePilotJobExecutor(std::string hostname) {
			this->createMulticoreJobExecutor(hostname, "no", "yes");
		}

		/**
		 * @brief Private helper function to instantiate a multicore job executor
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createMulticoreStandardAndPilotJobExecutor(std::string hostname) {
			this->createMulticoreJobExecutor(hostname, "yes", "yes");
		}

		void Simulation::createMulticoreJobExecutor(std::string hostname,
																								std::string supports_standard_jobs,
																								std::string support_pilot_jobs) {

			// Create the compute service
			std::unique_ptr<ComputeService> executor;
			try {
				executor = std::unique_ptr<MulticoreJobExecutor>(new MulticoreJobExecutor(this, hostname));
				executor->setProperty(ComputeService::SUPPORTS_STANDARD_JOBS, supports_standard_jobs);
				executor->setProperty(ComputeService::SUPPORTS_PILOT_JOBS, support_pilot_jobs);
			} catch (WRENCHException e) {
				throw e;
			}

			// Add it to the list of Compute Services
			running_compute_services.push_back(std::move(executor));
			return;
		}

		/**
		 * @brief Instantiate a WMS on a host
		 *
		 * @param wms_id is an ID for a WMS implementation
		 * @param sched_id is an ID for a scheduler implementation
		 * @param w is a pointer to the workflow that the WMS will execute
		 * @param hostname is the name of the host on which to start the WMS
		 */
		void Simulation::createWMS(int wms_id, int sched_id, Workflow *w, std::string hostname) {

			// Obtaining scheduler
			Scheduler *scheduler = SchedulerFactory::getInstance()->Create(sched_id);

			// Obtaining and configuring WMS
			std::unique_ptr<WMS> wms = EngineFactory::getInstance()->Create(wms_id);
			wms->configure(this, w, scheduler, hostname);

			// Add it to the list of WMSes
			WMSes.push_back(std::move(wms));
			return;
		}

		/**
		 * @brief Obtain the list of compute services
		 *
		 * @return vector of compute services
		 */
		std::set<ComputeService *> Simulation::getComputeServices() {
			std::set<ComputeService *> set = {};
			for (int i = 0; i < this->running_compute_services.size(); i++) {
				set.insert(this->running_compute_services[i].get());
			}
			return set;
		}

		/**
		 * @brief Shutdown all running compute services on the platform
		 */
		void Simulation::shutdown() {

			for (int i = 0; i < this->running_compute_services.size(); i++) {
				this->running_compute_services[i]->stop();
			}
		}

		/**
		 * @brief Remove a compute service from the list of known compute services
		 *
		 * @param cs is the compute service
		 */
		void Simulation::mark_compute_service_as_terminated(ComputeService *compute_service) {
			for (int i = 0; i < this->running_compute_services.size(); i++) {
				if (this->running_compute_services[i].get() == compute_service) {
					this->terminated_compute_services.push_back(std::move(this->running_compute_services[i]));
					this->running_compute_services.erase(this->running_compute_services.begin() + i);
					return;
				}
			}
			// If we didn't find the service, this means it was a hidden service that was
			// used as a building block for another higher-level service, which is fine
			return;
		}

};
