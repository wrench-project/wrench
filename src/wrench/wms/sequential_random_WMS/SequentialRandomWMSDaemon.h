/**
 *  @file    SequentialRandomWMSDaemon.h
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::SequentialRandomWMSDaemon class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::SequentialRandomWMSDaemon class implements the daemon for a simple WMS abstraction
 *
 */


#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include <simgrid_Sim4U_util/S4U_DaemonWithMailbox.h>
//#include "simgrid_MSG_util/MSG_DaemonWithMailbox.h"
#include "workflow/Workflow.h"

namespace WRENCH {

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
