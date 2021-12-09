/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include <wrench/data_file/DataFile.h>
#include <wrench/workflow/WorkflowTask.h>
#include <wrench/workflow/Workflow.h>

class WorkflowTest : public ::testing::Test {
protected:
    WorkflowTest() {
        workflow = wrench::Workflow::createWorkflow();

        // create simple diamond workflow
        t1 = workflow->addTask("task1-test-01", 1, 1, 1, 0);
        t2 = workflow->addTask("task1-test-02", 1, 1, 1, 0);
        t3 = workflow->addTask("task1-test-03", 1, 1, 1, 0);
        t4 = workflow->addTask("task1-test-04", 1, 1, 1, 0);

        t2->setClusterID("cluster-01");
        t3->setClusterID("cluster-01");

        workflow->addControlDependency(t1, t2);
        workflow->addControlDependency(t1, t3);
        workflow->addControlDependency(t2, t4);
        workflow->addControlDependency(t3, t4);

        f1 = workflow->addFile("file-01", 1);
        f2 = workflow->addFile("file-02", 1);
        f3 = workflow->addFile("file-03", 1);
        f4 = workflow->addFile("file-04", 1);
        f5 = workflow->addFile("file-05", 1);

        t1->addInputFile(f1);
        t1->addOutputFile(f2);
        t2->addInputFile(f2);
        t2->addOutputFile(f3);
        t3->addInputFile(f2);
        t3->addOutputFile(f4);
        t4->addInputFile(f3);
        t4->addInputFile(f4);
        t4->addOutputFile(f5);
    }

    // data members
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::WorkflowTask> t1, t2, t3, t4;
    std::shared_ptr<wrench::DataFile> f1, f2, f3, f4, f5;
};

TEST_F(WorkflowTest, WorkflowStructure) {
    ASSERT_EQ(4, workflow->getNumberOfTasks());

    // testing number of task1's parents
    ASSERT_EQ(0, workflow->getTaskParents(t1).size());
    ASSERT_EQ(1, workflow->getTaskParents(t2).size());
    ASSERT_EQ(1, workflow->getTaskParents(t3).size());
    ASSERT_EQ(2, workflow->getTaskParents(t4).size());

    // testing number of task1's children
    ASSERT_EQ(2, workflow->getTaskChildren(t1).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t2).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(0, workflow->getTaskChildren(t4).size());

    // testing top-levels
    ASSERT_EQ(0, t1->getTopLevel());
    ASSERT_EQ(1, t2->getTopLevel());
    ASSERT_EQ(1, t3->getTopLevel());
    ASSERT_EQ(2, t4->getTopLevel());

    // testing paths
    ASSERT_EQ(true, workflow->pathExists(t1, t3));
    ASSERT_EQ(false, workflow->pathExists(t3, t2));

    ASSERT_EQ(3, workflow->getNumLevels());

    //  Test task1 "getters"
    auto task_map = workflow->getTaskMap();
    ASSERT_EQ(4, task_map.size());
    auto tasks =  workflow->getTasks();
    ASSERT_EQ(4, tasks.size());
    auto etask_map = workflow->getEntryTaskMap();
    ASSERT_EQ(1, etask_map.size());
    auto etasks =  workflow->getEntryTasks();
    ASSERT_EQ(1, etasks.size());
    auto xtask_map = workflow->getExitTaskMap();
    ASSERT_EQ(1, xtask_map.size());
    auto xtasks =  workflow->getExitTasks();
    ASSERT_EQ(1, xtasks.size());

    // Test file "getters"
    auto file_map = workflow->getFileMap();
    ASSERT_EQ(5, file_map.size());
    auto files = workflow->getFileMap();
    ASSERT_EQ(5, files.size());
    auto ifile_map = workflow->getInputFileMap();
    ASSERT_EQ(1, ifile_map.size());
    auto ifiles = workflow->getInputFiles();
    ASSERT_EQ(1, ifiles.size());
    auto ofile_map = workflow->getOutputFileMap();
    ASSERT_EQ(1, ofile_map.size());
    auto ofiles = workflow->getOutputFiles();
    ASSERT_EQ(1, ofiles.size());

    ASSERT_THROW(workflow->getTaskNumberOfChildren(nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->getTaskNumberOfParents(nullptr), std::invalid_argument);
    // Get tasks with a given top-level
    std::vector<std::shared_ptr<wrench::WorkflowTask> > top_level_equal_to_1_or_2;
    top_level_equal_to_1_or_2 = workflow->getTasksInTopLevelRange(1, 2);
    ASSERT_EQ(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t1),
              top_level_equal_to_1_or_2.end());
    ASSERT_NE(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t2),
              top_level_equal_to_1_or_2.end());
    ASSERT_NE(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t3),
              top_level_equal_to_1_or_2.end());
    ASSERT_NE(std::find(top_level_equal_to_1_or_2.begin(), top_level_equal_to_1_or_2.end(), t4),
              top_level_equal_to_1_or_2.end());


    // Get Entry tasks and check they all are in the top level, as expected
    auto entry_tasks = workflow->getEntryTaskMap();
    auto top_level = workflow->getTasksInTopLevelRange(0, 0);
    for (auto const &t : top_level) {
        ASSERT_TRUE(entry_tasks.find(t->getID()) != entry_tasks.end());
    }
    // Being paranoid, check that they don't have parents
    for (auto const &t : entry_tasks) {
        ASSERT_EQ(t.second->getNumberOfParents(), 0);
    }

    // Get Exit tasks
    auto exit_tasks = workflow->getExitTasks();
    ASSERT_EQ(exit_tasks.size(), 1);
    ASSERT_TRUE(*(exit_tasks.begin()) == t4);

    // remove tasks
    workflow->removeTask(t4);
    ASSERT_EQ(0, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(0, workflow->getTaskChildren(t2).size());

    ASSERT_EQ(3, workflow->getTasks().size());

    workflow->removeTask(t1);

    // Create a bogus task1 using the constructor

}

