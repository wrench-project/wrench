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

#include <wrench-dev.h>

namespace wrench {


    /**
     * @brief A max-min Scheduler
     */
    class MaxMinScheduler : public Scheduler {


    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(JobManager *job_manager,
                           std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                           const std::set<ComputeService *> &compute_services);

    private:
        struct MaxMinComparator {
            bool operator()(std::pair<std::string, std::vector<WorkflowTask *>> &lhs,
                            std::pair<std::string, std::vector<WorkflowTask *>> &rhs);
        };
    /***********************/
    /** \endcond           */
    /***********************/
    };


}

#endif //WRENCH_MAXMINSCHEDULER_H
