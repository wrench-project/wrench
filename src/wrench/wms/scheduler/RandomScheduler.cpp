/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::RandomScheduler implements a simple random scheduler
 */

#include <xbt.h>
#include <set>
#include <workflow_job/StandardJob.h>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "wms/scheduler/RandomScheduler.h"
#include "job_manager/JobManager.h"
#include "simulation/Simulation.h"
#include "workflow_job/PilotJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(random_scheduler, "Log category for Random Scheduler");

namespace wrench {

		/**
		 * Default constructor
		 */
		RandomScheduler::RandomScheduler() {}

		/**
		 * @brief Schedule and run a set of ready tasks in available compute resources
		 *
		 * @param job_manager is a pointer to a job manager instance
		 * @param ready_tasks is a vector of ready tasks
		 * @param simualtion us a pointer to a simulation instance
		 */
		void RandomScheduler::scheduleTasks(JobManager *job_manager,
																	 std::vector<WorkflowTask *> ready_tasks,
																	 Simulation *simulation) {


			// TODO: Refactor to avoid code duplication

			XBT_INFO("There are %ld ready tasks to schedule", ready_tasks.size());
			for (int i = 0; i < ready_tasks.size(); i++) {
				bool successfully_scheduled = false;

				// First: attempt to run the task on a running pilot job
				std::set<PilotJob*> running_pilot_jobs = job_manager->getRunningPilotJobs();
				for (auto pj : running_pilot_jobs) {
					ComputeService *cs = pj->getComputeService();
					if (cs->isUp() && (cs->getProperty(ComputeService::SUPPORTS_STANDARD_JOBS) == "yes") && (cs->numIdleCores() > 0) ){
						XBT_INFO("Submitting task %s for execution to a pilot job", ready_tasks[i]->getId().c_str());
						WorkflowJob *job = (WorkflowJob *)job_manager->createStandardJob(ready_tasks[i]);
						job_manager->submitJob(job, cs);
						successfully_scheduled = true;
						break;
					}
				}
				if (successfully_scheduled) {
					continue;
				}
				XBT_INFO("Couldn't submit tasks to a pilot job... trying a standard job");
				// Second: attempt to run the task on a compute resource
				std::set<ComputeService *> compute_services = simulation->getComputeServices();
				for (auto cs : compute_services) {
					if ((cs->isUp()) && (cs->getProperty(ComputeService::SUPPORTS_STANDARD_JOBS) == "yes") &&  (cs->numIdleCores() > 0) ){
						XBT_INFO("Submitting task %s for execution as a standard job", ready_tasks[i]->getId().c_str());
						WorkflowJob *job = (WorkflowJob *)job_manager->createStandardJob(ready_tasks[i]);
						job_manager->submitJob(job, cs);
						successfully_scheduled = true;
						break;
					}
				}
				if (!successfully_scheduled) {
					break;
				}

			}
			XBT_INFO("Done with scheduling tasks as standard jobs");
		}

		/**
		 * @brief Submit pilot jobs to compute services
		 *
		 * @param job_manager is a pointer to a job manager instance
		 * @param workflow is a pointer to a workflow instance
		 * @param simulation is a pointer to a simulation instance
		 */
		void RandomScheduler::schedulePilotJobs(JobManager *job_manager,
																					Workflow *workflow,
																					Simulation *simulation) {

			// If there is always a pilot job in the system, do nothing
			if ((job_manager->getRunningPilotJobs().size() > 0) ||
					(job_manager->getRunningPilotJobs().size() > 0)) {
				XBT_INFO("There is already a pilot job in the system...");
				return;
			}

			// Submit a pilot job to the first compute service that can support it
			std::set<ComputeService*> compute_services = simulation->getComputeServices();
			ComputeService *target_service = nullptr;
			for (auto cs : compute_services) {
				if (cs->isUp() && (cs->getProperty(ComputeService::Property::SUPPORTS_PILOT_JOBS) == "yes")) {
					target_service = cs;
					break;
				}
			}
			if (target_service == nullptr) {
				XBT_INFO("No compute service supports pilot jobs");
				return;
			}

			// Submit a pilot job (1 core, 600 seconds)
			XBT_INFO("Submitting a pilot job (1 core, 600 seconds)");
			WorkflowJob *job = (WorkflowJob *)job_manager->createPilotJob(workflow, 1, 600.00);
			job_manager->submitJob(job, target_service);

		}

};