/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MINMINSCHEDULER_H
#define WRENCH_MINMINSCHEDULER_H

#include <wrench-dev.h>

namespace wrench {


    /**
      * @brief A min-min Scheduler
      */
    class MinMinStandardJobScheduler : public StandardJobScheduler {


    public:

        MinMinStandardJobScheduler(JobManager *job_manager) : job_manager(job_manager) {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        virtual void scheduleTasks(const std::set<ComputeService *> &compute_services,
                           const std::map<std::string, std::vector<WorkflowTask *>> &ready_tasks
        ) override;

    private:
        struct MinMinComparator {
            bool operator()(std::pair<std::string, std::vector<WorkflowTask *>> &lhs,
                            std::pair<std::string, std::vector<WorkflowTask *>> &rhs);
        };

        JobManager *job_manager;

        /***********************/
        /** \endcond           */
        /***********************/
    };


}

#endif //WRENCH_MINMINSCHEDULER_H
