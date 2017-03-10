/**
 *  @file    SequentialTaskExecutorDaemon.cpp
 *  @author  Henri Casanova
 *  @date    2/22/2017
 *  @version 1.0
 *
 *  @brief WRENCH::SequentialTaskExecutorDaemon class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::SequentialTaskExecutorDaemon class implements the daemon for a simple
 *  Compute Service abstraction.
 *
 */

#include <iostream>
#include <simgrid/msg.h>
#include <simgrid_Sim4U_util/S4U_Mailbox.h>
#include <simgrid_Sim4U_util/S4U_Simulation.h>

#include "SequentialTaskExecutorDaemon.h"
#include "simulation/SimulationMessage.h"
#include "exception/WRENCHException.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor_daemon, "Log category for Sequential Task Executor Daemon");

namespace WRENCH {

		/**
		 * @brief Constructor
		 */
		SequentialTaskExecutorDaemon::SequentialTaskExecutorDaemon(ComputeService *cs) :
						S4U_DaemonWithMailbox("sequential_task_executor", "sequential_task_executor") {
			this->compute_service = cs;
		}

		/**
		 * @brief Destructor
		 */
		SequentialTaskExecutorDaemon::~SequentialTaskExecutorDaemon() {
		}

		/**
		 * @brief main() method of the daemon
		 *
		 * @return 0 on termination
		 */
		int SequentialTaskExecutorDaemon::main() {
			XBT_INFO("New Sequential Task Executor starting (%s) ",
							 this->mailbox_name.c_str());

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
						XBT_INFO("Executing task %s",
										 m->task->id.c_str());

						m->task->setRunning();

						S4U_Simulation::compute(m->task->flops);

						// TODO: Find a way to get the clock()
						m->task->end_date = S4U_Simulation::getClock();
						m->task->setCompleted();

						// Send the callback
						XBT_INFO("Notifying mailbox %s that task %s has finished",
										 m->callback_mailbox.c_str(),
										 m->task->id.c_str());
						S4U_Mailbox::iput(m->callback_mailbox, new TaskDoneMessage(m->task, this->compute_service));

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
