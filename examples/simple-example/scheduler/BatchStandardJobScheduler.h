/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_BATCHSCHEDULER_H
#define WRENCH_BATCHSCHEDULER_H


#include <wrench-dev.h>

namespace wrench {

    /**
     * @brief A batch Scheduler
     */
    class BatchStandardJobScheduler : public StandardJobScheduler {

    public:

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(const std::set<ComputeService *> &compute_services,
                           const std::vector<WorkflowTask *> &tasks);


        /***********************/
        /** \endcond           */
        /***********************/

    };
}

#endif //WRENCH_BATCHSCHEDULER_H
