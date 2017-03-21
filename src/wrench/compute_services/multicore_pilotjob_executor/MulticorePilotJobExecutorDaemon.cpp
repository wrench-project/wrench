/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief TBD
 */


#include <simgrid_S4U_util/S4U_Simulation.h>
#include "MulticorePilotJobExecutorDaemon.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(multicore_pilot_job_executor, "Log category for Multicore Pilot Job Executor");


namespace wrench {

		/**
		 * @brief Constructor
		 *
		 * @param executors is a vector of sequential task executors
		 * @param cs is a pointer to the compute service for this daemon
		 */
		MulticorePilotJobExecutorDaemon::MulticorePilotJobExecutorDaemon() : S4U_DaemonWithMailbox("multicore_pilot_job_executor", "multicore_pilot_job_executor") {


		}

		/**
		 * @brief Main method of the daemon
		 *
		 * @return 0 on termination
		 */
		int MulticorePilotJobExecutorDaemon::main() {
			XBT_INFO("New Multicore Pilot Job Executor starting (%s)",
							 this->mailbox_name.c_str());


			XBT_INFO("Multicore Pilot Job Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
			return 0;
		}

};