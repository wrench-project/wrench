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
				/** Constructor, initialization, launching, shutting down **/
				Simulation();
				void init(int *, char **);
				void launch();
				void shutdown();

				/** Platform initialization **/
				void createPlatform(std::string);

				/** ComputeService creations **/
				void createMulticoreStandardJobExecutor(std::string hostname);
				void createMulticorePilotJobExecutor(std::string hostname);
				void createMulticoreStandardAndPilotJobExecutor(std::string hostname);

				/** ComputeService discovery **/
				std::set<ComputeService *> getComputeServices();

				/** WMS Creation **/
				void createWMS(int wms_id, int sched_id, Workflow *w, std::string hostname);


		private:
				friend class ComputeService;

				std::unique_ptr<S4U_Simulation> s4u_simulation;

				std::vector<std::unique_ptr<WMS>> WMSes;

				std::vector<std::unique_ptr<ComputeService>> running_compute_services;
				std::vector<std::unique_ptr<ComputeService>> terminated_compute_services;

				void mark_compute_service_as_terminated(ComputeService *cs);

				// Helper function
				void createMulticoreJobExecutor(std::string hostname,
																										std::string supports_standard_jobs,
																										std::string support_pilot_jobs);


				};

};

#endif //WRENCH_SIMULATION_H
