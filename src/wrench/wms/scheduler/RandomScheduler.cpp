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

XBT_LOG_NEW_DEFAULT_CATEGORY(random_scheduler, "Log category for Random Scheduler");

namespace wrench {

		/**
		 * Default constructor
		 */
		RandomScheduler::RandomScheduler() {}

		/**
		 * Schedule and run a set of ready tasks in available compute resources
		 *
		 * @param ready_tasks is a vector of ready tasks
		 * @param compute_services is a vector of available compute resources
		 * @param callback_mailbox is the name of the mailbox
		 */
		void RandomScheduler::runTasks(std::vector<WorkflowTask *> ready_tasks,
																	 std::set<ComputeService *> &compute_services) {

//			XBT_INFO("There are %ld ready tasks to schedule", ready_tasks.size());
			for (int i = 0; i < ready_tasks.size(); i++) {

				// schedule task to first available compute resource
				bool successfully_scheduled = false;
				for (auto cs : compute_services) {
					unsigned long cs_num_idle_cores = cs->numIdleCores();
					if (cs_num_idle_cores > 0) {
						XBT_INFO("Submitting task %s for execution", ready_tasks[i]->getId().c_str());
						StandardJob *job = new StandardJob(ready_tasks[i]);
						cs->runJob(job);
						successfully_scheduled = true;
						break;
					}
				}
				if (!successfully_scheduled) {
					break;
				}
			}
//			XBT_INFO("Done with scheduling for now");

		}
}