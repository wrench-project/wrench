/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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

        RandomStandardJobScheduler(JobManager *job_manager,
                                   StorageService *default_storage_service) : job_manager(job_manager),
                                                                              default_storage_service(
                                                                                      default_storage_service) {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(const std::set<ComputeService *> &compute_services,
                           const std::vector<WorkflowTask *> &tasks) override;

    private:
        JobManager *job_manager;
        StorageService *default_storage_service;

        /***********************/
        /** \endcond           */
        /***********************/
    };


};

#endif //WRENCH_RANDOMSCHEDULER_H
