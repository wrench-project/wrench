//
// Created by Henri Casanova on 2/22/17.
//

#include "SequentialTaskExecutor.h"
#include "../../simgrid_util/SimgridMailbox.h"

namespace WRENCH {

		SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname) : ComputeService("sequential_task_executor") {
			this->hostname = hostname;
			// Creating the main daemon
			this->main_daemon = std::unique_ptr<SequentialTaskExecutorDaemon>(new SequentialTaskExecutorDaemon());
			// Starting the main daemon
			this->main_daemon->start(this->hostname);
		}

		SequentialTaskExecutor::~SequentialTaskExecutor() {

		}


		int SequentialTaskExecutor::stop() {
			SimgridMailbox::put(this->main_daemon->mailbox, new StopDaemonMessage());
			return 0;
		}

		int SequentialTaskExecutor::runTask(std::shared_ptr<WorkflowTask> task) {

			return 0;
		};



}