/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::Simulation is a top-level class that keeps track of
 *  the simulation state.
 */

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>

#include "simgrid_S4U_util/S4U_Simulation.h"
#include "workflow/Workflow.h"
#include "compute_services/sequential_task_executor/SequentialTaskExecutor.h"
#include "compute_services/multicore_task_executor/MulticoreTaskExecutor.h"
#include "wms/sequential_random_WMS/SequentialRandomWMS.h"
#include "simgrid_MSG_util/MSG_Platform.h"


namespace wrench {

	class Simulation;

	class Simulation {

	public:
		Simulation();
		void init(int *, char **);
		void createPlatform(std::string);
		void createSequentialTaskExecutor(std::string hostname);
		void createMulticoreTaskExecutor(std::string hostname);
		void createSimpleWMS(Workflow *w, std::string hostname);
		void launch();
		void shutdown();

		/** for testing development purposes **/
		SequentialTaskExecutor *getSomeSequentialTaskExecutor();
		MulticoreTaskExecutor *getSomeMulticoreTaskExecutor();

	private:
		std::unique_ptr<S4U_Simulation> s4u_simulation;

//				std::unique_ptr<Platform> platform;
		std::vector<std::unique_ptr<SequentialRandomWMS>> WMSes;

		/** for testing development purposes **/
		std::vector<std::unique_ptr<SequentialTaskExecutor>> sequential_task_executors;
		std::vector<std::unique_ptr<MulticoreTaskExecutor>> multicore_task_executors;

	};

};

#endif //WRENCH_SIMULATION_H
