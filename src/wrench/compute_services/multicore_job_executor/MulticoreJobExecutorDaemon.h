/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::MulticoreJobExecutorDaemon implements the daemon for
 *  the MulticoreStandardJobExecutor Compute Service abstraction.
 *
 */

#ifndef WRENCH_MULTICORETASKEXECUTORDAEMON_H
#define WRENCH_MULTICORETASKEXECUTORDAEMON_H

#include <queue>
#include <set>
#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

#include "helper_daemons/sequential_task_executor/SequentialTaskExecutor.h"
#include "simulation/SimulationMessage.h"

namespace wrench {

		class MulticoreJobExecutorDaemon : public S4U_DaemonWithMailbox {

		public:
				MulticoreJobExecutorDaemon(ComputeService *cs, int num_worker_threads=-1, double ttl=-1);

		private:
				int num_worker_threads;
				double ttl;

				std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;

				std::set<SequentialTaskExecutor *> idle_sequential_task_executors;
				std::set<SequentialTaskExecutor *> busy_sequential_task_executors;
				std::set<StandardJob *> pending_jobs;
				std::queue<WorkflowTask *> waiting_task_queue;
				std::set<WorkflowTask *> running_task_set;

				int main();

				// Helper functions
				void initialize();
				void terminate_all_worker_threads();
				void fail_all_current_jobs();
				void process_task_completion(WorkflowTask *, SequentialTaskExecutor *);


				ComputeService *compute_service;
		};
}


#endif //WRENCH_MULTICORETASKEXECUTORDAEMON_H
