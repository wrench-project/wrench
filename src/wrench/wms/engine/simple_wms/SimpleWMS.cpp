/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief WRENCH::SimpleWMS implements a simple WMS abstraction
 */

#include "wms/engine/simple_wms/SimpleWMS.h"

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param s is a pointer to a simulation
	 * @param w is a pointer to a workflow
	 * @param sc is a pointer to a scheduler
	 * @param hostname is the hostname on which the WMS daemon runs
	 */
	SimpleWMS::SimpleWMS(Simulation *s, Workflow *w, Scheduler *sc, std::string hostname) : WMS(w, sc) {
		// Create the daemon
		this->wms_process = std::unique_ptr<SimpleWMSDaemon>(new SimpleWMSDaemon(s, w, sc));
		// Start the daemon
		this->wms_process->start(hostname);
	}
};