/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::Scheduler is a mostly abstract implementation of a WMS scheduler
 */

#ifndef WRENCH_SCHEDULER_H
#define WRENCH_SCHEDULER_H

#include "workflow/WorkflowTask.h"
#include "compute_services/ComputeService.h"
#include <set>
#include <job_manager/JobManager.h>

namespace wrench {


		class Scheduler {

		protected:
				Scheduler() {};
				uint16_t sched_type;

		public:
				virtual ~Scheduler() {};

				/**
				 * @brief Schedule and run a set of ready tasks in available compute resources
				 *
				 * @param job_manager is a pointer to a job manager instance
				 * @param ready_tasks is a vector of ready tasks
				 * @param simulation is a pointer to a simulation instance
				 */
				virtual void scheduleTasks(JobManager *job_manager,
																	 std::vector<WorkflowTask *> ready_tasks,
																	 Simulation *simulation) = 0;

				/**
				 * @brief Submits pilot jobs
				 *
				 * @param job_manager is a pointer to a job manager instance
				 * @param workflow is a pointer to a workflow instance
				 * @param simulation is a pointer to a simulation instance
				 */
				virtual void schedulePilotJobs(JobManager *job_manager,
																			 Workflow *workflow,
																			 Simulation *simulation) = 0;
		};

}

#endif //WRENCH_SCHEDULER_H
