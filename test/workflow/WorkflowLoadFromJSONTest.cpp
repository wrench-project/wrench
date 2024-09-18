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
#include <wrench/workflow/Workflow.h>
#include "../include/UniqueTmpPathPrefix.h"
#include <wrench/tools/wfcommons/WfCommonsWorkflowParser.h>

class WorkflowLoadFromJSONTest : public ::testing::Test {
protected:
    WorkflowLoadFromJSONTest() {
    }

    // data members
    std::string json_file_path = "../test/wfcommons_instances/1000genome-chameleon-2ch-100k-001.json";
};

TEST_F(WorkflowLoadFromJSONTest, LoadValidJSON) {

    std::shared_ptr<wrench::Workflow> workflow;

    ASSERT_THROW(workflow = wrench::WfCommonsWorkflowParser::createWorkflowFromJSON("bogus", "1f", false),
                 std::invalid_argument);
    ASSERT_NO_THROW(
            workflow = wrench::WfCommonsWorkflowParser::createWorkflowFromJSON(this->json_file_path, "1f", false));
    ASSERT_EQ(workflow->getNumberOfTasks(), 52);
    ASSERT_EQ(workflow->getFileMap().size(), 64);

    ASSERT_EQ(workflow->getTaskByID("individuals_ID0000001")->getPriority(), 20);
    ASSERT_EQ(workflow->getTaskByID("frequency_ID0000052")->getPriority(), 40);

    unsigned long num_input_files = workflow->getTaskByID("individuals_ID0000001")->getInputFiles().size();
    unsigned long num_output_files = workflow->getTaskByID("individuals_ID0000001")->getOutputFiles().size();

    ASSERT_EQ(num_input_files, 2);
    ASSERT_EQ(num_output_files, 1);

    //    ASSERT_NEAR(workflow->getTaskByID("individuals_ID0000001")->getFlops(), 588.914, 0.001);
    ASSERT_EQ(workflow->getTaskByID("individuals_ID0000001")->getMinNumCores(), 1);
    ASSERT_EQ(workflow->getTaskByID("individuals_ID0000001")->getMaxNumCores(), 1);

    ASSERT_EQ(workflow->getNumLevels(), 3);
    ASSERT_EQ(workflow->getTasksInTopLevelRange(0, 0).size(), 22);
    ASSERT_EQ(workflow->getTasksInTopLevelRange(1, 1).size(), 2);
    ASSERT_EQ(workflow->getTasksInTopLevelRange(2, 2).size(), 28);

    ASSERT_LT(workflow->getCompletionDate(), 0.0);
}
