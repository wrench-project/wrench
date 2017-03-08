/**
 *  @file    SequentialTaskExecutor.h
 *  @author  Henri Casanova
 *  @date    2/24/2017
 *  @version 1.0
 *
 *  @brief WRENCH::SequentialTaskExecutor class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::SequentialTaskExecutor class implements a simple
 *  Compute Service abstraction.
 *
 */

#ifndef SIMULATION_SEQUENTIALTASKEXECUTOR_H
#define SIMULATION_SEQUENTIALTASKEXECUTOR_H

#include "compute_services/ComputeService.h"
#include "SequentialTaskExecutorDaemon.h"

namespace WRENCH {

		class SequentialTaskExecutor : public ComputeService {

		public:
				SequentialTaskExecutor(std::string hostname);
				~SequentialTaskExecutor();

				void stop();
				int runTask(WorkflowTask *task, std::string callback_mailbox);

		private:
				std::string hostname;
				std::unique_ptr<SequentialTaskExecutorDaemon> daemon;

		};
};


#endif //SIMULATION_SEQUENTIALTASKEXECUTOR_H
