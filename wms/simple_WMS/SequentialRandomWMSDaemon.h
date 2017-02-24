//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include "../../simgrid_util/DaemonWithMailbox.h"
#include "../../workflow/Workflow.h"

namespace WRENCH {

		class Simulation;


		class SequentialRandomWMSDaemon: public DaemonWithMailbox {


		public:
				SequentialRandomWMSDaemon(Simulation *, Workflow *w);
				~SequentialRandomWMSDaemon();

		private:
				int main();

				Simulation *simulation;
				Workflow *workflow;

		};
}


#endif //WRENCH_SIMPLEWMSDAEMON_H
