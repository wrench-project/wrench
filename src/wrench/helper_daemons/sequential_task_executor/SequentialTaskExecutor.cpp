/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <simgrid_S4U_util/S4U_Simulation.h>
#include <logging/Logging.h>
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "exception/WRENCHException.h"
#include "SequentialTaskExecutor.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor, "Log category for Sequential Task Executor");

namespace wrench {

		SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname, std::string callback_mailbox) :
						S4U_DaemonWithMailbox("sequential_task_executor", "sequential_task_executor") {

			this->hostname = hostname;
			this->callback_mailbox = callback_mailbox;

			// Start my daemon on the host
			this->start(this->hostname);
		}

		void SequentialTaskExecutor::stop() {
			// Send a termination message to the daemon's mailbox
			S4U_Mailbox::put(this->mailbox_name, new StopDaemonMessage(0.0));
		}

		void SequentialTaskExecutor::kill() {
			this->kill_actor();
		}

		int SequentialTaskExecutor::runTask(WorkflowTask *task) {
			// Send a "run a task" message to the daemon's mailbox
			S4U_Mailbox::put(this->mailbox_name, new RunTaskMessage(task, 0.0));
			return 0;
		};

		int SequentialTaskExecutor::main() {

			Logging::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_BLUE);

			WRENCH_INFO("New Sequential Task Executor starting (%s) ", this->mailbox_name.c_str());

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
						WRENCH_INFO("Executing task %s (%lf flops)", m->task->getId().c_str(), m->task->getFlops());
						m->task->setRunning();
						S4U_Simulation::compute(m->task->flops);

						// Set the task completion time and state
						m->task->end_date = S4U_Simulation::getClock();
						m->task->setCompleted();

						// Send the callback
						WRENCH_INFO("Notifying mailbox %s that task %s has finished",
										 this->callback_mailbox.c_str(),
										 m->task->id.c_str());
						S4U_Mailbox::dput(this->callback_mailbox,
															new TaskDoneMessage(m->task, this, 0.0));

						break;
					}

					default: {
						throw WRENCHException("Unknown message type");
					}
				}
			}

			WRENCH_INFO("Sequential Task Executor Daemon on host %s terminated!", S4U_Simulation::getHostName().c_str());

			return 0;
		}

}
