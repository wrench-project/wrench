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

class WorkflowTaskTest : public ::testing::Test {
protected:
    WorkflowTaskTest() {
      workflow = new wrench::Workflow();
      workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(workflow);

      t1 = workflow->addTask("task-01", 100000, 1, 1, 1.0, 0);
      t2 = workflow->addTask("task-02", 100, 2, 4, 0.5, 0);

      workflow->addControlDependency(t1, t2);
    }

    // data members
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    wrench::Workflow *workflow;
    wrench::WorkflowTask *t1, *t2;
};

TEST_F(WorkflowTaskTest, TaskStructure) {
  // WorkflowTask structure sanity check
  ASSERT_EQ(t1->getWorkflow(), workflow);

  ASSERT_NE(t1->getID(), t2->getID());
  ASSERT_EQ(t1->getID(), "task-01");
  ASSERT_NE(t2->getID(), "task-01");

  ASSERT_GT(t1->getFlops(), t2->getFlops());

  ASSERT_EQ(t1->getMinNumCores(), 1);
  ASSERT_EQ(t1->getMaxNumCores(), 1);
  ASSERT_EQ(t1->getParallelEfficiency(), 1.0);
  ASSERT_EQ(t2->getMinNumCores(), 2);
  ASSERT_EQ(t2->getMaxNumCores(), 4);
  ASSERT_EQ(t2->getParallelEfficiency(), 0.5);

  ASSERT_EQ(t1->getState(), wrench::WorkflowTask::State::READY);
  ASSERT_EQ(t2->getState(), wrench::WorkflowTask::State::NOT_READY); // due to control dependency

  ASSERT_EQ(t1->getJob(), nullptr);
  ASSERT_EQ(t2->getJob(), nullptr);

  ASSERT_EQ(t1->getNumberOfParents(), 0);
  ASSERT_EQ(t2->getNumberOfParents(), 1);

  ASSERT_EQ(t1->getNumberOfChildren(), 1);
  ASSERT_EQ(t2->getNumberOfChildren(), 0);

  ASSERT_EQ(t1->getClusterID(), "");
}

TEST_F(WorkflowTaskTest, GetSet) {
  t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_NOT_READY);
  ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_NOT_READY);

  t1->setClusterID("my-cluster-id");
  ASSERT_EQ(t1->getClusterID(), "my-cluster-id");

  t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_READY);
  ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_READY);
  ASSERT_EQ(t2->getInternalState(), wrench::WorkflowTask::InternalState::TASK_NOT_READY);


  t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_RUNNING);
  ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_RUNNING);

  t1->setInternalState(wrench::WorkflowTask::InternalState::TASK_COMPLETED);
  ASSERT_EQ(t1->getInternalState(), wrench::WorkflowTask::InternalState::TASK_COMPLETED);
  t2->setInternalState(wrench::WorkflowTask::InternalState::TASK_READY);
  ASSERT_EQ(t2->getInternalState(), wrench::WorkflowTask::InternalState::TASK_READY);

  ASSERT_NO_THROW(t1->setEndDate(1.0));

  ASSERT_EQ(t1->getFailureCount(), 0);
  t1->incrementFailureCount();
  ASSERT_EQ(t1->getFailureCount(), 1);

  ASSERT_EQ(t1->getTaskType(), wrench::WorkflowTask::TaskType::COMPUTE);

  ASSERT_NO_THROW(t1->setComputationStartDate(1.0));
  ASSERT_DOUBLE_EQ(t1->getComputationStartDate(), 1.0);

  ASSERT_NO_THROW(t1->setComputationEndDate(1.0));
  ASSERT_DOUBLE_EQ(t1->getComputationEndDate(), 1.0);
}

TEST_F(WorkflowTaskTest, InputOutputFile) {
  wrench::WorkflowFile *f1 = workflow->addFile("file-01", 10);
  wrench::WorkflowFile *f2 = workflow->addFile("file-02", 100);

  t1->addInputFile(f1);
  t1->addOutputFile(f2);

  ASSERT_THROW(t1->addInputFile(f1), std::invalid_argument);
  ASSERT_THROW(t1->addOutputFile(f2), std::invalid_argument);

  ASSERT_THROW(t1->addInputFile(f2), std::invalid_argument);
  ASSERT_THROW(t1->addOutputFile(f1), std::invalid_argument);

  wrench::WorkflowTask* t3 = workflow->addTask("task-03", 50, 2, 4, 1.0, 0);
  t3->addInputFile(f2);

  ASSERT_EQ(t3->getNumberOfParents(), 1);
}

TEST_F(WorkflowTaskTest, StateToString) {

  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::NOT_READY), "NOT READY");
  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::READY), "READY");
  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::PENDING), "PENDING");
  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::State::COMPLETED), "COMPLETED");
  ASSERT_EQ(wrench::WorkflowTask::stateToString((wrench::WorkflowTask::State)100), "UNKNOWN STATE");

  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_READY), "READY");
  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_RUNNING), "RUNNING");
  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_COMPLETED), "COMPLETED");
  ASSERT_EQ(wrench::WorkflowTask::stateToString(wrench::WorkflowTask::InternalState::TASK_FAILED), "FAILED");
  ASSERT_EQ(wrench::WorkflowTask::stateToString((wrench::WorkflowTask::InternalState)100), "UNKNOWN STATE");
}