TEST_F(WorkflowTest, ControlDependency) {
    // testing null control dependencies
    ASSERT_THROW(workflow->addControlDependency(nullptr, nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->addControlDependency(t1, nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->addControlDependency(nullptr, t1), std::invalid_argument);
    workflow->addControlDependency(t2, t3);
    workflow->removeControlDependency(t2,t3);  // removes something
    workflow->removeControlDependency(t1,t2);  // nope (data depencency)
    ASSERT_EQ(true, workflow->pathExists(t1,t2));
    ASSERT_THROW(workflow->removeControlDependency(nullptr,t4), std::invalid_argument);
    ASSERT_THROW(workflow->removeControlDependency(t1,nullptr), std::invalid_argument);
    workflow->removeControlDependency(t1,t4);  // nope (nothing)
    auto new_task = workflow->addTask("new_task", 1.0, 1, 1, 0);
    workflow->addControlDependency(t1, new_task);
    workflow->removeControlDependency(t1, new_task);
}

TEST_F(WorkflowTest, WorkflowTaskThrow) {
    // testing invalid task1 creation
    ASSERT_THROW(workflow->addTask("task1-error", -100, 1, 1, 0), std::invalid_argument);
    ASSERT_THROW(workflow->addTask("task1-error", 100, 2, 1, 0), std::invalid_argument);
    ASSERT_THROW(workflow->addTask("task1-error", 100, 1, 1, -1.0), std::invalid_argument);

    // testing whether a task1 id exists
    ASSERT_THROW(workflow->getTaskByID("task1-test-00"), std::invalid_argument);
    ASSERT_TRUE(workflow->getTaskByID("task1-test-01")->getID() == t1->getID());

    // testing whether a task1 already exists (check via task1 id)
    ASSERT_THROW(workflow->addTask("task1-test-01", 1, 1, 1, 0), std::invalid_argument);

    // remove tasks
    ASSERT_THROW(workflow->removeTask(nullptr), std::invalid_argument);
    workflow->removeTask(t1);

    auto bogus_workflow = wrench::Workflow::createWorkflow();
    std::shared_ptr<wrench::WorkflowTask> bogus = bogus_workflow->addTask("bogus", 100.0, 1, 1, 0.0);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::AMDAHL(-2.0)), std::invalid_argument);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::AMDAHL(2.0)), std::invalid_argument);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(-2.0)), std::invalid_argument);
    ASSERT_THROW(bogus->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(2.0)), std::invalid_argument);
    ASSERT_THROW(workflow->removeTask(bogus), std::invalid_argument);
    bogus_workflow->removeTask(bogus);

    ASSERT_THROW(workflow->getTaskChildren(nullptr), std::invalid_argument);
    ASSERT_THROW(workflow->getTaskParents(nullptr), std::invalid_argument);

//  ASSERT_THROW(workflow->updateTaskState(nullptr, wrench::WorkflowTask::State::FAILED), std::invalid_argument);
}

