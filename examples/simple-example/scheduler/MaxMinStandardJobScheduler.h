/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MAXMINSCHEDULER_H
#define WRENCH_MAXMINSCHEDULER_H

#include <wrench-dev.h>
#include <map>

namespace wrench {


    /**
     * @brief A max-min Scheduler
     */
    class MaxMinStandardJobScheduler : public StandardJobScheduler {


    public:

        MaxMinStandardJobScheduler(JobManager *job_manager) : job_manager(job_manager) {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(const std::set<ComputeService *> &compute_services,
                           const std::vector<WorkflowTask *> &tasks);

    private:
        struct MaxMinComparator {
            bool operator()(WorkflowTask *&lhs, WorkflowTask *&rhs);
        };

        JobManager *job_manager;
        /***********************/
        /** \endcond           */
        /***********************/
    };


}

#endif //WRENCH_MAXMINSCHEDULER_H
