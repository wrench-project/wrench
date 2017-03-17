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

		if (ready_tasks.size() > 0) {
			XBT_INFO("There are %ld ready tasks", ready_tasks.size());
		}
		for (int i = 0; i < ready_tasks.size(); i++) {
			XBT_INFO("Submitting task %s for execution", ready_tasks[i]->getId().c_str());

			// schedule task to first available compute resource
			for (auto cs : compute_services) {
				if (cs->hasIdleCore()) {
					ready_tasks[i]->setScheduled();
					cs->runTask(ready_tasks[i]);
					break;
				}
			}
		}


	}
}