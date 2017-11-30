/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_CLOUDSCHEDULER_H
#define WRENCH_CLOUDSCHEDULER_H

#include <wrench-dev.h>

namespace wrench {

    class Simulation;

    /**
     * @brief A cloud Scheduler
     */
    class CloudScheduler : public Scheduler {

    public:
        CloudScheduler(CloudService *cloud_service, Simulation *simulation);

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(JobManager *job_manager, std::map<std::string, std::vector<WorkflowTask *>> ready_tasks,
                           const std::set<ComputeService *> &compute_services) override;

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::string choosePMHostname();

        CloudService *cloud_service;
        std::vector<std::string> execution_hosts;
        std::map<std::string, std::vector<std::string>> vm_list;
        Simulation *simulation;
    };
}

#endif //WRENCH_CLOUDSCHEDULER_H
