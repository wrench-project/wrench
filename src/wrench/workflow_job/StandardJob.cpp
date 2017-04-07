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


#include <set>
#include <exception/WRENCHException.h>
#include "workflow/Workflow.h"
#include "StandardJob.h"

namespace wrench {

		/***********************************************************/
		/**	DEVELOPER METHODS BELOW **/
		/***********************************************************/

		/*! \cond DEVELOPER */


		/**
		 * @brief Constructor given a vector of tasks
		 * @param tasks is the vector of tasks
		 */
		StandardJob::StandardJob(std::vector<WorkflowTask*> tasks) {
			this->type = WorkflowJob::STANDARD;
			this->num_cores = 1;
			this->duration = 0.0;

			for (auto t : tasks) {
				if (t->getState() != WorkflowTask::READY) {
					throw WRENCHException("All tasks in a StandardJob must be READY");
				}
			}
			for (auto t : tasks) {
				this->tasks.push_back(t);
				t->job = this;
				this->duration += t->getFlops();
			}
			this->num_completed_tasks = 0;
			this->workflow = this->tasks[0]->workflow;

			this->state = StandardJob::State::NOT_SUBMITTED;
			this->name = "standard_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());

		}


		/**
		 * @brief get the number of tasks in the job
		 * @return the number of tasks
		 */
		unsigned long StandardJob::getNumTasks() {
			return this->tasks.size();
		}

		/**
		 * @brief Get the workflow tasks in the job
		 * @return a vector of tasks
		 */
		std::vector<WorkflowTask*> StandardJob::getTasks() {
			return this->tasks;
		}

		/**
		 * @brief Set the containing job
		 * @param job is the job
		 */
		void WorkflowTask::setWorkflowJob(WorkflowJob *job) {
			this->job = job;
		}

		/**
		 * @brief Get the containing job
		 * @return the job
		 */
		WorkflowJob *WorkflowTask::getWorkflowJob() {
			return this->job;
		}

		/*! \endcond */



};