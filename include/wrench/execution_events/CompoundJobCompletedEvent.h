/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_COMPOUND_JOB_COMPLETED_EVENT_H
#define WRENCH_COMPOUND_JOB_COMPLETED_EVENT_H

#include <string>
#include <utility>
#include "ExecutionEvent.h"
#include "wrench/failure_causes/FailureCause.h"

/***********************/
/** \cond DEVELOPER    */
/***********************/

namespace wrench {

    class WorkflowTask;

    class WorkflowFile;

    class CompoundJob;

    class PilotJob;

    class ComputeService;

    class StorageService;

    class FileRegistryService;

    class FileRegistryService;



    /**
     * @brief A "compound job has completed" ExecutionEvent
     */
    class CompoundJobCompletedEvent : public ExecutionEvent {

    private:

        friend class ExecutionEvent;

        /**
         * @brief Constructor
         * @param compound_job: a compound job
         * @param compute_service: a compute service
         */
        CompoundJobCompletedEvent(std::shared_ptr<CompoundJob> compound_job,
                                  std::shared_ptr<ComputeService>  compute_service)
                : job(std::move(compound_job)), compute_service(std::move(compute_service)) {}
    public:

        /** @brief The compound job that has completed */
        std::shared_ptr<CompoundJob> job;
        /** @brief The compute service on which the compound job has completed */
        std::shared_ptr<ComputeService>  compute_service;

        /** 
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override { return "CompoundJobCompletedEvent (job: " + this->job->getName() + "; cs = " + this->compute_service->getName() + ")";}
    };


};

/***********************/
/** \endcond           */
/***********************/



#endif //WRENCH_COMPOUND_JOB_COMPLETED_EVENT_H
