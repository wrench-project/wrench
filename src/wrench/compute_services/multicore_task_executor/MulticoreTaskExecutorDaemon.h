/**
 *  @brief WRENCH::MulticoreTaskExecutorDaemon implements the daemon for
 *  the MulticoreTaskExecutor Compute Service abstraction.
 *
 */

#ifndef WRENCH_MULTICORETASKEXECUTORDAEMON_H
#define WRENCH_MULTICORETASKEXECUTORDAEMON_H


#include <compute_services/sequential_task_executor/SequentialTaskExecutor.h>
#include <simulation/SimulationMessage.h>
#include <queue>
#include <set>

namespace WRENCH {

		class MulticoreTaskExecutorDaemon : public S4U_DaemonWithMailbox {

		public:
				MulticoreTaskExecutorDaemon(std::vector<SequentialTaskExecutor *>, ComputeService *cs);

		private:
				std::set<SequentialTaskExecutor *> idle_sequential_task_executors;
				std::set<SequentialTaskExecutor *> busy_sequential_task_executors;
				std::queue<WorkflowTask *> task_queue;

				int main();

				ComputeService *compute_service;


		};
}


#endif //WRENCH_MULTICORETASKEXECUTORDAEMON_H
