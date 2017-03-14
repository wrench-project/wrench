/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief WRENCH::SequentialRandomWMS implements a simple WMS abstraction
 */

#include "SequentialRandomWMS.h"

namespace wrench {

		/**
		 * @brief Constructor
		 *
		 * @param s is a pointer to a simulation
		 * @param w is a pointer to a workflow
		 * @param hostname is the hostname on which the WMS daemon runs
		 */
		SequentialRandomWMS::SequentialRandomWMS(Simulation *s, Workflow *w, std::string hostname): WMS(w) {
			// Create the daemon
			this->wms_process = std::unique_ptr<SequentialRandomWMSDaemon>(new SequentialRandomWMSDaemon(s, w));
			// Start the daemon
			this->wms_process->start(hostname);
		}

};