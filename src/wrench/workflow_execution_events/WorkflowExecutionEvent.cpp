/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Workflow.getNextEvent() method.
 */


#include <simgrid_S4U_util/S4U_Mailbox.h>

#include <simulation/SimulationMessage.h>

#include "WorkflowExecutionEvent.h"

namespace wrench {

    /**
     * @brief Block the calling process until a WorkflowExecutionEvent is generated
     *        based on messages received on a mailbox
     *
     * @param mailbox: the name of the receiving mailbox
     * @return a unique pointer to a WorkflowExecutionEvent object
     *
     * @throw std::runtime_error
     */
    std::unique_ptr<WorkflowExecutionEvent> WorkflowExecutionEvent::waitForNextExecutionEvent(std::string mailbox) {

      // Get the message on the mailbox
      std::unique_ptr<SimulationMessage> message = S4U_Mailbox::get(mailbox);

      std::unique_ptr<WorkflowExecutionEvent> event =
              std::unique_ptr<WorkflowExecutionEvent>(new WorkflowExecutionEvent());

      switch (message->type) {

        case SimulationMessage::STANDARD_JOB_DONE: {
          std::unique_ptr<StandardJobDoneMessage> m(static_cast<StandardJobDoneMessage *>(message.release()));
          event->type = WorkflowExecutionEvent::STANDARD_JOB_COMPLETION;
          event->job = (WorkflowJob *) m->job;
          event->compute_service = m->compute_service;
          return event;
        }

        case SimulationMessage::STANDARD_JOB_FAILED: {
          std::unique_ptr<StandardJobFailedMessage> m(static_cast<StandardJobFailedMessage *>(message.release()));
          event->type = WorkflowExecutionEvent::STANDARD_JOB_FAILURE;
          event->job = (WorkflowJob *) m->job;
          event->compute_service = m->compute_service;
          return event;
        }

        case SimulationMessage::PILOT_JOB_STARTED: {
          std::unique_ptr<PilotJobStartedMessage> m(static_cast<PilotJobStartedMessage *>(message.release()));
          event->type = WorkflowExecutionEvent::PILOT_JOB_START;
          event->job = (WorkflowJob *) m->job;
          event->compute_service = m->compute_service;
          return event;
        }

        case SimulationMessage::PILOT_JOB_EXPIRED: {
          std::unique_ptr<PilotJobExpiredMessage> m(static_cast<PilotJobExpiredMessage *>(message.release()));
          event->type = WorkflowExecutionEvent::PILOT_JOB_EXPIRATION;
          event->job = (WorkflowJob *) m->job;
          event->compute_service = m->compute_service;
          return event;
        }

        case SimulationMessage::JOB_TYPE_NOT_SUPPORTED: {
          std::unique_ptr<JobTypeNotSupportedMessage> m(static_cast<JobTypeNotSupportedMessage *>(message.release()));
          event->type = WorkflowExecutionEvent::UNSUPPORTED_JOB_TYPE;
          event->job = (WorkflowJob *) m->job;
          event->compute_service = m->compute_service;
          return event;
        }

        default: {
          throw std::runtime_error("Non-handled message type when generating execution event");
        }
      }
    }


    /**
     * @brief Constructor
     */
    WorkflowExecutionEvent::WorkflowExecutionEvent() {
      this->type = WorkflowExecutionEvent::UNDEFINED;
      this->job = nullptr;
      this->compute_service = nullptr;
    }


};