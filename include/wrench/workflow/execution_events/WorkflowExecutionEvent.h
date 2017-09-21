/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_WORKFLOWEXECUTIONEVENT_H
#define WRENCH_WORKFLOWEXECUTIONEVENT_H


#include <string>
#include "FailureCause.h"

namespace wrench {


    class WorkflowTask;

    class WorkflowFile;

    class WorkflowJob;

    class ComputeService;

    class StorageService;

    class FailureCause;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A class to represent the various execution events that
     * are relevant to the execution of a workflow.
     */
    class WorkflowExecutionEvent {

    public:

        /** @brief Workflow execution event types */
        enum EventType {
            /** @brief An error type */
            UNDEFINED,
            /** @brief A job was submitted to a compute service that does not support its type */
            UNSUPPORTED_JOB_TYPE,
            /** @brief A standard job successfully completed */
            STANDARD_JOB_COMPLETION,
            /** @brief A standard job failed */
            STANDARD_JOB_FAILURE,
            /** @brief A pilot job started */
            PILOT_JOB_START,
            /** @brief A pilot job expired */
            PILOT_JOB_EXPIRATION,
            /** @brief A file copy operation completed */
            FILE_COPY_COMPLETION,
            /** @brief A file copy operation failed */
            FILE_COPY_FAILURE,
        };


        /** @brief The event type */
        WorkflowExecutionEvent::EventType type;
        /** @brief The relevant job, or nullptr */
        WorkflowJob *job = nullptr;
        /** @brief The relevant compute service, or nullptr */
        ComputeService *compute_service = nullptr;
        /** @brief The relevant file, or nullptr */
        WorkflowFile *file = nullptr;
        /** @brief The relevant storage service, or nullptr */
        StorageService *storage_service = nullptr;
        /** @brief The relevant failure cause, or nullptr */
        std::shared_ptr<FailureCause> failure_cause = nullptr;


    private:
        WorkflowExecutionEvent();

        friend class Workflow;

        static std::unique_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent(std::string);

    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_WORKFLOWEXECUTIONEVENT_H
