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

#include "SequentialTaskExecutorDaemon.h"
#include "simgrid_util/Message.h"
#include "simgrid_util/Mailbox.h"
#include "simgrid_util/Computation.h"
#include "simgrid_util/Host.h"
#include "simgrid_util/Clock.h"
#include "exception/WRENCHException.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor_daemon, "Log category for Sequential Task Executor Daemon");

namespace WRENCH {

		/**
		 * @brief Constructor
		 */
		SequentialTaskExecutorDaemon::SequentialTaskExecutorDaemon(ComputeService *cs): DaemonWithMailbox("sequential_task_executor_daemon") {
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
			XBT_INFO("New Sequential Task Executor Daemon started on host %s (%s) ",
							 Host::getHostName().c_str(),
							 this->mailbox.c_str());

			bool keep_going = true;
			while(keep_going) {

				// Wait for a message
				std::unique_ptr<Message> message = Mailbox::get(this->mailbox);

				switch(message->type) {

					case Message::STOP_DAEMON: {
						keep_going = false;
						break;
					}

					case Message::RUN_TASK: {
						std::unique_ptr<RunTaskMessage> m(static_cast<RunTaskMessage*>(message.release()));
						// Run the task
						XBT_INFO("Executing task %s",
										 m->task->id.c_str());
						m->task->setRunning();
						Computation::simulateComputation(m->task->flops);
						m->task->end_date = Clock::getClock();
						m->task->setCompleted();


						// Send the callback
						XBT_INFO("Notifying mailbox %s that task %s has finished",
										 m->callback_mailbox.c_str(),
										 m->task->id.c_str());
						Mailbox::iput(m->callback_mailbox, new TaskDoneMessage(m->task, this->compute_service));

						break;
					}

					default: {
						throw WRENCHException("Unknown message type");
					}
				}
			}

			XBT_INFO("Sequential Task Executor Daemon on host %s terminated!", Host::getHostName().c_str());
			return 0;
		}

};