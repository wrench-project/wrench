/**
 *  @file    MulticoreTaskExecutorDaemon.h
 *  @author  Henri Casanova
 *  @date    3/7/2017
 *  @version 1.0
 *
 *  @brief WRENCH::MulticoreTaskExecutorDaemon class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::MulticoreTaskExecutorDaemon class implements the daemon for a simple
 *  Compute Service abstraction.
 *
 */

#ifndef WRENCH_MULTICORETASKEXECUTORDAEMON_H
#define WRENCH_MULTICORETASKEXECUTORDAEMON_H


#include <simgrid_util/DaemonWithMailbox.h>
#include <compute_services/sequential_task_executor/SequentialTaskExecutor.h>
#include <simgrid_util/Message.h>
#include <queue>
#include <set>

namespace WRENCH {
		
		class MulticoreTaskExecutorDaemon: public DaemonWithMailbox {

		public:
				MulticoreTaskExecutorDaemon(std::vector<SequentialTaskExecutor *>, ComputeService *cs);
				~MulticoreTaskExecutorDaemon();


		private:
				std::set<SequentialTaskExecutor *> idle_sequential_task_executors;
				std::set<SequentialTaskExecutor *> busy_sequential_task_executors;
				std::queue<WorkflowTask *> task_queue;
				int main();

				ComputeService *compute_service;


		};
}


#endif //WRENCH_MULTICORETASKEXECUTORDAEMON_H
