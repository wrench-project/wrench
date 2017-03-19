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
				StandardJob(std::vector<WorkflowTask*> tasks);
				StandardJob(WorkflowTask *task);

				std::vector<WorkflowTask *> tasks;

				// Callback mailbox management
				std::string pop_callback_mailbox();
				void push_callback_mailbox(std::string);
				int num_completed_tasks;


		private:
				std::stack<std::string> callback_mailbox_stack;		// Stack of callback mailboxes
				Workflow *workflow;


		};

};


#endif //WRENCH_MULTITASKJOB_H
