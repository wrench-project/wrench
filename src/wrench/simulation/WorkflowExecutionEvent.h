/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief TBD
 */


#ifndef WRENCH_WORKFLOWEXECUTIONEVENT_H
#define WRENCH_WORKFLOWEXECUTIONEVENT_H


#include <string>
#include <workflow/WorkflowTask.h>
#include <compute_services/ComputeService.h>

namespace wrench {


		class WorkflowExecutionEvent {

		public:
				enum EventType {
						UNDEFINED,
						TASK_COMPLETION,
						TASK_FAILURE
				};


				WorkflowExecutionEvent::EventType type;
				WorkflowTask *task;
				ComputeService *compute_service;

		private:

				friend class Workflow;


				WorkflowExecutionEvent();

				static std::unique_ptr<WorkflowExecutionEvent> get_next_execution_event(std::string);

		};

};


#endif //WRENCH_WORKFLOWEXECUTIONEVENT_H
