/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::MulticorePilotjobExecutor implements a simple
 *  Compute Service abstraction for a multi-core pilotjob executor.
 */


#ifndef WRENCH_MULTICOREPILOTJOBEXECUTOR_H
#define WRENCH_MULTICOREPILOTJOBEXECUTOR_H


#include <compute_services/ComputeService.h>
#include <helper_daemons/sequential_task_executor/SequentialTaskExecutor.h>
#include "MulticorePilotJobExecutorDaemon.h"

namespace wrench {

		class MulticorePilotJobExecutor : public ComputeService {

		public:
				MulticorePilotJobExecutor(std::string hostname, Simulation *simulation);

		private:
				std::unique_ptr<MulticorePilotJobExecutorDaemon> daemon;
				std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;
		};

};


#endif //WRENCH_MULTICOREPILOTJOBEXECUTOR_H
