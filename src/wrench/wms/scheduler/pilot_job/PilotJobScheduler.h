/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_PILOTJOBSCHEDULER_H
#define WRENCH_PILOTJOBSCHEDULER_H

#include <set>

#include "managers/job_manager/JobManager.h"
#include "wms/scheduler/Scheduler.h"
#include "workflow/Workflow.h"

namespace wrench {

    /**
     * @brief A pilot job scheduler
     */
    class PilotJobScheduler {

    public:
        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        virtual void schedule(Scheduler *, Workflow *, JobManager *, const std::set<ComputeService *> &) = 0;

        /***********************/
        /** \endcond           */
        /***********************/
    };
}

#endif //WRENCH_PILOTJOBSCHEDULER_H
