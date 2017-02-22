//
// Created by Henri Casanova on 2/22/17.
//

#include <iostream>
#include <simgrid/msg.h>

#include "SequentialTaskExecutorDaemon.h"

namespace WRENCH {

		SequentialTaskExecutorDaemon::SequentialTaskExecutorDaemon(): DaemonWithMailbox("sequential_task_executor_daemon") {

		}

		SequentialTaskExecutorDaemon::~SequentialTaskExecutorDaemon() {

		}

		int SequentialTaskExecutorDaemon::main() {
			std::cerr << "New Sequential Task Executor Daemon started on host " << MSG_host_get_name(MSG_host_self()) << " listening on mailbox" << this->mailbox << std::endl;
			std::cerr << "Sequential Task Executor Daemon started on host " << MSG_host_get_name(MSG_host_self()) << " terminating" << std::endl;
			return 0;
		}

};