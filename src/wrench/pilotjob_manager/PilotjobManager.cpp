/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief PilotJobManager implements a simple facility for managing pilot
 *        jobs submitted by a WMS to pilot-job-enabled compute services.
 */

#include <string>
#include <simgrid_S4U_util/S4U_Simulation.h>
#include "PilotJobManager.h"
#include "PilotJobManagerDaemon.h"

namespace wrench {

		PilotjobManager::PilotjobManager() {
			// Create the  daemon
			this->daemon = std::unique_ptr<PilotjobManagerDaemon>(
							new PilotjobManagerDaemon());

			// Start the daemon
			std::string localhost = S4U_Simulation::getHostName();
			this->daemon->start(localhost);
		}


};