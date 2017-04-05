/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::MaxMinScheduler implements a Max-Min algorithm
 */

#include <xbt.h>
#include <set>
#include <workflow_job/StandardJob.h>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "wms/scheduler/MaxMinScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(maxmin_scheduler, "Log category for Max-Min Scheduler");

namespace wrench {

	/**
	 * Default constructor
	 */
	MaxMinScheduler::MaxMinScheduler() {}

	bool MaxMinScheduler::MaxMinComparator:: operator()(WorkflowTask *&lhs, WorkflowTask *&rhs) {
		return lhs->getFlops() > rhs->getFlops();
	}

	/**
	 * @brief Schedule and run a set of ready tasks in available compute resources
	 *
	 * @param job_manager
	 * @param ready_tasks is a vector of ready tasks
	 * @param compute_services is a vector of available compute resources
	 */
	void MaxMinScheduler::scheduleTasks(JobManager *job_manager,
	                               std::vector<WorkflowTask *> ready_tasks,
	                               const std::set<ComputeService *> &compute_services) {

		std::sort(ready_tasks.begin(), ready_tasks.end(), MaxMinComparator());

		for (auto it : ready_tasks) {

			// schedule task to first available compute resource
			bool successfully_scheduled = false;
			for (auto cs : compute_services) {

				unsigned long cs_num_idle_cores = cs->numIdleCores();
				if (cs_num_idle_cores > 0) {
					XBT_INFO("Submitting task %s for execution", (*it).getId().c_str());
					StandardJob *job = job_manager->createStandardJob(it);
					cs->runStandardJob(job);
					successfully_scheduled = true;
					break;
				}
			}
			if (!successfully_scheduled) {
				break;
			}
		}


	}

		/**
		* @brief Submits pilot jobs
		*
		* @param job_manager is a pointer to a job manager instance
		* @param workflow is a pointer to a workflow instance
		* @param compute_services is a set of compute services
		*/
		void MaxMinScheduler::schedulePilotJobs(JobManager *job_manager,
																						Workflow *workflow,
																						const std::set<ComputeService *> &compute_services) {
			XBT_INFO("Max-Min Scheduler doesn't do anything with pilot jobs");

			return;
		}

}