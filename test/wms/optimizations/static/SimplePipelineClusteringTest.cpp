/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include <wrench/workflow/WorkflowTask.h>
#include <workflow/Workflow.h>

#include "wms/optimizations/static/SimplePipelineClustering.h"

class SimplePipelineClusteringTest : public ::testing::Test {
protected:
    SimplePipelineClusteringTest() {
      workflow = wrench::Workflow::createWorkflow();

      // create simple diamond workflow
      t1 = workflow->addTask("task-test-01", 1);
      t2 = workflow->addTask("task-test-02", 10);
      t3 = workflow->addTask("task-test-03", 100);
      t4 = workflow->addTask("task-test-04", 1000);
      t5 = workflow->addTask("task-test-05", 10000);

      workflow->addControlDependency(t1, t2);
      workflow->addControlDependency(t2, t3);
      workflow->addControlDependency(t3, t4);
      workflow->addControlDependency(t3, t5);
    }

    // data members
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::WorkflowTask> t1, *t2, *t3, *t4, *t5;
};

TEST_F(SimplePipelineClusteringTest, GroupTasks) {
  wrench::StaticOptimization *opt = new wrench::SimplePipelineClustering();

  ASSERT_EQ(workflow->getNumberOfTasks(), 5);

  opt->process(workflow);

  std::map<std::string, std::vector<std::shared_ptr<wrench::WorkflowTask> >> map = workflow->getReadyTasks();
  ASSERT_EQ(map.size(), 1);

  ASSERT_EQ(map.begin()->second.size(), 3);
  ASSERT_EQ(map["PIPELINE_CLUSTER_1"].size(), 3);
}
