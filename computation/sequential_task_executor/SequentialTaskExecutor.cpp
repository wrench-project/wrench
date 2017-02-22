//
// Created by Henri Casanova on 2/22/17.
//

#include "SequentialTaskExecutor.h"
#include "SequentialTaskExecutorDaemon.h"

namespace WRENCH {

		SequentialTaskExecutor::SequentialTaskExecutor(std::string hostname) : ComputeService("sequential_task_executor") {
			this->hostname = hostname;
			this->main_daemon = new SequentialTaskExecutorDaemon();
		}

		SequentialTaskExecutor::~SequentialTaskExecutor() {

		}

		int SequentialTaskExecutor::start() {
			this->main_daemon->start(this->hostname);
			return 0;
		}

		int SequentialTaskExecutor::stop() {

			return 0;
		}

		int SequentialTaskExecutor::runTask(std::shared_ptr<WorkflowTask> task) {

			return 0;
		};



}