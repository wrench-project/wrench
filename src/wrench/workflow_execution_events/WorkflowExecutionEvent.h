/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::WorkflowExecutionEvent implements several events that
 * are relevant to the execution of a WMS, which are accessed via some
 * Workflow.getNextEvent() method.
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
						STANDARD_JOB_COMPLETION,
						STANDARD_JOB_FAILURE,
						PILOT_JOB_START,
						PILOT_JOB_EXPIRATION,
				};


				WorkflowExecutionEvent::EventType type;
				WorkflowJob *job;
				ComputeService *compute_service;

		private:
				WorkflowExecutionEvent();

				friend class Workflow;
				static std::unique_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent(std::string);

		};

};


#endif //WRENCH_WORKFLOWEXECUTIONEVENT_H
