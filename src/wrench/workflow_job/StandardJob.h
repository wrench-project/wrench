/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief A Job that consists of 1 or more READY tasks
 */


#ifndef WRENCH_MULTITASKJOB_H
#define WRENCH_MULTITASKJOB_H


#include <vector>
#include <workflow/WorkflowTask.h>
#include "WorkflowJob.h"

namespace wrench {

		class StandardJob : public WorkflowJob {

		public:
				enum State {
						NOT_SUBMITTED,
						PENDING,
						RUNNING,
						COMPLETED,
						FAILED,
				};

				int num_completed_tasks;

				unsigned long getNumTasks();
				std::vector<WorkflowTask*> getTasks();



		private:
				friend class JobManager;
				friend class JobManagerDaemon;

				StandardJob(std::vector<WorkflowTask*> tasks);
				std::vector<WorkflowTask *> tasks;
				State state;

		};

};


#endif //WRENCH_MULTITASKJOB_H
