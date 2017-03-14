/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::MulticoreTaskExecutor implements a simple
 *  Compute Service abstraction for a multi-core task executor.
 */

#ifndef WRENCH_MULTICORETASKEXECUTOR_H
#define WRENCH_MULTICORETASKEXECUTOR_H


#include <workflow/WorkflowTask.h>
#include <compute_services/ComputeService.h>
#include <compute_services/sequential_task_executor/SequentialTaskExecutor.h>
#include <simulation/SimulationMessage.h>
#include "MulticoreTaskExecutorDaemon.h"

namespace wrench {

		class MulticoreTaskExecutor : public ComputeService {

		public:
				MulticoreTaskExecutor(std::string hostname);
				void stop();
				int runTask(WorkflowTask *task, std::string callback_mailbox);

		private:
				std::string hostname;
				std::unique_ptr<MulticoreTaskExecutorDaemon> daemon;
				std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;
		};
};


#endif //WRENCH_MULTICORETASKEXECUTOR_H
