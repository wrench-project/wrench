/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <xbt.h>
#include <set>
#include <workflow_job/StandardJob.h>
#include <logging/Logging.h>

#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "wms/scheduler/MaxMinScheduler.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(maxmin_scheduler, "Log category for Max-Min Scheduler");

namespace wrench {

	/**
	 * @brief Default constructor
	 */
	MaxMinScheduler::MaxMinScheduler() {}

	bool MaxMinScheduler::MaxMinComparator:: operator()(WorkflowTask *&lhs, WorkflowTask *&rhs) {
		return lhs->getFlops() > rhs->getFlops();
	}

	/**
	 * @brief Schedule and run a set of ready tasks in available compute resources
	 *
	 * @param job_manager: a pointer to a JobManager object
	 * @param ready_tasks: a vector of ready WorkflowTask objects (i.e., ready tasks in the workflow)
	 * @param compute_services: a set of pointers to ComputeService objects (i.e., compute services available to run jobs)
	 */
	void MaxMinScheduler::scheduleTasks(JobManager *job_manager,
	                               std::vector<WorkflowTask *> ready_tasks,
	                               const std::set<ComputeService *> &compute_services) {

		std::sort(ready_tasks.begin(), ready_tasks.end(), MaxMinComparator());

		for (auto it : ready_tasks) {

			// schedule task to first available compute resource
			bool successfully_scheduled = false;
			for (auto cs : compute_services) {

				if (cs->canRunJob(WorkflowJob::STANDARD, 1, (*it).getFlops())) {
					WRENCH_INFO("Submitting task %s for execution", (*it).getId().c_str());
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
		* @brief Schedule and run pilot jobs
		*
		* @param job_manager: a pointer to a JobManager object
		* @param workflow: a pointer to a Workflow object
 	  * @param flops: the number of flops that the PilotJob should be able to do before terminating
		* @param compute_services: a set of pointers to ComputeSertvice objects (i.e., compute services available to run jobs)
		*/
		void MaxMinScheduler::schedulePilotJobs(JobManager *job_manager,
																						Workflow *workflow,
																						double flops,
																						const std::set<ComputeService *> &compute_services) {
			WRENCH_INFO("Max-Min Scheduler doesn't do anything with pilot jobs");

			return;
		}

}