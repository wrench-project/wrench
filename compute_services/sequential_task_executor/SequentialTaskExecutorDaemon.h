//
// Created by Henri Casanova on 2/22/17.
//

#ifndef SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H
#define SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H

#include "../../simgrid_util/DaemonWithMailbox.h"

namespace WRENCH {
		class SequentialTaskExecutorDaemon: public DaemonWithMailbox {

		public:
				SequentialTaskExecutorDaemon();
				~SequentialTaskExecutorDaemon();

		private:
				int main();

		};
}


#endif //SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H
