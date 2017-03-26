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

	constexpr char scheduler_name[] = "RandomScheduler";

	class RandomScheduler : public SchedulerTmpl<scheduler_name, RandomScheduler> {

	public:
		RandomScheduler();

		void runTasks(JobManager *job_manager, std::vector<WorkflowTask *> ready_tasks,
		                      std::set<ComputeService *> &compute_services);
	};
}

#endif //WRENCH_RANDOMSCHEDULER_H
