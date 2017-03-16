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

namespace wrench {

	class Scheduler {

	protected:
		Scheduler() {};
		uint16_t sched_type;

	public:
		virtual ~Scheduler() {};

		/**
		 * Schedule and run a set of ready tasks in available compute resources
		 *
		 * @param ready_tasks is a vector of ready tasks
		 * @param compute_services is a vector of available compute resources
		 * @param callback_mailbox is the name of the mailbox
		 */
		virtual void runTasks(std::vector<WorkflowTask *> ready_tasks,
		                      std::vector<std::unique_ptr<ComputeService>> &compute_services,
		                      std::string callback_mailbox) = 0;
	};

}

#endif //WRENCH_SCHEDULER_H
