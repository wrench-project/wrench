/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/job_manager/JobManagerMessage.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     */
    JobManagerMessage::JobManagerMessage() : SimulationMessage(0) {
    }


    /**
     * @brief Constructor 
     * @param job: the job that is done
     * @param compute_service: the ComputeService on which it ran 
     * @param necessary_state_changes: necessary task state changes
     */
    JobManagerStandardJobCompletedMessage::JobManagerStandardJobCompletedMessage(std::shared_ptr<StandardJob> job,
                                                                                 std::shared_ptr<ComputeService> compute_service,
                                                                                 std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> necessary_state_changes)
        : JobManagerMessage() {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (compute_service == nullptr)) {
            throw std::invalid_argument("JobManagerStandardJobCompletedMessage::JobManagerStandardJobCompletedMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(compute_service);
        this->necessary_state_changes = std::move(necessary_state_changes);
    }

    /**
     * @brief Constructor 
     * @param job: the job that has failed
     * @param compute_service: the ComputeService on which it ran 
     * @param necessary_state_changes: necessary task state changes
     * @param necessary_failure_count_increments: necessary task failure count increments
     * @param cause: the cause of the failure
     */
    JobManagerStandardJobFailedMessage::JobManagerStandardJobFailedMessage(std::shared_ptr<StandardJob> job,
                                                                           std::shared_ptr<ComputeService> compute_service,
                                                                           std::map<std::shared_ptr<WorkflowTask>, WorkflowTask::State> necessary_state_changes,
                                                                           std::set<std::shared_ptr<WorkflowTask>> necessary_failure_count_increments,
                                                                           std::shared_ptr<FailureCause> cause) : JobManagerMessage() {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (compute_service == nullptr) || (cause == nullptr)) {
            throw std::invalid_argument("JobManagerStandardJobFailedMessage::JobManagerStandardJobFailedMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(compute_service);
        this->necessary_state_changes = std::move(necessary_state_changes);
        this->necessary_failure_count_increments = std::move(necessary_failure_count_increments);
        this->cause = std::move(cause);
    }

    /**
     * @brief Message from by job manager to notify somebody of a standard job successfully completed
     *
     * @param job: the job
     * @param compute_service: the compute service that did the job
     */
    JobManagerCompoundJobCompletedMessage::JobManagerCompoundJobCompletedMessage(std::shared_ptr<CompoundJob> job,
                                                                                 std::shared_ptr<ComputeService> compute_service) : JobManagerMessage() {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (compute_service == nullptr)) {
            throw std::invalid_argument("JobManagerCompoundJobCompletedMessage::JobManagerCompoundJobCompletedMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(compute_service);
    }

    /**
     * @brief Message from by job manager to notify somebody of a standard job has failed to complete
     *
     * @param job: the job
     * @param compute_service: the compute service that did the job
     * @param cause: the failure cause
     */
    JobManagerCompoundJobFailedMessage::JobManagerCompoundJobFailedMessage(std::shared_ptr<CompoundJob> job,
                                                                           std::shared_ptr<ComputeService> compute_service,
                                                                           std::shared_ptr<FailureCause> cause) : JobManagerMessage() {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((job == nullptr) || (compute_service == nullptr)) {
            throw std::invalid_argument("JobManagerCompoundJobFailedMessage::JobManagerCompoundJobFailedMessage(): Invalid arguments");
        }
#endif
        this->job = std::move(job);
        this->compute_service = std::move(compute_service);
        this->cause = std::move(cause);
    }

    /**
     * @brief Message sent to the job manager to wake it up
     */
    JobManagerWakeupMessage::JobManagerWakeupMessage() : JobManagerMessage() {
    }
}// namespace wrench
