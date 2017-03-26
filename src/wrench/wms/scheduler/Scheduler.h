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
		std::string sched_type;

	public:
		virtual ~Scheduler() {};

		/**
		 * Schedule and run a set of ready tasks in available compute resources
		 *
		 * @param job_manager is a pointer to a job manager
		 * @param ready_tasks is a vector of ready tasks
		 * @param compute_services is a vector of available compute resources
		 */
		virtual void runTasks(JobManager *job_manager,
		                      std::vector<WorkflowTask *> ready_tasks,
		                      std::set<ComputeService *> &compute_services) = 0;
	};

}

#endif //WRENCH_SCHEDULER_H
