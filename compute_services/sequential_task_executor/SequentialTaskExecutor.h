//
// Created by Henri Casanova on 2/22/17.
//

#ifndef SIMULATION_SEQUENTIALTASKEXECUTOR_H
#define SIMULATION_SEQUENTIALTASKEXECUTOR_H

#include "../ComputeService.h"
#include "SequentialTaskExecutorDaemon.h"

namespace WRENCH {

		class SequentialTaskExecutor : public ComputeService {

		public:
				SequentialTaskExecutor(std::string hostname);
				~SequentialTaskExecutor();

//				int start();
				int stop();
				int runTask(std::shared_ptr<WorkflowTask> task, std::string callback_mailbox);


		private:
				std::string hostname;
				std::unique_ptr<SequentialTaskExecutorDaemon> main_daemon;

		};
};


#endif //SIMULATION_SEQUENTIALTASKEXECUTOR_H
