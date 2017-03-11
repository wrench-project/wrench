/**
 *  @brief WRENCH::SequentialTaskExecutorDaemon implements the daemon for the
 *  SequentialTaskExecutor Compute Service abstraction.
 */

#ifndef SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H
#define SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H

#include <compute_services/ComputeService.h>
#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace WRENCH {
		class SequentialTaskExecutorDaemon : public S4U_DaemonWithMailbox {

		public:
				SequentialTaskExecutorDaemon(ComputeService *cs);

		private:
				int main();
				ComputeService *compute_service;

		};
}

#endif //SIMULATION_SEQUENTIALTASKEXECUTORDAEMON_H
