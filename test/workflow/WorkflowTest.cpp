/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "gtest/gtest.h"
#include "workflow/Workflow.h"

TEST(Workflow__Test, WorkflowStructure) {
  wrench::Workflow *workflow = new wrench::Workflow();
  EXPECT_EQ(0, workflow->getNumberOfTasks());

  // create simple diamond workflow
  wrench::WorkflowTask *t1 = workflow->addTask("task-test-01", 1);
  wrench::WorkflowTask *t2 = workflow->addTask("task-test-02", 1);
  wrench::WorkflowTask *t3 = workflow->addTask("task-test-03", 1);
  wrench::WorkflowTask *t4 = workflow->addTask("task-test-04", 1);
  EXPECT_EQ(4, workflow->getNumberOfTasks());

  workflow->addControlDependency(t1, t2);
  workflow->addControlDependency(t1, t3);
  workflow->addControlDependency(t2, t4);
  workflow->addControlDependency(t3, t4);

  // test task's parents
  EXPECT_EQ(0, workflow->getTaskParents(t1).size());
  EXPECT_EQ(1, workflow->getTaskParents(t2).size());
  EXPECT_EQ(1, workflow->getTaskParents(t3).size());
  EXPECT_EQ(2, workflow->getTaskParents(t4).size());

  // test task's children
  EXPECT_EQ(2, workflow->getTaskChildren(t1).size());
  EXPECT_EQ(1, workflow->getTaskChildren(t2).size());
  EXPECT_EQ(1, workflow->getTaskChildren(t3).size());
  EXPECT_EQ(0, workflow->getTaskChildren(t4).size());

  // remove tasks
  workflow->removeTask(t4);
  EXPECT_EQ(0, workflow->getTaskChildren(t3).size());
  EXPECT_EQ(0, workflow->getTaskChildren(t2).size());

  EXPECT_EQ(3, workflow->getTasks().size());

  workflow->removeTask(t1);
}

TEST(Workflow__Test, ControlDependency) {
  wrench::Workflow *workflow = new wrench::Workflow();
  wrench::WorkflowTask *t1 = workflow->addTask("task-test-01", 1);

  // test null control dependencies
  EXPECT_THROW(workflow->addControlDependency(nullptr, nullptr), std::invalid_argument);
  EXPECT_THROW(workflow->addControlDependency(t1, nullptr), std::invalid_argument);
  EXPECT_THROW(workflow->addControlDependency(nullptr, t1), std::invalid_argument);
}

TEST(Workflow__Test, WorkflowTask) {
  wrench::Workflow *workflow = new wrench::Workflow();

  wrench::WorkflowTask *t1 = workflow->addTask("task-test-01", 1);
  wrench::WorkflowTask *t2 = workflow->addTask("task-test-02", 1);
  wrench::WorkflowTask *t3 = workflow->addTask("task-test-03", 1);

  EXPECT_THROW(workflow->getWorkflowTaskByID("task-test-00"), std::invalid_argument);
  EXPECT_TRUE(workflow->getWorkflowTaskByID("task-test-01")->getId() == t1->getId());
}