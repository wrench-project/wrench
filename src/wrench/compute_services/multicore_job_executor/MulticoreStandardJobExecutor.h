/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::MulticoreJobExecutor implements a simple
 *  Compute Service abstraction for a multi-core task executor.
 */

#ifndef WRENCH_MULTICORETASKEXECUTOR_H
#define WRENCH_MULTICORETASKEXECUTOR_H


#include "workflow/WorkflowTask.h"
#include "compute_services/ComputeService.h"
#include "helper_daemons/sequential_task_executor/SequentialTaskExecutor.h"
#include "simulation/SimulationMessage.h"
#include "simulation/Simulation.h"
#include "compute_services/multicore_job_executor/MulticoreStandardJobExecutorDaemon.h"

namespace wrench {

	class MulticoreStandardJobExecutor : public ComputeService {

	public:
//		MulticoreStandardJobExecutor(std::string hostname, Simulation *simulation);
//		MulticoreStandardJobExecutor(std::string hostname, int num_worker_threads, Simulation *simulation);
		MulticoreStandardJobExecutor(Simulation *simulation, std::string hostname, int num_worker_threads = -1, double ttl = -1.0);
		void stop();
		int runStandardJob(StandardJob *job);
		unsigned long numIdleCores();

	private:
		std::unique_ptr<MulticoreStandardJobExecutorDaemon> daemon;
		std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;
	};
};


#endif //WRENCH_MULTICORETASKEXECUTOR_H
