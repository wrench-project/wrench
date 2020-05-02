/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_EXAMPLE_BATCHSTANDARDJOBSCHEDULER_H
#define WRENCH_EXAMPLE_BATCHSTANDARDJOBSCHEDULER_H

#include <wrench-dev.h>

namespace wrench {

    /**
     * @brief A batch Scheduler
     */
    class BatchStandardJobScheduler : public StandardJobScheduler {

    public:

        explicit BatchStandardJobScheduler(std::shared_ptr<StorageService> default_storage_service) :
                default_storage_service(default_storage_service) {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(const std::set<std::shared_ptr<ComputeService>> &compute_services,
                           const std::vector<WorkflowTask *> &tasks) override;

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::shared_ptr<StorageService> default_storage_service;

    };
}

#endif //WRENCH_EXAMPLE_BATCHSTANDARDJOBSCHEDULER_H
