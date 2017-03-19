/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::MulticoreTaskExecutorDaemon implements the daemon for
 *  the MulticoreTaskExecutor Compute Service abstraction.
 *
 */

#ifndef WRENCH_MULTICORETASKEXECUTORDAEMON_H
#define WRENCH_MULTICORETASKEXECUTORDAEMON_H

#include <queue>
#include <set>

#include "helper_daemons/sequential_task_executor/SequentialTaskExecutor.h"
#include "simulation/SimulationMessage.h"

namespace wrench {

	class MulticoreJobExecutorDaemon : public S4U_DaemonWithMailbox {

	public:
		MulticoreJobExecutorDaemon(ComputeService *cs);
		bool hasIdleCore();

	private:
		std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;

		std::set<SequentialTaskExecutor *> idle_sequential_task_executors;
		std::set<SequentialTaskExecutor *> busy_sequential_task_executors;
		std::set<StandardJob *> pending_jobs;
		std::queue<WorkflowTask *> waiting_task_queue;
		std::set<WorkflowTask *> running_task_set;

		int main();

		ComputeService *compute_service;
	};
}


#endif //WRENCH_MULTICORETASKEXECUTORDAEMON_H
