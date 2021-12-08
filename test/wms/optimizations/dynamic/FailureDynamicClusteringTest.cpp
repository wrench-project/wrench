/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include "workflow/Workflow.h"
#include "wms/optimizations/dynamic/FailureDynamicClustering.h"


class FailureDynamicClusteringTest : public ::testing::Test {
protected:
    FailureDynamicClusteringTest() {
      workflow = new wrench::Workflow();

      // create simple diamond workflow
      t1 = workflow->addTask("task-test-01", 1);
      t2 = workflow->addTask("task-test-02", 1);
      t3 = workflow->addTask("task-test-03", 1);
      t4 = workflow->addTask("task-test-04", 1);

      t2->setClusterID("cluster-01");
      t3->setClusterID("cluster-01");

      workflow->addControlDependency(t1, t2);
      workflow->addControlDependency(t1, t3);
      workflow->addControlDependency(t2, t4);
      workflow->addControlDependency(t3, t4);
    }

    // data members
    wrench::Workflow *workflow;
    std::shared_ptr<wrench::WorkflowTask> t1, *t2, *t3, *t4;
};

TEST_F(FailureDynamicClusteringTest, UngroupFailedTasks) {
  wrench::DynamicOptimization *opt = new wrench::FailureDynamicClustering();

  opt->process(workflow);

  // initial task state
  ASSERT_EQ(workflow->getReadyTasks().size(), 1);

  t1->setRunning();
  t1->setCompleted();

  // set t2 as failed task
  t2->incrementFailureCount();

  opt->process(workflow);
  ASSERT_EQ(workflow->getReadyTasks().size(), 2);
}
