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

#ifndef WRENCH_MAXMINSCHEDULER_H
#define WRENCH_MAXMINSCHEDULER_H

#include "wms/scheduler/SchedulerFactory.h"

namespace wrench {

	class JobManager;

	extern const char maxmin_name[] = "MaxMinScheduler";

	class MaxMinScheduler : public SchedulerTmpl<maxmin_name, MaxMinScheduler> {

	public:
		MaxMinScheduler();

		void runTasks(JobManager *job_manager, std::vector<WorkflowTask *> ready_tasks,
		              std::set<ComputeService *> &compute_services);

		struct MaxMinComparator {
			bool operator()(WorkflowTask *&lhs, WorkflowTask *&rhs);
		};

	};
}

#endif //WRENCH_MAXMINSCHEDULER_H
