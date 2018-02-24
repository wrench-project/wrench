/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CRITICALPATHSCHEDULER_H
#define WRENCH_CRITICALPATHSCHEDULER_H

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

        CriticalPathPilotJobScheduler(Workflow *workflow) : workflow(workflow) {}

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        void schedulePilotJobs(const std::set<ComputeService *> &compute_services) override;

    protected:
        double getFlops(Workflow *, const std::vector<WorkflowTask *> &);

        unsigned long getMaxParallelization(Workflow *, const std::set<WorkflowTask *> &);
        /***********************/
        /** \endcond           */
        /***********************/

    private:
        Workflow *workflow;

        std::map<WorkflowTask *, double> flopsMap;

        FRIEND_TEST(CriticalPathSchedulerTest, GetTotalFlops);

        FRIEND_TEST(CriticalPathSchedulerTest, GetMaxParallelization);
    };

}


#endif //WRENCH_CRITICALPATHSCHEDULER_H
