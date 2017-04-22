/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <set>
#include "workflow/Workflow.h"
#include "StandardJob.h"

namespace wrench {

		/**
		 * @brief Constructor
		 * @param tasks: the vector of WorkflowTasks object that comprise the job
		 *
		 * @throw std::invalid_argument
		 */
		StandardJob::StandardJob(std::vector<WorkflowTask*> tasks) {
			this->type = WorkflowJob::STANDARD;
			this->num_cores = 1;
			this->duration = 0.0;

			for (auto t : tasks) {
				if (t->getState() != WorkflowTask::READY) {
					throw std::invalid_argument("All tasks used to create a StandardJob must be READY");
				}
			}
			for (auto t : tasks) {
				this->tasks.push_back(t);
				t->setJob(this);
				this->duration += t->getFlops();
			}
			this->num_completed_tasks = 0;
			this->workflow = this->tasks[0]->getWorkflow();

			this->state = StandardJob::State::NOT_SUBMITTED;
			this->name = "standard_job_" + std::to_string(WorkflowJob::getNewUniqueNumber());

		}


		/**
		 * @brief Get the number of tasks in the job
		 * @return the number of tasks
		 */
		unsigned long StandardJob::getNumTasks() {
			return this->tasks.size();
		}

		/**
		 * @brief Increment "the number of completed tasks" counter
		 */
		void StandardJob::incrementNumCompletedTasks() {
			this->num_completed_tasks++;
		}

		/**
		 * @brief Get the number of completed tasks in the job
		 * @return the number of completed tasks
		 */
		unsigned long StandardJob::getNumCompletedTasks() {
			return this->num_completed_tasks;
		}


		/**
		 * @brief Get the workflow tasks in the job
		 * @return a vector of pointers to WorkflowTasks objects
		 */
		std::vector<WorkflowTask*> StandardJob::getTasks() {
			return this->tasks;
		}

		/**
		 * @brief Set the containing job
		 * @param job: the containing job
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

};