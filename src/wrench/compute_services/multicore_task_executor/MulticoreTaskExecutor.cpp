/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::MulticoreTaskExecutor implements a simple
 *  Compute Service abstraction for a multi-core task executor.
 *
 */

#include <workflow/WorkflowTask.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simgrid_S4U_util/S4U_Simulation.h>
#include "MulticoreTaskExecutor.h"

namespace wrench {

	/**
	 * @brief Constructor that starts the daemon for the service on a host
	 *
	 * @param hostname is the name of the host
	 */
	MulticoreTaskExecutor::MulticoreTaskExecutor(std::string hostname) : ComputeService("multicore_task_executor") {
		this->hostname = hostname;

		// Start one sequential task executor daemon per core
		int num_cores = S4U_Simulation::getNumCores(this->hostname);
		for (int i = 0; i < num_cores; i++) {
			// Start a sequential task executor
			std::unique_ptr<SequentialTaskExecutor> seq_executor =
					std::unique_ptr<SequentialTaskExecutor>(new SequentialTaskExecutor(this->hostname));
			// Add it to the list of sequential task executors
			this->sequential_task_executors.push_back(std::move(seq_executor));
		}

		// Create a list of raw pointers to the sequential task executors
		std::vector<SequentialTaskExecutor *> executor_ptrs;
		for (int i = 0; i < this->sequential_task_executors.size(); i++) {
			executor_ptrs.push_back(this->sequential_task_executors[i].get());
		}

		// Create the main daemon
		this->daemon = std::unique_ptr<MulticoreTaskExecutorDaemon>(
				new MulticoreTaskExecutorDaemon(executor_ptrs, this));

		// Start the daemon
		this->daemon->start(this->hostname);

	}

	/**
	 * @brief Stop the multi-core task executor
	 */
	void MulticoreTaskExecutor::stop() {
		// Stop all sequential task executors
		for (auto &seq_exec : this->sequential_task_executors) {
			seq_exec->stop();
		}

		// Send a termination message to the daemon's mailbox
		S4U_Mailbox::put(this->daemon->mailbox_name, new StopDaemonMessage());
	}

	/**
	 * @brief Have the service execute a workflow task
	 *
	 * @param task is a pointer the workflow task
	 * @param callback_mailbox is the name of a mailbox to which a "task done" callback will be sent
	 * @return 0 on success
	 */
	int MulticoreTaskExecutor::runTask(WorkflowTask *task) {

		// Asynchronously send a "run a task" message to the daemon's mailbox
		S4U_Mailbox::put(this->daemon->mailbox_name, new RunTaskMessage(task));
		return 0;
	};

	/**
	 * @brief Check whether the service has at least an idle core
	 *
	 * @return
	 */
	bool MulticoreTaskExecutor::hasIdleCore() {
		return this->daemon->hasIdleCore();
	}
}