/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <xbt.h>

#include "wms/optimizations/dynamic/SimpleDynamicClusteringForFailures.h"

class SimpleDynamicClusteringForFailures : public ::testing::Test {
protected:
    SimpleDynamicClusteringForFailures() {
      workflow = new wrench::Workflow();

      // create simple diamond workflow
      t1 = workflow->addTask("task-test-01", 1);
      t2 = workflow->addTask("task-test-02", 1);
      t3 = workflow->addTask("task-test-03", 1);
      t4 = workflow->addTask("task-test-04", 1);

      t2->setClusterId("cluster-01");
      t3->setClusterId("cluster-01");

      workflow->addControlDependency(t1, t2);
      workflow->addControlDependency(t1, t3);
      workflow->addControlDependency(t2, t4);
      workflow->addControlDependency(t3, t4);
    }

    // data members
    wrench::Workflow *workflow;
    wrench::WorkflowTask *t1, *t2, *t3, *t4;
};

TEST_F(SimpleDynamicClusteringForFailures, UngroupFailedTasks) {
  wrench::DynamicOptimization *opt = new wrench::SimpleDynamicClusteringForFailures();

  opt->process(workflow);

  // initial task state
  EXPECT_EQ(workflow->getReadyTasks().size(), 1);

  t1->setRunning();
  t1->setCompleted();

  // set t2 as failed task
  t2->incrementFailureCount();

  opt->process(workflow);
  EXPECT_EQ(workflow->getReadyTasks().size(), 2);
}
