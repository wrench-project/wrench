/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_EXAMPLE_CRITICALPATHSCHEDULER_H
#define WRENCH_EXAMPLE_CRITICALPATHSCHEDULER_H

#include <gtest/gtest_prod.h>
#include <set>
#include <vector>

#include <wrench-dev.h>

namespace wrench {

    /**
     * @brief A critical path pilot job Scheduler
     */
    class CriticalPathPilotJobScheduler : public PilotJobScheduler {

    public:

        explicit CriticalPathPilotJobScheduler(std::shared_ptr<Workflow> workflow) : workflow(workflow) {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        void schedulePilotJobs(const std::set<std::shared_ptr<ComputeService>> &compute_services) override;

    protected:
        double getFlops(std::shared_ptr<Workflow> workflow, const std::vector<std::shared_ptr<WorkflowTask>> &);

        unsigned long getMaxParallelization(std::shared_ptr<Workflow> workflow, const std::set<std::shared_ptr<WorkflowTask>> &);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        std::shared_ptr<Workflow> workflow;

        std::map<std::shared_ptr<WorkflowTask>, double> flopsMap;

        int num_running_pilot_jobs = 0;

        FRIEND_TEST(CriticalPathSchedulerTest, GetTotalFlops);

        FRIEND_TEST(CriticalPathSchedulerTest, GetMaxParallelization);
    };

}


#endif //WRENCH_EXAMPLE_CRITICALPATHSCHEDULER_H