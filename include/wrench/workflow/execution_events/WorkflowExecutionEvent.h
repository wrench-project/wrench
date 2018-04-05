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

    class StandardJob;
    class PilotJob;

    class ComputeService;
    class StorageService;
    class FileRegistryService;

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

        WorkflowExecutionEvent(EventType type) : type(type) {}

        friend class Workflow;

        static std::unique_ptr<WorkflowExecutionEvent> waitForNextExecutionEvent(std::string);

    };

    class StandardJobCompletionEvent : public WorkflowExecutionEvent {

    public:

        StandardJobCompletionEvent(StandardJob *standard_job,
                                   ComputeService *compute_service)
                : WorkflowExecutionEvent(STANDARD_JOB_COMPLETION),
                  standard_job(standard_job), compute_service(compute_service) {}

        StandardJob *standard_job;
        ComputeService *compute_service;
    };

    class StandardJobFailedEvent : public WorkflowExecutionEvent {

    public:

        StandardJobFailedEvent(StandardJob *standard_job,
                               ComputeService *compute_service,
                               std::shared_ptr<FailureCause> failure_cause)
                : WorkflowExecutionEvent(STANDARD_JOB_FAILURE),
                  standard_job(standard_job),
                  compute_service(compute_service),
                  failure_cause(failure_cause) {}

        StandardJob *standard_job;
        ComputeService *compute_service;
        std::shared_ptr<FailureCause> failure_cause;
    };


    class PilotJobStartedEvent : public WorkflowExecutionEvent {

    public:

        PilotJobStartedEvent(PilotJob *pilot_job,
                             ComputeService *compute_service)
                : WorkflowExecutionEvent(PILOT_JOB_START),
                  pilot_job(pilot_job), compute_service(compute_service) {}

        PilotJob *pilot_job;
        ComputeService *compute_service;
    };

    class PilotJobExpiredEvent : public WorkflowExecutionEvent {

    public:

        PilotJobExpiredEvent(PilotJob *pilot_job,
                             ComputeService *compute_service)
                : WorkflowExecutionEvent(PILOT_JOB_EXPIRATION),
                  pilot_job(pilot_job), compute_service(compute_service) {}

        PilotJob *pilot_job;
        ComputeService *compute_service;
    };

    class FileCopyCompletedEvent : public WorkflowExecutionEvent {

    public:

        FileCopyCompletedEvent(WorkflowFile *file,
                               StorageService *storage_service)
                : WorkflowExecutionEvent(FILE_COPY_COMPLETION),
                  file(file), storage_service(storage_service) {}

        WorkflowFile *file;
        StorageService *storage_service;
    };


    class FileCopyFailedEvent : public WorkflowExecutionEvent {

    public:

        FileCopyFailedEvent(WorkflowFile *file,
                            StorageService *storage_service,
                            FileRegistryService *file_registry_service,
                            bool file_registry_service_updated,
                            std::shared_ptr<FailureCause> failure_cause
        )
                : WorkflowExecutionEvent(FILE_COPY_COMPLETION),
                  file(file), storage_service(storage_service),
                  file_registry_service(file_registry_service),
                  file_registry_service_updated(file_registry_service_updated) {}

        WorkflowFile *file;
        StorageService *storage_service;
        FileRegistryService *file_registry_service;
        bool file_registry_service_updated;
        std::shared_ptr<FailureCause> failure_cause;

    };




/***********************/
/** \endcond           */
/***********************/

};


#endif //WRENCH_WORKFLOWEXECUTIONEVENT_H
