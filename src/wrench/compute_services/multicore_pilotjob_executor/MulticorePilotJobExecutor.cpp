/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::MulticorePilotJobExecutor implements a simple
 *  Compute Service abstraction for a multi-core pilotjob executor.
 *
 */


#include "MulticorePilotJobExecutor.h"

namespace wrench {

		/**
	 * @brief Constructor that starts the daemon for the service on a host,
	 *        registering it with a WRENCH Simulation
	 *
	 * @param hostname is the name of the host
	 * @param simulation is a pointer to a Simulation
	 */
		MulticorePilotJobExecutor::MulticorePilotJobExecutor(std::string hostname, Simulation *simulation) :
						ComputeService("multicore_pilot_job_executor", simulation) {

			// Set all relevant properties
			this->setProperty(ComputeService::SUPPORTS_STANDARD_JOBS, "yes");
			this->setProperty(ComputeService::SUPPORTS_PILOT_JOBS, "no");

			// Create the main daemon
			this->daemon = std::unique_ptr<MulticorePilotJobExecutorDaemon>(
							new MulticorePilotJobExecutorDaemon());

			// Start the daemon on the same host
			this->daemon->start(hostname);

		}
		

};
