/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialTaskExecutor class implements a simple
 *  sequential Compute Service abstraction.
 */

#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <exception/WRENCHException.h>
#include "SequentialTaskExecutor.h"

namespace wrench {

	/**
	 * @brief Constructor, which starts the daemon for the service on a host
	 *
	 * @param hostname is the name of the host
	 */
	SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname, Simulation *simulation) :
					ComputeService("sequential_task_executor", simulation) {
		this->hostname = hostname;
		// Create the daemon
		this->daemon = std::unique_ptr<SequentialTaskExecutorDaemon>(new SequentialTaskExecutorDaemon(this));
		// Start the daemon on the host
		this->daemon->start(this->hostname);
	}

	/**
	 * @brief Terminate the sequential task executor
	 */
	void SequentialTaskExecutor::stop() {
		// Send a termination message to the daemon's mailbox
		S4U_Mailbox::put(this->daemon->mailbox_name, new StopDaemonMessage());

		ComputeService::stop();
	}
		/**
	 * @brief Kills the sequential task executor
	 */
		void SequentialTaskExecutor::kill() {
			this->daemon->kill_actor();
		}

	/**
	 * @brief Have the sequential task executor execute a workflow task
	 *
	 * @param task is a pointer to the workflow task
	 * @param callback_mailbox is the name of a mailbox to which a "task done" callback will be sent
	 *
	 * @return 0 on success
	 */
	int SequentialTaskExecutor::runTask(WorkflowTask *task) {
		if (this->state == ComputeService::TERMINATED) {
			throw WRENCHException("Trying to run a task on a compute service that's terminated");
		}
		// Send a "run a task" message to the daemon's mailbox
		S4U_Mailbox::put(this->daemon->mailbox_name, new RunTaskMessage(task));
		return 0;
	};

	/**
	 * @brief Whether the executor is idle or busy
	 *
	 * @return True when idle
	 */
	bool SequentialTaskExecutor::hasIdleCore() {
		return this->daemon->isIdle();
	}
}
