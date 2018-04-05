/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Workflow.getNextEvent() method.
 */


#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>

#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "services/storage/StorageServiceMessage.h"
#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(workflow_execution_event, "Log category for Workflow Execution Event");


namespace wrench {

    /**
     * @brief Block the calling process until a WorkflowExecutionEvent is generated
     *        based on messages received on a mailbox
     *
     * @param mailbox: the name of the receiving mailbox
     * @return a workflow execution event
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    std::unique_ptr<WorkflowExecutionEvent> WorkflowExecutionEvent::waitForNextExecutionEvent(std::string mailbox) {

      // Get the message from the mailbox_name
      std::unique_ptr<SimulationMessage> message = nullptr;
      try {
        message = S4U_Mailbox::getMessage(mailbox);
      } catch (std::shared_ptr<NetworkError> &cause) {
        throw WorkflowExecutionException(cause);
      }

      if (auto m = dynamic_cast<ComputeServiceStandardJobDoneMessage *>(message.get())) {
        return std::unique_ptr<StandardJobCompletionEvent>(
          new StandardJobCompletionEvent(m->job, m->compute_service));

      } else if (auto m = dynamic_cast<ComputeServiceStandardJobFailedMessage *>(message.get())) {
        return std::unique_ptr<StandardJobFailedEvent>(
                new StandardJobFailedEvent(m->job, m->compute_service, m->cause));

      } else if (auto m = dynamic_cast<ComputeServicePilotJobStartedMessage *>(message.get())) {
        return std::unique_ptr<PilotJobStartedEvent>(
                new PilotJobStartedEvent(m->job, m->compute_service));

      } else if (auto m = dynamic_cast<ComputeServicePilotJobExpiredMessage *>(message.get())) {
      return std::unique_ptr<PilotJobExpiredEvent>(
              new PilotJobExpiredEvent(m->job, m->compute_service));

      } else if (auto m = dynamic_cast<StorageServiceFileCopyAnswerMessage *>(message.get())) {
        if (m->success) {
          return std::unique_ptr<FileCopyCompletedEvent>(
                  new FileCopyCompletedEvent(m->file, m->storage_service));

        } else {
          return std::unique_ptr<FileCopyFailedEvent>(
                  new FileCopyFailedEvent(m->file, m->storage_service, m->failure_cause));
        }
      } else {
        throw std::runtime_error(
                "WorkflowExecutionEvent::waitForNextExecutionEvent(): Non-handled message type when generating execution event");
      }
    }


};
