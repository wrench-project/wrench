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

namespace wrench {


    class WorkflowTask;

    class WorkflowFile;

    class WorkflowJob;

    class ComputeService;

    class StorageService;

    class WorkflowExecutionFailureCause;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A class to represent the various execution events that
     * are relevant to the execution of a Workflow.
     */
    class WorkflowExecutionEvent {

    public:

        /** @brief Workflow execution event types */
        enum EventType {
            UNDEFINED,
            UNSUPPORTED_JOB_TYPE,
            STANDARD_JOB_COMPLETION,
            STANDARD_JOB_FAILURE,
            PILOT_JOB_START,
            PILOT_JOB_EXPIRATION,
            FILE_COPY_COMPLETION,
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
        WorkflowExecutionFailureCause *failure_cause = nullptr;

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
