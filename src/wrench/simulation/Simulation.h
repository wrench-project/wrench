/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief wrench::Simulation is a top-level class that keeps track of
 *  the simulation state.
 */

#ifndef WRENCH_SIMULATION_H
#define WRENCH_SIMULATION_H

#include <string>
#include <vector>

#include "simgrid_S4U_util/S4U_Simulation.h"
#include "workflow/Workflow.h"
#include "compute_services/multicore_job_executor/MulticoreJobExecutor.h"
#include "wms/WMS.h"
#include "simgrid_MSG_util/MSG_Platform.h"


namespace wrench {

	class Simulation {

	public:
		Simulation();
		void init(int *, char **);
		void createPlatform(std::string);
		void createMulticoreStandardJobExecutor(std::string hostname);
		void createWMS(int wms_id, int sched_id, Workflow *w, std::string hostname);
		void launch();
		void shutdown();
		std::set<ComputeService *> getComputeServices();


	private:
		friend class ComputeService;

		std::unique_ptr<S4U_Simulation> s4u_simulation;

		std::vector<std::unique_ptr<WMS>> WMSes;

		std::vector<std::unique_ptr<ComputeService>> running_compute_services;
		std::vector<std::unique_ptr<ComputeService>> terminated_compute_services;

		void mark_compute_service_as_terminated(ComputeService *cs);


	};

};

#endif //WRENCH_SIMULATION_H
