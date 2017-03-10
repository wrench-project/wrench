/**
 *  @file    MulticoreTaskExecutor.h
 *  @author  Henri Casanova
 *  @date    3/7/2017
 *  @version 1.0
 *
 *  @brief WRENCH::MulticoreTaskExecutor class implementation
 *
 *  @section DESCRIPTION
 *
 *  The WRENCH::MulticoreTaskExecutor class implements a simple
 *  Compute Service abstraction.
 *
 */

#ifndef WRENCH_MULTICORETASKEXECUTOR_H
#define WRENCH_MULTICORETASKEXECUTOR_H


#include <workflow/WorkflowTask.h>
#include <compute_services/ComputeService.h>
#include <compute_services/sequential_task_executor/SequentialTaskExecutor.h>
#include <simulation/SimulationMessage.h>
#include "MulticoreTaskExecutorDaemon.h"

namespace WRENCH {

		class MulticoreTaskExecutor : public ComputeService {


		public:
				MulticoreTaskExecutor(std::string hostname);

				~MulticoreTaskExecutor();

				void stop();

				int runTask(WorkflowTask *task, std::string callback_mailbox);

		private:
				std::string hostname;
				std::unique_ptr<MulticoreTaskExecutorDaemon> daemon;
				std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;
		};
};


#endif //WRENCH_MULTICORETASKEXECUTOR_H