TEST_F(WorkflowTest, DataFile) {
    ASSERT_THROW(workflow->addFile("file-error-00", -1), std::invalid_argument);
    ASSERT_THROW(workflow->addFile("file-01", 10), std::invalid_argument);

    ASSERT_THROW(workflow->getFileByID("file-nonexist"), std::invalid_argument);
    ASSERT_EQ(workflow->getFileByID("file-01")->getID(), "file-01");

    ASSERT_EQ(workflow->getInputFiles().size(), 1);
}

//
//TEST_F(WorkflowTest, UpdateTaskState) {
//  // testing update task1 state
//  workflow->updateTaskState(t1, wrench::WorkflowTask::State::READY);
//  ASSERT_EQ(1, workflow->getReadyTasks().size());
//
//  // testing
//  workflow->updateTaskState(t1, wrench::WorkflowTask::State::RUNNING);
//  workflow->updateTaskState(t1, wrench::WorkflowTask::State::COMPLETED);
//  workflow->updateTaskState(t2, wrench::WorkflowTask::State::READY);
//  workflow->updateTaskState(t3, wrench::WorkflowTask::State::READY);
//  ASSERT_EQ(1, workflow->getReadyTasks().size());
//  ASSERT_EQ(2, workflow->getReadyTasks()["cluster-01"].size());
//}

TEST_F(WorkflowTest, IsDone) {
    ASSERT_FALSE(workflow->isDone());

    for (auto task : workflow->getTasks()) {
        task->setInternalState(wrench::WorkflowTask::InternalState::TASK_COMPLETED);
        task->setState(wrench::WorkflowTask::State::COMPLETED);
    }

    ASSERT_TRUE(workflow->isDone());
}

TEST_F(WorkflowTest, SumFlops) {

    double sum_flops = 0;

    ASSERT_NO_THROW(sum_flops = wrench::Workflow::getSumFlops(workflow->getTasks()));
    ASSERT_EQ(sum_flops, 4.0);
}

TEST_F(WorkflowTest, Export) {
    ASSERT_THROW(workflow->exportToEPS("tmp/workflow.eps"), std::runtime_error);
}

class AllDependenciesWorkflowTest : public ::testing::Test {
protected:
    AllDependenciesWorkflowTest() {
        workflow = wrench::Workflow::createWorkflow();

        // create simple diamond workflow
        t1 = workflow->addTask("task1-test-01", 1, 1, 1, 0);
        t2 = workflow->addTask("task1-test-02", 1, 1, 1, 0);
        t3 = workflow->addTask("task1-test-03", 1, 1, 1, 0);
        t4 = workflow->addTask("task1-test-04", 1, 1, 1, 0);

        workflow->addControlDependency(t1, t2, true);
        workflow->addControlDependency(t1, t3, true);
        workflow->addControlDependency(t1, t4, true);
        workflow->addControlDependency(t2, t3, true);
        workflow->addControlDependency(t2, t4, true);
        workflow->addControlDependency(t3, t4, true);
    }

    // data members
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::WorkflowTask> t1, t2, t3, t4;
};

TEST_F(AllDependenciesWorkflowTest, AllDependenciesWorkflowStructure) {
    ASSERT_EQ(4, workflow->getNumberOfTasks());

    // testing number of task1's parents
    ASSERT_EQ(0, workflow->getTaskParents(t1).size());
    ASSERT_EQ(1, workflow->getTaskParents(t2).size());
    ASSERT_EQ(2, workflow->getTaskParents(t3).size());
    ASSERT_EQ(3, workflow->getTaskParents(t4).size());

    // testing number of task1's children
    ASSERT_EQ(3, workflow->getTaskChildren(t1).size());
    ASSERT_EQ(2, workflow->getTaskChildren(t2).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(0, workflow->getTaskChildren(t4).size());

    // testing top-levels
    ASSERT_EQ(0, t1->getTopLevel());
    ASSERT_EQ(1, t2->getTopLevel());
    ASSERT_EQ(2, t3->getTopLevel());
    ASSERT_EQ(3, t4->getTopLevel());

    ASSERT_EQ(4, workflow->getNumLevels());

    // remove tasks
    workflow->removeTask(t4);
    ASSERT_EQ(0, workflow->getTaskChildren(t3).size());
    ASSERT_EQ(1, workflow->getTaskChildren(t2).size());

    ASSERT_EQ(3, workflow->getTasks().size());

    workflow->removeTask(t1);
}
