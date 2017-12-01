/**
 * Copyright (c) 2017. The WRENCH Team.
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

    class Simulation;

    /**
     * @brief A batch Scheduler
     */
    class BatchScheduler : public Scheduler {

    public:
        BatchScheduler(ComputeService *cloud_service, Simulation *simulation);

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(JobManager *job_manager,
                           std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                           const std::set<ComputeService *> &compute_services) override;

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        ComputeService *batch_service;
        Simulation *simulation;
    };
}

#endif //WRENCH_BATCHSCHEDULER_H
