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
#include "compute_services/multicore_job_executor/MulticoreJobExecutorDaemon.h"

namespace wrench {

		class Simulation;
		
		class MulticoreJobExecutor : public ComputeService {

		public:
				/** Construct/Start, Stop **/
				MulticoreJobExecutor(Simulation *simulation, std::string hostname, int num_worker_threads = -1, double ttl = -1.0);
				void stop();

				/** Run jobs **/
				int runStandardJob(StandardJob *job);
				int runPilotJob(PilotJob *job);

				/** Get information **/
				unsigned long numIdleCores();

		private:
				std::unique_ptr<MulticoreJobExecutorDaemon> daemon;
		};
};


#endif //WRENCH_MULTICORETASKEXECUTOR_H
