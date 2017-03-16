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

	class RandomScheduler : public SchedulerTmpl<1, RandomScheduler> {

	public:
		RandomScheduler();

		virtual void runTasks(std::vector<WorkflowTask *> ready_tasks,
		                      std::vector<std::unique_ptr<ComputeService>> &compute_services,
		                      std::string callback_mailbox);
	};
}

#endif //WRENCH_RANDOMSCHEDULER_H
