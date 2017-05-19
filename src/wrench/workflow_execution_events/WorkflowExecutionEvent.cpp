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
#include <services/compute_services/ComputeServiceMessage.h>

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

      if (ComputeServiceStandardJobDoneMessage *m = dynamic_cast<ComputeServiceStandardJobDoneMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::STANDARD_JOB_COMPLETION;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
        return event;
      } else if (ComputeServiceStandardJobFailedMessage *m = dynamic_cast<ComputeServiceStandardJobFailedMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::STANDARD_JOB_FAILURE;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
        event->cause = m->cause;
        return event;
      } else if (ComputeServicePilotJobStartedMessage * m = dynamic_cast<ComputeServicePilotJobStartedMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::PILOT_JOB_START;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
        return event;
      } else if (ComputeServicePilotJobExpiredMessage *m = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::PILOT_JOB_EXPIRATION;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
        return event;
      } else if (ComputeServiceJobTypeNotSupportedMessage *m = dynamic_cast<ComputeServiceJobTypeNotSupportedMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::UNSUPPORTED_JOB_TYPE;
        event->job = m->job;
        event->compute_service = m->compute_service;
        return event;
      } else {
        throw std::runtime_error("Non-handled message type when generating execution event");
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