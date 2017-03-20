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

		/**
		 * @brief Constructor given a vector of tasks
		 * @param tasks is the vector of tasks
		 */
		StandardJob::StandardJob(std::vector<WorkflowTask*> tasks) {
			this->type = WorkflowJob::STANDARD;

			for (auto t : tasks) {
				if (t->getState() != WorkflowTask::READY) {
					throw WRENCHException("All tasks in a StandardJob must be READY");
				}
			}
			for (auto t : tasks) {
				this->tasks.push_back(t);
				t->job = this;
			}
			this->num_completed_tasks = 0;
			this->workflow = this->tasks[0]->workflow;
		}
		/**
		 * Constructor given a single task
		 * @param task
		 */
		StandardJob::StandardJob(WorkflowTask *task) {
			this->type = WorkflowJob::STANDARD;

			if (task->getState() != WorkflowTask::READY) {
				throw WRENCHException("All tasks in a StandardJob must be READY");
			}
			this->tasks.push_back(task);
			task->job = this;
			this->num_completed_tasks = 0;
			this->workflow = this->tasks[0]->workflow;

		}

		/**
		 * @brief Gets the "next" callback mailbox (returns the
		 *         workflow mailbox if the mailbox stack is empty)
		 * @return the next callback mailbox
		 */
		std::string StandardJob::pop_callback_mailbox() {
			if (this->callback_mailbox_stack.size() == 0) {
				return this->workflow->getCallbackMailbox();
			} else {
				std::string mailbox = this->callback_mailbox_stack.top();
				this->callback_mailbox_stack.pop();
				return mailbox;
			}
		}

		/**
		 * @brief pushes a callback mailbox
		 * @param mailbox is the mailbox name
		 */
		void StandardJob::push_callback_mailbox(std::string mailbox) {
			this->callback_mailbox_stack.push(mailbox);
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


};