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

#include "../../include/TestWithFork.h"

#include <fstream>

/**********************************************************************/
/**                    SimulationDumpJSONTest                        **/
/**********************************************************************/

class SimulationDumpJSONTest : public ::testing::Test {

public:
    wrench::WorkflowTask *t1 = nullptr;
    wrench::WorkflowTask *t2 = nullptr;

    void do_SimulationDumpWorkflowExecutionJSON_test();

protected:
    SimulationDumpJSONTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"host1\" speed=\"1f\" core=\"10\"> "
                          "         <prop id=\"ram\" value=\"10\"/>"
                          "       </host>"
                          "       <host id=\"host2\" speed=\"1f\" core=\"20\"> "
                          "          <prop id=\"ram\" value=\"20\"/> "
                          "       </host> "
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                          "       <route src=\"host1\" dst=\"host2\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
    }

    std::string platform_file_path = "/tmp/platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};

void SimulationDumpJSONTest::do_SimulationDumpWorkflowExecutionJSON_test() {
    workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

    t1 = workflow->addTask("task1", 1, 1, 1, 1.0, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 1.0, 0);

    t1->setStartDate(1.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresAllocated(8);

    t1->setStartDate(2.0);
    t1->setExecutionHost("host2");
    t1->setNumCoresAllocated(10);

    t2->setStartDate(3.0);
    t2->setExecutionHost("host2");
    t2->setNumCoresAllocated(20);

    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("simulation_dump_workflow_execution_test");

    std::unique_ptr<wrench::Simulation> simulation = std::unique_ptr<wrench::Simulation>(new wrench::Simulation());

    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);

    const nlohmann::json EXPECTED_JSON = R"(
        [
            {
                "task_id": "task1",
                "execution_host": {
                    "hostname": "host2",
                    "flop_rate": 1.0,
                    "memory": 20.0,
                    "cores": 20
                },
                "num_cores_allocated": 10,
                "whole_task": {"start": 2.0, "end": -1},
                "read": {"start": -1, "end": -1},
                "compute": {"start": -1, "end": -1},
                "write": {"start": -1, "end": -1},
                "failed": -1,
                "terminated": -1
            },
            {
                "task_id": "task1",
                "execution_host": {
                    "hostname": "host1",
                    "flop_rate": 1.0,
                    "memory": 10.0,
                    "cores": 10
                },
                "num_cores_allocated": 8,
                "whole_task": {"start": 1.0, "end": -1},
                "read": {"start": -1, "end": -1},
                "compute": {"start": -1, "end": -1},
                "write": {"start": -1, "end": -1},
                "failed": -1,
                "terminated": -1
            },
            {
                "task_id": "task2",
                "execution_host": {
                    "hostname": "host2",
                    "flop_rate": 1.0,
                    "memory": 20.0,
                    "cores": 20
                },
                "num_cores_allocated": 20,
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
    EXPECT_THROW(simulation->dumpWorkflowExecutionJSON(nullptr, json_file_path), std::invalid_argument);
    EXPECT_THROW(simulation->dumpWorkflowExecutionJSON(workflow.get(), ""), std::invalid_argument);

    EXPECT_NO_THROW(simulation->dumpWorkflowExecutionJSON(workflow.get(), json_file_path));

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

    free(argv[0]);
    free(argv);
}

TEST_F(SimulationDumpJSONTest, SimulationDumpJSONTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpWorkflowExecutionJSON_test);
}

