/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialTaskExecutorDaemon implements the daemon for the
 *  SequentialTaskExecutor.
 */

#include <iostream>
#include <simgrid/msg.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simgrid_S4U_util/S4U_Simulation.h>

#include "SequentialTaskExecutorDaemon.h"
#include "simulation/SimulationMessage.h"
#include "exception/WRENCHException.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor_daemon, "Log category for Sequential Task Executor Daemon");

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param cs is a pointer to the corresponding compute service
	 */
	SequentialTaskExecutorDaemon::SequentialTaskExecutorDaemon(std::string cb, SequentialTaskExecutor *executor) :
			S4U_DaemonWithMailbox("sequential_task_executor", "sequential_task_executor") {
		this->callback_mailbox = cb;
		this->task_executor = executor;
	}

	/**
	 * @brief Main method of the sequential task executor daemon
	 *
	 * @return 0 on termination
	 */
	int SequentialTaskExecutorDaemon::main() {

		XBT_INFO("New Sequential Task Executor starting (%s) ", this->mailbox_name.c_str());

		bool keep_going = true;
		while (keep_going) {

			// Wait for a message
			std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(this->mailbox_name);

			switch (message->type) {

				case SimulationMessage::STOP_DAEMON: {
					keep_going = false;
					break;
				}

				case SimulationMessage::RUN_TASK: {
					std::unique_ptr<RunTaskMessage> m(static_cast<RunTaskMessage *>(message.release()));

					// Run the task
					XBT_INFO("Executing task %s", m->task->getId().c_str());
					m->task->setRunning();
					S4U_Simulation::compute(m->task->flops);

					// Set the task completion time and state
					m->task->end_date = S4U_Simulation::getClock();
					m->task->setCompleted();

					// Send the callback
					XBT_INFO("Notifying mailbox %s that task %s has finished",
					         this->callback_mailbox.c_str(),
					         m->task->id.c_str());
					S4U_Mailbox::put(this->callback_mailbox,
														new TaskDoneMessage(m->task, this->task_executor));

					break;
				}

				default: {
					throw WRENCHException("Unknown message type");
				}
			}
		}

		XBT_INFO("Sequential Task Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());
		return 0;
	}

};
