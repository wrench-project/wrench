/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Workflow.getNextEvent() method.
 */


#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
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
     *        based on messages received on a commport, or until a timeout occurs
     *
     * @param commport: the name of the receiving commport
     * @param timeout: a timeout value in seconds (-1 means: no timeout)
     * @return a workflow execution event (or nullptr in case of a timeout)
     *
     */
    std::shared_ptr<ExecutionEvent>
    ExecutionEvent::waitForNextExecutionEvent(S4U_CommPort *commport, double timeout) {
        // Get the message from the commport
        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = commport->getMessage<SimulationMessage>(timeout);
        } catch (ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<NetworkError>(e.getCause());
            if (cause->isTimeout()) {
                return nullptr;
            } else {
                throw;
            }
        }

        if (auto jmcjcm = std::dynamic_pointer_cast<JobManagerCompoundJobCompletedMessage>(message)) {
            return std::shared_ptr<CompoundJobCompletedEvent>(
                    new CompoundJobCompletedEvent(jmcjcm->job, jmcjcm->compute_service));

        } else if (auto jmcjfm = std::dynamic_pointer_cast<JobManagerCompoundJobFailedMessage>(message)) {
            return std::shared_ptr<CompoundJobFailedEvent>(
                    new CompoundJobFailedEvent(jmcjfm->job, jmcjfm->compute_service, jmcjfm->cause));

        } else if (auto jmsjcm = std::dynamic_pointer_cast<JobManagerStandardJobCompletedMessage>(message)) {
            std::set<std::shared_ptr<WorkflowTask>> failure_count_increments;
            jmsjcm->job->applyTaskUpdates(jmsjcm->necessary_state_changes, failure_count_increments);
            return std::shared_ptr<StandardJobCompletedEvent>(
                    new StandardJobCompletedEvent(jmsjcm->job, jmsjcm->compute_service));

        } else if (auto jmsjfm = std::dynamic_pointer_cast<JobManagerStandardJobFailedMessage>(message)) {
            jmsjfm->job->applyTaskUpdates(jmsjfm->necessary_state_changes, jmsjfm->necessary_failure_count_increments);
            return std::shared_ptr<StandardJobFailedEvent>(
                    new StandardJobFailedEvent(jmsjfm->job, jmsjfm->compute_service, jmsjfm->cause));

        } else if (auto cspjsm = std::dynamic_pointer_cast<ComputeServicePilotJobStartedMessage>(message)) {
            return std::shared_ptr<PilotJobStartedEvent>(new PilotJobStartedEvent(cspjsm->job, cspjsm->compute_service));

        } else if (auto cspjem = std::dynamic_pointer_cast<ComputeServicePilotJobExpiredMessage>(message)) {
            return std::shared_ptr<PilotJobExpiredEvent>(new PilotJobExpiredEvent(cspjem->job, cspjem->compute_service));

        } else if (auto dmfcam = std::dynamic_pointer_cast<DataManagerFileCopyAnswerMessage>(message)) {
            if (dmfcam->success) {
                return std::shared_ptr<FileCopyCompletedEvent>(new FileCopyCompletedEvent(
                        dmfcam->src_location, dmfcam->dst_location));

            } else {
                return std::shared_ptr<FileCopyFailedEvent>(
                        new FileCopyFailedEvent(dmfcam->src_location, dmfcam->dst_location, dmfcam->failure_cause));
            }
        } else if (auto dmfram = std::dynamic_pointer_cast<DataManagerFileReadAnswerMessage>(message)) {
            if (dmfram->success) {
                return std::shared_ptr<FileReadCompletedEvent>(new FileReadCompletedEvent(
                        dmfram->location, dmfram->num_bytes));

            } else {
                return std::shared_ptr<FileReadFailedEvent>(
                        new FileReadFailedEvent(dmfram->location, dmfram->num_bytes, dmfram->failure_cause));
            }
        } else if (auto dmfwam = std::dynamic_pointer_cast<DataManagerFileWriteAnswerMessage>(message)) {
            if (dmfwam->success) {
                return std::shared_ptr<FileWriteCompletedEvent>(new FileWriteCompletedEvent(
                        dmfwam->location));

            } else {
                return std::shared_ptr<FileWriteFailedEvent>(
                        new FileWriteFailedEvent(dmfwam->location, dmfwam->failure_cause));
            }

        } else if (auto ecatm = std::dynamic_pointer_cast<ExecutionControllerAlarmTimerMessage>(message)) {
            return std::shared_ptr<TimerEvent>(new TimerEvent(ecatm->message));
        } else {
            throw std::runtime_error(
                    "ExecutionEvent::waitForNextExecutionEvent(): Non-handled message type when generating execution event (" +
                    message->getName() + ")");
        }
    }

}// namespace wrench
