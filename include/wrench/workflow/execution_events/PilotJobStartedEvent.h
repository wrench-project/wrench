/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_PILOT_JOB_STARTED_EVENT_H
#define WRENCH_PILOT_JOB_STARTED_EVENT_H

#include <string>
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
     * @brief A "pilot job has started" WorkflowExecutionEvent
     */
    class PilotJobStartedEvent : public WorkflowExecutionEvent {

    private:

        friend class WorkflowExecutionEvent;

        /**
         * @brief Constructor
         * @param pilot_job: a pilot job
         * @param compute_service: a compute service
         */
        PilotJobStartedEvent(std::shared_ptr<PilotJob> pilot_job,
                             std::shared_ptr<ComputeService>  compute_service)
                : pilot_job(pilot_job), compute_service(compute_service) {}

    public:
        /** @brief The pilot job that has started */
        std::shared_ptr<PilotJob> pilot_job;
        /** @brief The compute service on which the pilot job has started */
        std::shared_ptr<ComputeService>  compute_service;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override { return "PilotJobStartedEvent (cs = " + this->compute_service->getName() + ")";}

    };

};

/***********************/
/** \endcond           */
/***********************/



#endif //WRENCH_PILOT_JOB_STARTED_EVENT_H
