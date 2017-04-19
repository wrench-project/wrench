/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_MULTITASKJOB_H
#define WRENCH_MULTITASKJOB_H


#include <vector>
#include <workflow/WorkflowTask.h>
#include "WorkflowJob.h"

namespace wrench {

		/***********************/
		/** \cond DEVELOPER    */
		/***********************/

		/**
		 * @brief A standard (i.e., non-pilot) WorkflowJob
		 */
		class StandardJob : public WorkflowJob {

		public:
				enum State {
						NOT_SUBMITTED,
						PENDING,
						RUNNING,
						COMPLETED,
						FAILED,
				};


				std::vector<WorkflowTask*> getTasks();
				unsigned long getNumCompletedTasks();
				unsigned long getNumTasks();



		private:
				friend class JobManager;
				friend class JobManagerDaemon;
				friend class MulticoreJobExecutor;

				StandardJob(std::vector<WorkflowTask*> tasks);
				std::vector<WorkflowTask *> tasks;
				State state;
				unsigned long num_completed_tasks;

		};

		/***********************/
		/** \endcond DEVELOPER */
		/***********************/


};


#endif //WRENCH_MULTITASKJOB_H
