//
// Created by Henri Casanova on 2/22/17.
//

#ifndef WRENCH_SIMPLEWMSDAEMON_H
#define WRENCH_SIMPLEWMSDAEMON_H

#include "../../simgrid_util/DaemonWithMailbox.h"
#include "../../workflow/Workflow.h"

namespace WRENCH {

//		class Simulation;


		class SimpleWMSDaemon: public DaemonWithMailbox {


		public:
//				SimpleWMSDaemon(std::shared_ptr<Simulation> s, Workflow *w);
				SimpleWMSDaemon(Workflow *w);
				~SimpleWMSDaemon();

		private:
				int main();

//				std::shared_ptr<Simulation> simulation;
				Workflow *workflow;

		};
}


#endif //WRENCH_SIMPLEWMSDAEMON_H
