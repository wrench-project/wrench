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


#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <exception/WRENCHException.h>
#include "WorkflowExecutionEvent.h"

namespace wrench {

		WorkflowExecutionEvent::WorkflowExecutionEvent() {
			this->type = WorkflowExecutionEvent::UNDEFINED;
			this->task = nullptr;
			this->compute_service = nullptr;
		}

		std::unique_ptr<WorkflowExecutionEvent> WorkflowExecutionEvent::get_next_execution_event(std::string mailbox) {

			// Get the message on the mailbox
			std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(mailbox);

			std::unique_ptr<WorkflowExecutionEvent> event =
							std::unique_ptr<WorkflowExecutionEvent>(new WorkflowExecutionEvent());

			switch (message->type) {

				case SimulationMessage::TASK_DONE: {
					std::unique_ptr<TaskDoneMessage> m(static_cast<TaskDoneMessage *>(message.release()));
					event->type = WorkflowExecutionEvent::TASK_COMPLETION;
					event->task = m->task;
					event->compute_service = m->compute_service;
					return event;
				}

				case SimulationMessage::TASK_FAILED: {
					std::unique_ptr<TaskFailedMessage> m(static_cast<TaskFailedMessage *>(message.release()));
					event->type = WorkflowExecutionEvent::TASK_FAILURE;
					event->task = m->task;
					event->compute_service = m->compute_service;
					return event;
				}

				default: {
					throw WRENCHException("Non-handled message type when generating execution event");
				}
			}

		}
};