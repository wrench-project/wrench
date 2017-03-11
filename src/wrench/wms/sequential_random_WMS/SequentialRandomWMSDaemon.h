/**
 *  @brief WRENCH::SequentialRandomWMSDaemon implements the daemon for a simple WMS abstraction
 */

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>
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
