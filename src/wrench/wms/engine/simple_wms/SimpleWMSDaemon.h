/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SimpleWMSDaemon implements the daemon for a simple WMS abstraction
 */

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include "simgrid_S4U_util/S4U_DaemonWithMailbox.h"
#include "workflow/Workflow.h"
#include "wms/scheduler/Scheduler.h"

namespace wrench {

	class Simulation; // forward ref

	class SimpleWMSDaemon : public S4U_DaemonWithMailbox {

	public:
		SimpleWMSDaemon(Simulation *, Workflow *w, Scheduler *s);

	private:
		int main();

		Simulation *simulation;
		Workflow *workflow;
		Scheduler *scheduler;

	};
}

#endif //WRENCH_SIMPLEWMSDAEMON_H
