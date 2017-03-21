/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::EngineDaemon is a template class for WMS engines
 */

#ifndef WRENCH_ENGINEDAEMON_H
#define WRENCH_ENGINEDAEMON_H

#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"
#include "workflow/Workflow.h"
#include "wms/scheduler/Scheduler.h"

namespace wrench {

	class EngineDaemon : public S4U_DaemonWithMailbox {

	public:
		/**
		 * @brief Constructor
		 *
		 * @param s is a pointer to the simulation
		 * @param w is a pointer to the workflow to execute
		 * @param sc is a pointer to a scheduler
		 */
		EngineDaemon(Simulation *s, Workflow *w, Scheduler *sc) : S4U_DaemonWithMailbox("simple_wms", "simple_wms") {
			this->simulation = s;
			this->workflow = w;
			this->scheduler = sc;
		}

	protected:
		Simulation *simulation;
		Workflow *workflow;
		Scheduler *scheduler;
	};
}

#endif //WRENCH_ENGINEDAEMON_H
