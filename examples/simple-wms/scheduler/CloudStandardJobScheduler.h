/**
 * Copyright (c) 2017-2018. The WRENCH Team.
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
    class CloudStandardJobScheduler : public StandardJobScheduler {


    public:
        CloudStandardJobScheduler() {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        void scheduleTasks(const std::set<wrench::ComputeService *> &compute_services,
                           const std::map<std::string, std::vector<wrench::WorkflowTask *>> &tasks) override;

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::string choosePMHostname();

        std::vector<std::string> execution_hosts;
        std::map<std::string, std::vector<std::string>> vm_list;
        Simulation *simulation;
    };
}

#endif //WRENCH_CLOUDSCHEDULER_H
