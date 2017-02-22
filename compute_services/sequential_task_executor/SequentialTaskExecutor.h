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
				int runTask(std::shared_ptr<WorkflowTask> task);


		private:
				std::string hostname;
				SequentialTaskExecutorDaemon *main_daemon;

		};
};


#endif //SIMULATION_SEQUENTIALTASKEXECUTOR_H
