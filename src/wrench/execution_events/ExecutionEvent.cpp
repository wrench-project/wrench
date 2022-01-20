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
#include <managers/JobManagerMessage.h>

#include <wrench/simulation/SimulationMessage.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/execution_controller/ExecutionControllerMessage.h>
#include "services/storage/StorageServiceMessage.h"
#include <wrench/exceptions/ExecutionException.h>
#include "wrench.h"

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
        std::unique_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(mailbox, timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            if (cause->isTimeout()) {
                return nullptr;
            }
            throw ExecutionException(cause);
        }

        if (auto m = dynamic_cast<JobManagerCompoundJobCompletedMessage*>(message.get())) {
            return std::shared_ptr<CompoundJobCompletedEvent>(
                    new CompoundJobCompletedEvent(m->job, m->compute_service));

        } else if (auto m = dynamic_cast<JobManagerCompoundJobFailedMessage*>(message.get())) {
            return std::shared_ptr<CompoundJobFailedEvent>(
                    new CompoundJobFailedEvent(m->job, m->compute_service, m->cause));

        } else if (auto m = dynamic_cast<JobManagerStandardJobCompletedMessage*>(message.get())) {
            std::set<std::shared_ptr<WorkflowTask>> failure_count_increments;
            m->job->applyTaskUpdates(m->necessary_state_changes, failure_count_increments);
            return std::shared_ptr<StandardJobCompletedEvent>(
                    new StandardJobCompletedEvent(m->job, m->compute_service));

        } else if (auto m = dynamic_cast<JobManagerStandardJobFailedMessage*>(message.get())) {
            m->job->applyTaskUpdates(m->necessary_state_changes, m->necessary_failure_count_increments);
            return std::shared_ptr<StandardJobFailedEvent>(
                    new StandardJobFailedEvent(m->job, m->compute_service, m->cause));

        } else if (auto m = dynamic_cast<ComputeServicePilotJobStartedMessage*>(message.get())) {
            return std::shared_ptr<PilotJobStartedEvent>(new PilotJobStartedEvent(m->job, m->compute_service));

        } else if (auto m = dynamic_cast<ComputeServicePilotJobExpiredMessage*>(message.get())) {
            return std::shared_ptr<PilotJobExpiredEvent>(new PilotJobExpiredEvent(m->job, m->compute_service));

        } else if (auto m = dynamic_cast<StorageServiceFileCopyAnswerMessage*>(message.get())) {
            if (m->success) {
                return std::shared_ptr<FileCopyCompletedEvent>(new FileCopyCompletedEvent(
                        m->file, m->src, m->dst, m->file_registry_service, m->file_registry_service_updated));

            } else {
                return std::shared_ptr<FileCopyFailedEvent>(
                        new FileCopyFailedEvent(m->file, m->src, m->dst, m->failure_cause));
            }
        } else if (auto m = dynamic_cast<ExecutionControllerAlarmTimerMessage*>(message.get())) {
            return std::shared_ptr<TimerEvent>(new TimerEvent(m->message));
        } else {
            throw std::runtime_error(
                    "ExecutionEvent::waitForNextExecutionEvent(): Non-handled message type when generating execution event (" +
                    message->getName() +")");
        }
    }

};
