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
#include "wrench/managers/job_manager/JobManagerMessage.h"
#include "wrench/managers/data_movement_manager/DataMovementManagerMessage.h"

#include <wrench/simulation/SimulationMessage.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/execution_controller/ExecutionControllerMessage.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/exceptions/ExecutionException.h>

WRENCH_LOG_CATEGORY(wrench_core_workflow_execution_event, "Log category for Workflow Execution Event");


namespace wrench {

    /**
     * @brief Block the calling process until a ExecutionEvent is generated
     *        based on messages received on a mailbox
     *
     * @param mailbox: the name of the receiving mailbox
     * @return a workflow execution event
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    std::shared_ptr<ExecutionEvent> ExecutionEvent::waitForNextExecutionEvent(simgrid::s4u::Mailbox *mailbox) {
        return ExecutionEvent::waitForNextExecutionEvent(mailbox, -1);
    }

    /**
     * @brief Block the calling process until a ExecutionEvent is generated
     *        based on messages received on a mailbox, or until a timeout ooccurs
     *
     * @param mailbox: the name of the receiving mailbox
     * @param timeout: a timeout value in seconds (-1 means: no timeout)
     * @return a workflow execution event (or nullptr in case of a timeout)
     *
     * @throw ExecutionException
     * @throw std::runtime_error
     */
    std::shared_ptr<ExecutionEvent>
    ExecutionEvent::waitForNextExecutionEvent(simgrid::s4u::Mailbox *mailbox, double timeout) {
        // Get the message from the mailbox_name
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage<SimulationMessage>(mailbox, timeout);
        } catch (ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<NetworkError>(e.getCause());
            if (cause->isTimeout()) {
                return nullptr;
            } else {
                throw;
            }
        }

        if (auto m = std::dynamic_pointer_cast<JobManagerCompoundJobCompletedMessage>(message)) {
            return std::shared_ptr<CompoundJobCompletedEvent>(
                    new CompoundJobCompletedEvent(m->job, m->compute_service));

        } else if (auto m = std::dynamic_pointer_cast<JobManagerCompoundJobFailedMessage>(message)) {
            return std::shared_ptr<CompoundJobFailedEvent>(
                    new CompoundJobFailedEvent(m->job, m->compute_service, m->cause));

        } else if (auto m = std::dynamic_pointer_cast<JobManagerStandardJobCompletedMessage>(message)) {
            std::set<std::shared_ptr<WorkflowTask>> failure_count_increments;
            m->job->applyTaskUpdates(m->necessary_state_changes, failure_count_increments);
            return std::shared_ptr<StandardJobCompletedEvent>(
                    new StandardJobCompletedEvent(m->job, m->compute_service));

        } else if (auto m = std::dynamic_pointer_cast<JobManagerStandardJobFailedMessage>(message)) {
            m->job->applyTaskUpdates(m->necessary_state_changes, m->necessary_failure_count_increments);
            return std::shared_ptr<StandardJobFailedEvent>(
                    new StandardJobFailedEvent(m->job, m->compute_service, m->cause));

        } else if (auto m = std::dynamic_pointer_cast<ComputeServicePilotJobStartedMessage>(message)) {
            return std::shared_ptr<PilotJobStartedEvent>(new PilotJobStartedEvent(m->job, m->compute_service));

        } else if (auto m = std::dynamic_pointer_cast<ComputeServicePilotJobExpiredMessage>(message)) {
            return std::shared_ptr<PilotJobExpiredEvent>(new PilotJobExpiredEvent(m->job, m->compute_service));

        } else if (auto m = std::dynamic_pointer_cast<DataManagerFileCopyAnswerMessage>(message)) {
            if (m->success) {
                return std::shared_ptr<FileCopyCompletedEvent>(new FileCopyCompletedEvent(
                        m->src_location, m->dst_location));

            } else {
                return std::shared_ptr<FileCopyFailedEvent>(
                        new FileCopyFailedEvent(m->src_location, m->dst_location, m->failure_cause));
            }
        } else if (auto m = std::dynamic_pointer_cast<DataManagerFileReadAnswerMessage>(message)) {
            if (m->success) {
                return std::shared_ptr<FileReadCompletedEvent>(new FileReadCompletedEvent(
                        m->location, m->num_bytes));

            } else {
                return std::shared_ptr<FileReadFailedEvent>(
                        new FileReadFailedEvent(m->location, m->num_bytes, m->failure_cause));
            }
        } else if (auto m = std::dynamic_pointer_cast<DataManagerFileWriteAnswerMessage>(message)) {
            if (m->success) {
                return std::shared_ptr<FileWriteCompletedEvent>(new FileWriteCompletedEvent(
                        m->location));

            } else {
                return std::shared_ptr<FileWriteFailedEvent>(
                        new FileWriteFailedEvent(m->location, m->failure_cause));
            }

        } else if (auto m = std::dynamic_pointer_cast<ExecutionControllerAlarmTimerMessage>(message)) {
            return std::shared_ptr<TimerEvent>(new TimerEvent(m->message));
        } else {
            throw std::runtime_error(
                    "ExecutionEvent::waitForNextExecutionEvent(): Non-handled message type when generating execution event (" +
                    message->getName() + ")");
        }
    }

}// namespace wrench
