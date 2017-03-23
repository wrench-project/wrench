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


#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <exception/WRENCHException.h>
#include "WorkflowExecutionEvent.h"

namespace wrench {

		WorkflowExecutionEvent::WorkflowExecutionEvent() {
			this->type = WorkflowExecutionEvent::UNDEFINED;
			this->job = nullptr;
			this->compute_service = nullptr;
		}

		std::unique_ptr<WorkflowExecutionEvent> WorkflowExecutionEvent::get_next_execution_event(std::string mailbox) {

			// Get the message on the mailbox
			std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(mailbox);

			std::unique_ptr<WorkflowExecutionEvent> event =
							std::unique_ptr<WorkflowExecutionEvent>(new WorkflowExecutionEvent());

			switch (message->type) {

				case SimulationMessage::STANDARD_JOB_DONE: {
					std::unique_ptr<StandardJobDoneMessage> m(static_cast<StandardJobDoneMessage *>(message.release()));
					event->type = WorkflowExecutionEvent::STANDARD_JOB_COMPLETION;
					event->job = m->job;
					event->compute_service = m->compute_service;
					return event;
				}

				case SimulationMessage::STANDARD_JOB_FAILED: {
					std::unique_ptr<StandardJobFailedMessage> m(static_cast<StandardJobFailedMessage *>(message.release()));
					event->type = WorkflowExecutionEvent::STANDARD_JOB_FAILURE;
					event->job = m->job;
					event->compute_service = m->compute_service;
					return event;
				}

				default: {
					throw WRENCHException("Non-handled message type when generating execution event");
				}
			}

		}
};