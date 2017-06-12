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
#include <services/storage_services/StorageServiceMessage.h>
#include <exceptions/WorkflowExecutionException.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(workflow_execution_event, "Log category for Workflow Execution Event");


namespace wrench {

    /**
     * @brief Block the calling process until a WorkflowExecutionEvent is generated
     *        based on messages received on a mailbox
     *
     * @param mailbox: the name of the receiving mailbox
     * @return a unique pointer to a WorkflowExecutionEvent object
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    std::unique_ptr<WorkflowExecutionEvent> WorkflowExecutionEvent::waitForNextExecutionEvent(std::string mailbox) {

      // Get the message from the mailbox
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(mailbox);
      } catch (std::runtime_error &e) {
        if (!strcmp(e.what(), "network_error")) {
          throw WorkflowExecutionException(new NetworkError());
        } else {
          throw std::runtime_error(
                  "WorkflowExecutionEvent::waitForNextExecutionEvent(): Unknown exception: " + std::string(e.what()));
        }
      }

      std::unique_ptr<WorkflowExecutionEvent> event =
              std::unique_ptr<WorkflowExecutionEvent>(new WorkflowExecutionEvent());

      if (ComputeServiceStandardJobDoneMessage *m = dynamic_cast<ComputeServiceStandardJobDoneMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::STANDARD_JOB_COMPLETION;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
      } else if (ComputeServiceStandardJobFailedMessage *m = dynamic_cast<ComputeServiceStandardJobFailedMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::STANDARD_JOB_FAILURE;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
        event->failure_cause = m->cause;
      } else if (ComputeServicePilotJobStartedMessage *m = dynamic_cast<ComputeServicePilotJobStartedMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::PILOT_JOB_START;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
      } else if (ComputeServicePilotJobExpiredMessage *m = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::PILOT_JOB_EXPIRATION;
        event->job = (WorkflowJob *) m->job;
        event->compute_service = m->compute_service;
      } else if (ComputeServiceJobTypeNotSupportedMessage *m = dynamic_cast<ComputeServiceJobTypeNotSupportedMessage *>(message.get())) {
        event->type = WorkflowExecutionEvent::UNSUPPORTED_JOB_TYPE;
        event->job = m->job;
        event->compute_service = m->compute_service;
      } else if (StorageServiceFileCopyAnswerMessage *m = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {
        if (m->success) {
          event->type = WorkflowExecutionEvent::FILE_COPY_COMPLETION;
          event->file = m->file;
          event->storage_service = m->storage_service;
        } else {
          event->type = WorkflowExecutionEvent::FILE_COPY_FAILURE;
          event->file = m->file;
          event->storage_service = m->storage_service;
          event->failure_cause = m->failure_cause;
        }
      } else {
        throw std::runtime_error("Non-handled message type when generating execution event");
      }
      return event;
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