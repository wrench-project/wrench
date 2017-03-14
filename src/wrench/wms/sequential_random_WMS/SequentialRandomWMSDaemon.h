/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialRandomWMSDaemon implements the daemon for a simple WMS abstraction
 */

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>
#include "workflow/Workflow.h"

namespace wrench {

		class Simulation; // forward ref

		class SequentialRandomWMSDaemon: public S4U_DaemonWithMailbox {

		public:
				SequentialRandomWMSDaemon(Simulation *, Workflow *w);

		private:
				int main();

				Simulation *simulation;
				Workflow *workflow;

		};
}


#endif //WRENCH_SIMPLEWMSDAEMON_H
