/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STANDARD_JOB_COMPLETED_EVENT_H
#define WRENCH_STANDARD_JOB_COMPLETED_EVENT_H

#include <string>
#include "wrench/workflow//execution_events/WorkflowExecutionEvent.h"
#include "wrench/workflow/failure_causes/FailureCause.h"

/***********************/
/** \cond DEVELOPER    */
/***********************/

namespace wrench {

    class WorkflowTask;

    class WorkflowFile;

    class StandardJob;

    class PilotJob;

    class ComputeService;

    class StorageService;

    class FileRegistryService;

    class FileRegistryService;



    /**
     * @brief A "standard job has completed" WorkflowExecutionEvent
     */
    class StandardJobCompletedEvent : public WorkflowExecutionEvent {

    private:

        friend class WorkflowExecutionEvent;

        /**
         * @brief Constructor
         * @param standard_job: a standard job
         * @param compute_service: a compute service
         */
        StandardJobCompletedEvent(StandardJob *standard_job,
                                  std::shared_ptr<ComputeService>  compute_service)
                : standard_job(standard_job), compute_service(compute_service) {}
    public:

        /** @brief The standard job that has completed */
        StandardJob *standard_job;
        /** @brief The compute service on which the standard job has completed */
        std::shared_ptr<ComputeService>  compute_service;

        /** 
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override { return "StandardJobCompletedEvent (job: " + this->standard_job->getName() + "; cs = " + this->compute_service->getName() + ")";}
    };


};

/***********************/
/** \endcond           */
/***********************/



#endif //WRENCH_STANDARD_JOB_COMPLETED_EVENT_H
