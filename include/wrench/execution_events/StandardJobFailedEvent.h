/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STANDARD_JOB_FAILED_H
#define WRENCH_STANDARD_JOB_FAILED_H

#include <string>
#include "wrench/failure_causes/FailureCause.h"

/***********************/
/** \cond DEVELOPER    */
/***********************/

namespace wrench {

    class WorkflowTask;

    class DataFile;

    class StandardJob;

    class PilotJob;

    class ComputeService;

    class StorageService;

    class FileRegistryService;

    class FileRegistryService;

    /**
     * @brief A "standard job has failed" ExecutionEvent
     */
    class StandardJobFailedEvent : public ExecutionEvent {

    private:

        friend class ExecutionEvent;

        /**
         * @brief Constructor
         * @param standard_job: a standard job
         * @param compute_service: a compute service
         * @param failure_cause: a failure_cause
         */
        StandardJobFailedEvent(std::shared_ptr<StandardJob> standard_job,
                               std::shared_ptr<ComputeService>  compute_service,
                               std::shared_ptr<FailureCause> failure_cause)
                : standard_job(standard_job),
                  compute_service(compute_service),
                  failure_cause(failure_cause) {}

    public:

        /** @brief The standard job that has failed */
        std::shared_ptr<StandardJob> standard_job;
        /** @brief The compute service on which the job has failed */
        std::shared_ptr<ComputeService>  compute_service;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> failure_cause;

        /** 
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override { return "StandardJobFailedEvent (job: " + this->standard_job->getName() + "; cs = " +
                                                 this->compute_service->getName() + "; cause: " + this->failure_cause->toString() + ")";}

    };


};

/***********************/
/** \endcond           */
/***********************/



#endif //WRENCH_STANDARD_JOB_FAILED_H
