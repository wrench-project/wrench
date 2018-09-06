/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>

#include <wrench-dev.h>

#include <nlohmann/json.hpp>

#include <fstream>

/**********************************************************************/
/**                    SimulationDumpJSONTest                        **/
/**********************************************************************/

class SimulationDumpJSONTest : public ::testing::Test {
public:
    wrench::WorkflowTask *t1, *t2;
    std::unique_ptr<wrench::Workflow> workflow;
};

TEST_F(SimulationDumpJSONTest, SimulationDumpJSONTest) {

    workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

    t1 = workflow->addTask("task1", 1, 1, 1, 1.0, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 1.0, 0);

    std::unique_ptr<wrench::Simulation> simulation = std::unique_ptr<wrench::Simulation>(new wrench::Simulation());

    t1->setStartDate(1.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresUsed(10);

    t1->setStartDate(2.0);
    t1->setExecutionHost("host2");
    t1->setNumCoresUsed(20);

    t2->setStartDate(3.0);
    t2->setExecutionHost("host2");
    t2->setNumCoresUsed(30);

    const nlohmann::json EXPECTED_JSON = R"(
        [
            {
                "task_id": "task1",
                "execution_host": "host2",
                "num_cores_used": 20,
                "whole_task": {"start": 2.0, "end": -1},
                "read": {"start": -1, "end": -1},
                "compute": {"start": -1, "end": -1},
                "write": {"start": -1, "end": -1},
                "failed": -1,
                "terminated": -1
            },
            {
                "task_id": "task1",
                "execution_host": "host1",
                "num_cores_used": 10,
                "whole_task": {"start": 1.0, "end": -1},
                "read": {"start": -1, "end": -1},
                "compute": {"start": -1, "end": -1},
                "write": {"start": -1, "end": -1},
                "failed": -1,
                "terminated": -1
            },
            {
                "task_id": "task2",
                "execution_host": "host2",
                "num_cores_used": 30,
                "whole_task": {"start": 3.0, "end": -1},
                "read": {"start": -1, "end": -1},
                "compute": {"start": -1, "end": -1},
                "write": {"start": -1, "end": -1},
                "failed": -1,
                "terminated": -1
            }
        ]
    )"_json;

    std::string json_file_path("/tmp/workflow_data.json");
    EXPECT_THROW(simulation->dumpWorkflowTaskDataJSON(nullptr, json_file_path), std::invalid_argument);
    EXPECT_THROW(simulation->dumpWorkflowTaskDataJSON(workflow.get(), ""), std::invalid_argument);

    EXPECT_NO_THROW(simulation->dumpWorkflowTaskDataJSON(workflow.get(), json_file_path));

    std::ifstream json_file(json_file_path);
    nlohmann::json RESULT_JSON;
    json_file >> RESULT_JSON;


    /*
     * The setters/getters for WorkflowTask data are tested in WorkflowTaskTest.cpp using a simulated environment.
     * Here we want to make sure that all task runs (whether they were successful or not) have been written as json
     * to the file.
     */
    EXPECT_TRUE(RESULT_JSON == EXPECTED_JSON);
    //std::cerr << std::setw(4) << RESULT_JSON << std::endl;
}

