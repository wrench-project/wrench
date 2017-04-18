/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MINMINSCHEDULER_H
#define WRENCH_MINMINSCHEDULER_H

#include "wms/scheduler/SchedulerFactory.h"

namespace wrench {

		class JobManager;

		extern const char scheduler_name[] = "MinMinScheduler";

		/**
 		 * @brief A min-min Scheduler
 		 */
		class MinMinScheduler : public SchedulerTmpl<scheduler_name, MinMinScheduler> {

		public:
				MinMinScheduler();

				void scheduleTasks(JobManager *job_manager, std::vector<WorkflowTask *> ready_tasks,
													 const std::set<ComputeService *> &compute_services);
				void schedulePilotJobs(JobManager *job_manager,
															 Workflow *workflow,
															 double flops,
															 const std::set<ComputeService *> &compute_services);

				/**
				 * @brief Helper struct for the MinMinScheduler
				 */
				struct MinMinComparator {
						bool operator()(WorkflowTask *&lhs, WorkflowTask *&rhs);
				};

		};
}

#endif //WRENCH_MINMINSCHEDULER_H
