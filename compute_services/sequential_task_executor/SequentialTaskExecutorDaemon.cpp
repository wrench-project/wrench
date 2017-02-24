//
// Created by Henri Casanova on 2/22/17.
//

#include <iostream>
#include <simgrid/msg.h>


#include "SequentialTaskExecutorDaemon.h"
#include "../../simgrid_util/Message.h"
#include "../../simgrid_util/Mailbox.h"
#include "../../simgrid_util/Computation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sequential_task_executor_daemon, "Log category for Sequential Task Executor Daemon");


namespace WRENCH {

		SequentialTaskExecutorDaemon::SequentialTaskExecutorDaemon(): DaemonWithMailbox("sequential_task_executor_daemon") {

		}

		SequentialTaskExecutorDaemon::~SequentialTaskExecutorDaemon() {

		}

		int SequentialTaskExecutorDaemon::main() {
			XBT_INFO("New Sequential Task Executor Daemon started on host %s (%s) ",
							 MSG_host_get_name(MSG_host_self()),
							 this->mailbox.c_str());

			bool keep_going = true;
			while(keep_going) {

				std::unique_ptr<Message> message = Mailbox::get(this->mailbox);

				switch(message->type) {

					case Message::STOP_DAEMON: {
						keep_going = false;
						break;
					}

					case Message::RUN_TASK: {
						std::unique_ptr<RunTaskMessage> m(static_cast<RunTaskMessage*>(message.release()));
						/* Run the task */
						XBT_INFO("Executing task %s",
										 m->task->id.c_str());
						Computation::simulateComputation(m->task->execution_time);

						/* Send the callback */
						XBT_INFO("Notifying mailbox %s that task %s has finished",
										 m->callback_mailbox.c_str(),
										 m->task->id.c_str());

						Mailbox::put(m->callback_mailbox, new TaskDoneMessage(m->task));
						break;
					}

					default: {
						XBT_INFO("UNKNOWN MESSAGE TYPE - ABORTING");
						keep_going = false;
						break;
					}
				}

			}

			XBT_INFO("Sequential Task Executor Daemon on host %s terminated!", MSG_host_get_name(MSG_host_self()));
			return 0;



		}

};