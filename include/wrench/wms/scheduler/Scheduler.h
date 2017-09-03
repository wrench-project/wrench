/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SCHEDULER_H
#define WRENCH_SCHEDULER_H

#include <set>

#include "wrench/managers/JobManager.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/workflow/WorkflowTask.h"

namespace wrench {

    class JobManager;

    /**
     * @brief A workflow scheduler
     */
    class Scheduler {

    public:
        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        virtual void scheduleTasks(JobManager *job_manager,
                                   std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                                   const std::set<ComputeService *> &compute_services) = 0;

        virtual void schedulePilotJobs(JobManager *job_manager,
                                       Workflow *workflow,
                                       double pilot_job_duration,
                                       const std::set<ComputeService *> &compute_services);

        static double getTotalFlops(std::vector<WorkflowTask *> tasks);
        /***********************/
        /** \endcond           */
        /***********************/
    };

}

#endif //WRENCH_SCHEDULER_H
