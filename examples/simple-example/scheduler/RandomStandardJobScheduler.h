/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_RANDOMSCHEDULER_H
#define WRENCH_RANDOMSCHEDULER_H

#include <wrench-dev.h>

namespace wrench {


    /**
     * @brief A random Scheduler
     */
    class RandomStandardJobScheduler : public StandardJobScheduler {

    public:

        RandomStandardJobScheduler(JobManager *job_manager) : job_manager(job_manager) {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(const std::set<ComputeService *> &compute_services,
                           const std::map<std::string, std::vector<WorkflowTask *>> &tasks);

    private:
        JobManager *job_manager;

        /***********************/
        /** \endcond           */
        /***********************/
    };


};

#endif //WRENCH_RANDOMSCHEDULER_H
