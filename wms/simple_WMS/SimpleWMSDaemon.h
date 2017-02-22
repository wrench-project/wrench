//
// Created by Henri Casanova on 2/22/17.
//

#ifndef SIMULATION_SIMPLEWMSDAEMON_H
#define SIMULATION_SIMPLEWMSDAEMON_H

#include "../../simgrid_util/DaemonWithMailbox.h"
#include "../../platform/Platform.h"
#include "../../workflow/Workflow.h"

namespace WRENCH {

		class SimpleWMSDaemon: public DaemonWithMailbox {

		public:
				SimpleWMSDaemon(Platform *p, Workflow *w);
				~SimpleWMSDaemon();

		private:
				int main();

				Platform *platform;
				Workflow *workflow;

		};
}


#endif //SIMULATION_SIMPLEWMSDAEMON_H
