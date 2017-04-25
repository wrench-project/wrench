/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MAXMINSCHEDULER_H
#define WRENCH_MAXMINSCHEDULER_H

#include "wms/scheduler/Scheduler.h"

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A max-min Scheduler class
     */
    class MaxMinScheduler : public Scheduler {

    public:
        void scheduleTasks(JobManager *job_manager,
                           std::vector<WorkflowTask *> ready_tasks,
                           const std::set<ComputeService *> &compute_services);

        void schedulePilotJobs(JobManager *job_manager,
                               Workflow *workflow,
                               double flops,
                               const std::set<ComputeService *> &compute_services);

        struct MaxMinComparator {
            bool operator()(WorkflowTask *&lhs, WorkflowTask *&rhs);
        };

    };

    /***********************/
    /** \endcond           */
    /***********************/

}

#endif //WRENCH_MAXMINSCHEDULER_H
