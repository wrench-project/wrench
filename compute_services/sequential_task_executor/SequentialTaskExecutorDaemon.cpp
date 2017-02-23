//
// Created by Henri Casanova on 2/22/17.
//

#include <iostream>
#include <simgrid/msg.h>


#include "SequentialTaskExecutorDaemon.h"
#include "../../simgrid_util/SimgridMessages.h"
#include "../../simgrid_util/SimgridMailbox.h"

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

			while(true) {
				SimgridMessage *message =SimgridMailbox::get(this->mailbox);
				switch(message->type) {
					case SimgridMessage::STOP_DAEMON:
						delete message;
						break;
					default:
						delete message;
						break;
				}

				XBT_INFO("Sequential Task Executor Daemon  on host %s ",
								 MSG_host_get_name(MSG_host_self()));

				return 0;

			}

			std::cerr << "Sequential Task Executor Daemon started on host " << MSG_host_get_name(MSG_host_self()) << " terminating" << std::endl;
			return 0;
		}

};