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

#ifndef WRENCH_RANDOMSCHEDULER_H
#define WRENCH_RANDOMSCHEDULER_H

#include "wms/scheduler/SchedulerFactory.h"

namespace wrench {

		class JobManager;

		extern const char random_name[] = "RandomScheduler";

		class RandomScheduler : public SchedulerTmpl<random_name, RandomScheduler> {

		public:
				RandomScheduler();

				void scheduleTasks(JobManager *job_manager,
													 std::vector<WorkflowTask *> ready_tasks,
													 const std::set<ComputeService *> &compute_services);

				void schedulePilotJobs(JobManager *job_manager,
															 Workflow *workflow,
															 double flops,
															 const std::set<ComputeService *> &compute_services);
		};
};

#endif //WRENCH_RANDOMSCHEDULER_H
