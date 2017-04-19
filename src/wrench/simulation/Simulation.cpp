/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <logging/Logging.h>
#include "compute_services/multicore_job_executor/MulticoreJobExecutor.h"
#include "simulation/Simulation.h"
#include "exception/WRENCHException.h"
#include "wms/engine/EngineFactory.h"
#include "wms/scheduler/SchedulerFactory.h"

#include "wms/optimizations/static/SimplePipelineClustering.h"

namespace wrench {

		/**
		 * @brief Default constructor, which creates a simulation
		 */
		Simulation::Simulation() {
			// Customize the logging format
//			xbt_log_control_set("root.fmt:[%d][%h:%t(%i)]%e%m%n");
			xbt_log_control_set("root.fmt:[%d][%h:%t]%e%m%n");

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
		void Simulation::createMulticoreStandardJobExecutor(std::string hostname,
																												std::map<MulticoreJobExecutor::Property , std::string> plist) {
			this->createMulticoreJobExecutor(hostname, true, false, plist);
		}

		/**
		 * @brief Instantiate a multicore pilot job executor on a host
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createMulticorePilotJobExecutor(std::string hostname,
																										 std::map<MulticoreJobExecutor::Property , std::string> plist) {
			this->createMulticoreJobExecutor(hostname, false, true, plist);
		}

		/**
		 * @brief Private helper function to instantiate a multicore job executor
		 *
		 * @param hostname is the name of the host in the physical platform
		 */
		void Simulation::createMulticoreStandardAndPilotJobExecutor(std::string hostname,
																																std::map<MulticoreJobExecutor::Property , std::string> plist) {
			this->createMulticoreJobExecutor(hostname, true, true, plist);
		}

		/**
		 * @brief Instantiate a WMS on a host
		 *
		 * @param wms_id is an ID for a WMS implementation
		 * @param sched_id is an ID for a scheduler implementation
		 * @param workflow is a pointer to the workflow that the WMS will execute
		 * @param hostname is the name of the host on which to start the WMS
		 */
		void Simulation::createWMS(std::string wms_id, std::string sched_id, Workflow *workflow, std::string hostname) {
			// Obtaining scheduler
			std::unique_ptr<Scheduler> scheduler = SchedulerFactory::getInstance()->Create(sched_id);

			// Obtaining and configuring WMS
			std::unique_ptr<WMS> wms = EngineFactory::getInstance()->Create(wms_id);
			wms->configure(this, workflow, std::move(scheduler), hostname);

			std::unique_ptr<StaticOptimization> opt(new SimplePipelineClustering());
			wms->add_static_optimization(std::move(opt));

			// Add it to the list of WMSes
			WMSes.push_back(std::move(wms));
		}

		/*****************************/
		/**	DEVELOPER METHODS BELOW **/
		/*****************************/

		/*! \cond DEVELOPER */

		/**
		 * @brief Obtain the list of compute services
		 *
		 * @return vector of compute services
		 */
		std::set<ComputeService *> Simulation::getComputeServices() {
			std::set<ComputeService *> set = {};
			for (auto it = this->running_compute_services.begin(); it != this->running_compute_services.end(); it++) {
				set.insert((*it).get());
			}
			return set;
		}

		/**
		 * @brief Shutdown all running compute services on the platform
		 */
		void Simulation::shutdownAllComputeServices() {

			for (int i = 0; i < this->running_compute_services.size(); i++) {
				this->running_compute_services[i]->stop();
			}
		}

		/*! \endcond */

		/****************************/
		/**	INTERNAL METHODS BELOW **/
		/****************************/

		/*! \cond INTERNAL */

		/**
		 * @brief Method to create an unregistered executor
		 *
		 * @param hostname  is the hostname
		 * @param supports_standard_jobs is true/false
		 * @param support_pilot_jobs is true/false
		 * @param plist is a property list
		 * @num_cores is the number of cores
		 * @ttl is the time-to-live of the executor
		 * @suffix is a suffix to be appended to the process name (useful for debugging)
		 */
		MulticoreJobExecutor *Simulation::createUnregisteredMulticoreJobExecutor(std::string hostname,
																																						 bool supports_standard_jobs,
																																						 bool supports_pilot_jobs,
																																						 std::map<MulticoreJobExecutor::Property , std::string> plist,
																																						 int num_cores,
																																						 double ttl,
																																						 PilotJob *pj,
																																						 std::string suffix) {

			// Create the compute service
			MulticoreJobExecutor *executor;
			try {
				executor = new MulticoreJobExecutor(nullptr, hostname, plist, num_cores, ttl, pj, suffix);
				executor->setSupportStandardJobs(supports_standard_jobs);
				executor->setSupportPilotJobs(supports_pilot_jobs);
			} catch (WRENCHException e) {
				throw e;
			}
			return executor;
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

		/**
		 * @brief Helper method
		 * @param hostname  is the host on which to start the executor
		 * @param supports_standard_jobs is true if the executor supports standard jobs
		 * @param support_pilot_jobs is true if the executor supports pilot jobs
		 */
		void Simulation::createMulticoreJobExecutor(std::string hostname,
																								bool supports_standard_jobs,
																								bool support_pilot_jobs,
																								std::map<MulticoreJobExecutor::Property , std::string> plist) {

			// Create the compute service
			MulticoreJobExecutor *executor;
			try {
				executor = new MulticoreJobExecutor(this, hostname);
				executor->setSupportStandardJobs(supports_standard_jobs);
				executor->setSupportPilotJobs(support_pilot_jobs);
			} catch (WRENCHException e) {
				throw e;
			}

			// Set its properties
			for (auto p : plist) {
				executor->setProperty(p.first, p.second);
			}

			// Add a unique ptr to the list of Compute Services
			std::unique_ptr<ComputeService> ptr = std::unique_ptr<ComputeService>(executor);
			running_compute_services.push_back(std::move(ptr));
			return;
		}

		/*! \endcond */
};
