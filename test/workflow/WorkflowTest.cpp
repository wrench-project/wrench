/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include "wrench/workflow/Workflow.h"

class WorkflowTest : public ::testing::Test {
protected:
    WorkflowTest() {
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

      f1 = workflow->addFile("file-01", 1);
      f2 = workflow->addFile("file-02", 1);
      f3 = workflow->addFile("file-03", 1);
      f4 = workflow->addFile("file-04", 1);

      t1->addInputFile(f1);
      t1->addOutputFile(f2);
      t2->addInputFile(f2);
      t2->addOutputFile(f3);
      t3->addInputFile(f2);
      t4->addInputFile(f3);
      t4->addInputFile(f4);
      t3->addOutputFile(f4);
    }

    // data members
    wrench::Workflow *workflow;
    wrench::WorkflowTask *t1, *t2, *t3, *t4;
    wrench::WorkflowFile *f1, *f2, *f3, *f4;
};

TEST_F(WorkflowTest, WorkflowStructure) {
  ASSERT_EQ(4, workflow->getNumberOfTasks());

  // testing number of task's parents
  EXPECT_EQ(0, workflow->getTaskParents(t1).size());
  EXPECT_EQ(1, workflow->getTaskParents(t2).size());
  EXPECT_EQ(1, workflow->getTaskParents(t3).size());
  EXPECT_EQ(2, workflow->getTaskParents(t4).size());

  // testing number of task's children
  EXPECT_EQ(2, workflow->getTaskChildren(t1).size());
  EXPECT_EQ(1, workflow->getTaskChildren(t2).size());
  EXPECT_EQ(1, workflow->getTaskChildren(t3).size());
  EXPECT_EQ(0, workflow->getTaskChildren(t4).size());

  // testing top-levels
  EXPECT_EQ(0, t1->getTopLevel());
  EXPECT_EQ(1, t2->getTopLevel());
  EXPECT_EQ(1, t3->getTopLevel());
  EXPECT_EQ(2, t4->getTopLevel());

  // remove tasks
  workflow->removeTask(t4);
  EXPECT_EQ(0, workflow->getTaskChildren(t3).size());
  EXPECT_EQ(0, workflow->getTaskChildren(t2).size());

  EXPECT_EQ(3, workflow->getTasks().size());

  workflow->removeTask(t1);
}

TEST_F(WorkflowTest, ControlDependency) {
  // testing null control dependencies
  EXPECT_THROW(workflow->addControlDependency(nullptr, nullptr), std::invalid_argument);
  EXPECT_THROW(workflow->addControlDependency(t1, nullptr), std::invalid_argument);
  EXPECT_THROW(workflow->addControlDependency(nullptr, t1), std::invalid_argument);
}

TEST_F(WorkflowTest, WorkflowTaskThrow) {
  // testing invalid task creation
  EXPECT_THROW(workflow->addTask("task-error", -100), std::invalid_argument);
  EXPECT_THROW(workflow->addTask("task-error", 100, -4), std::invalid_argument);

  // testing whether a task id exists
  EXPECT_THROW(workflow->getWorkflowTaskByID("task-test-00"), std::invalid_argument);
  EXPECT_TRUE(workflow->getWorkflowTaskByID("task-test-01")->getId() == t1->getId());

  // testing whether a task already exists (check via task id)
  EXPECT_THROW(workflow->addTask("task-test-01", 1), std::invalid_argument);
  EXPECT_THROW(workflow->addTask("task-test-01", 1, 1), std::invalid_argument);
  EXPECT_THROW(workflow->addTask("task-test-01", 1, 10), std::invalid_argument);
  EXPECT_THROW(workflow->addTask("task-test-01", 10000, 1), std::invalid_argument);

  // remove tasks
  EXPECT_THROW(workflow->removeTask(nullptr), std::invalid_argument);
  workflow->removeTask(t1);

  EXPECT_THROW(workflow->getTaskChildren(nullptr), std::invalid_argument);
  EXPECT_THROW(workflow->getTaskParents(nullptr), std::invalid_argument);

  EXPECT_THROW(workflow->updateTaskState(nullptr, wrench::WorkflowTask::State::FAILED), std::invalid_argument);
}

TEST_F(WorkflowTest, WorkflowFile) {
  EXPECT_THROW(workflow->addFile("file-error-00", -1), std::invalid_argument);
  EXPECT_THROW(workflow->addFile("file-01", 10), std::invalid_argument);

  EXPECT_THROW(workflow->getWorkflowFileByID("file-nonexist"), std::invalid_argument);
  EXPECT_EQ(workflow->getWorkflowFileByID("file-01")->getId(), "file-01");

  EXPECT_EQ(workflow->getInputFiles().size(), 1);
}

TEST_F(WorkflowTest, UpdateTaskState) {
  // testing update task state
  workflow->updateTaskState(t1, wrench::WorkflowTask::State::READY);
  ASSERT_EQ(1, workflow->getReadyTasks().size());

  // testing
  workflow->updateTaskState(t1, wrench::WorkflowTask::State::RUNNING);
  workflow->updateTaskState(t1, wrench::WorkflowTask::State::COMPLETED);
  workflow->updateTaskState(t2, wrench::WorkflowTask::State::READY);
  workflow->updateTaskState(t3, wrench::WorkflowTask::State::READY);
  ASSERT_EQ(1, workflow->getReadyTasks().size());
  EXPECT_EQ(2, workflow->getReadyTasks()["cluster-01"].size());
}

TEST_F(WorkflowTest, IsDone) {
  ASSERT_FALSE(workflow->isDone());

  for (auto task : workflow->getTasks()) {
    task->setState(wrench::WorkflowTask::State::RUNNING);
    task->setState(wrench::WorkflowTask::State::COMPLETED);
  }

  EXPECT_TRUE(workflow->isDone());
}

TEST_F(WorkflowTest, SumFlops) {

  double sum_flops = 0;

  EXPECT_NO_THROW(sum_flops = wrench::Workflow::getSumFlops(workflow->getTasks()));
  ASSERT_EQ(sum_flops, 4.0);
}
