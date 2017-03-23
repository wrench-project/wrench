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

				// Fixed values
				int num_worker_threads; // total threads to run tasks from standard jobs
				double ttl;							// time-to-live

				int num_available_worker_threads; // number of worker threads that can currently be
				                                  // used to run tasks from standard jobs

				// Vector of worker threads
				std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;

				// Vector of idle worker threads available to run tasks from standard jobs
				std::set<SequentialTaskExecutor *> idle_sequential_task_executors;
				// Vector of worker threads currencly running tasks from standard jobs
				std::set<SequentialTaskExecutor *> busy_sequential_task_executors;

				// Queue of pending jobs (standard or pilot) that haven't began executing
				std::queue<WorkflowJob *> pending_jobs;

				// Set of currently running jobs
				std::set<WorkflowJob *> running_jobs;

				// Queue of standard job tasks waiting for execution
				std::queue<WorkflowTask *> pending_tasks;

				// Set of currently running standard job tasks
				std::set<WorkflowTask *> running_tasks;

				int main();

				// Helper functions to make main() a bit more palatable
				void initialize();
				void terminateAllWorkerThreads();
				void failCurrentjobs();
				void processTaskCompletion(WorkflowTask *, SequentialTaskExecutor *);
				bool processNextMessage();
				bool dispatchNextPendingTask();
				bool dispatchNextPendingJob();


				// Pointer to the ComputeService container
				ComputeService *compute_service;
		};
}


#endif //WRENCH_MULTICORETASKEXECUTORDAEMON_H
