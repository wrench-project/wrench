/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include "wms/scheduler/pilot_job/CriticalPathScheduler.h"

namespace wrench {
    class CriticalPathSchedulerTest : public ::testing::Test {

    protected:
        CriticalPathSchedulerTest() {
            workflow = new Workflow();

            // create simple diamond workflow
            t1 = workflow->addTask("task-test-01", 100);
            t2 = workflow->addTask("task-test-02", 200);
            t3 = workflow->addTask("task-test-03", 100);
            t4 = workflow->addTask("task-test-04", 200);
            t5 = workflow->addTask("task-test-05", 100);
            t6 = workflow->addTask("task-test-06", 100);

            workflow->addControlDependency(t1, t2);
            workflow->addControlDependency(t1, t3);
            workflow->addControlDependency(t2, t4);
            workflow->addControlDependency(t3, t5);
            workflow->addControlDependency(t4, t6);
            workflow->addControlDependency(t5, t6);
        }

        // data members
        Workflow *workflow;
        std::shared_ptr<WorkflowTask> t1, *t2, *t3, *t4, *t5, *t6;
    };

    TEST_F(CriticalPathSchedulerTest, GetTotalFlops) {
        std::unique_ptr<CriticalPathScheduler> scheduler(new CriticalPathScheduler());

        std::vector<std::shared_ptr<WorkflowTask>> tasks;
        tasks.push_back(t1);
        tasks.push_back(t2);
        tasks.push_back(t3);
        tasks.push_back(t4);
        tasks.push_back(t5);
        tasks.push_back(t6);

        ASSERT_EQ(600, scheduler->getFlops(this->workflow, tasks));

        tasks.erase(std::remove(tasks.begin(), tasks.end(), t1), tasks.end());
        ASSERT_EQ(500, scheduler->getFlops(this->workflow, tasks));

        tasks.erase(std::remove(tasks.begin(), tasks.end(), t2), tasks.end());
        ASSERT_EQ(300, scheduler->getFlops(this->workflow, tasks));

        tasks.erase(std::remove(tasks.begin(), tasks.end(), t4), tasks.end());
        ASSERT_EQ(300, scheduler->getFlops(this->workflow, tasks));
    }

    TEST_F(CriticalPathSchedulerTest, GetMaxParallelization) {
        std::unique_ptr<CriticalPathScheduler> scheduler(new CriticalPathScheduler());

        std::set<std::shared_ptr<WorkflowTask>> tasks = {t1};

        ASSERT_EQ(2, scheduler->getMaxParallelization(this->workflow, tasks));

        tasks = {t2, t3};
        ASSERT_EQ(2, scheduler->getMaxParallelization(this->workflow, tasks));

        tasks = {t2};
        ASSERT_EQ(1, scheduler->getMaxParallelization(this->workflow, tasks));

        tasks = {t3};
        ASSERT_EQ(1, scheduler->getMaxParallelization(this->workflow, tasks));
    }
}// namespace wrench
