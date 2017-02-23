//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include "../../simgrid_util/DaemonWithMailbox.h"
#include "../../workflow/Workflow.h"

namespace WRENCH {

		class Simulation;


		class SimpleWMSDaemon: public DaemonWithMailbox {


		public:
				SimpleWMSDaemon(Simulation *, Workflow *w);
				~SimpleWMSDaemon();

		private:
				int main();

				Simulation *simulation;
				Workflow *workflow;

		};
}


#endif //WRENCH_SIMPLEWMSDAEMON_H
