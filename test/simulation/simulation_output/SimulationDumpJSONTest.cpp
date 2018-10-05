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

#include <algorithm>

/**********************************************************************/
/**                    SimulationDumpJSONTest                        **/
/**********************************************************************/

class SimulationDumpJSONTest : public ::testing::Test {

public:
    wrench::WorkflowTask *t1 = nullptr;
    wrench::WorkflowTask *t2 = nullptr;
    wrench::WorkflowTask *t3 = nullptr;
    wrench::WorkflowTask *t4 = nullptr;

    void do_SimulationDumpWorkflowExecutionJSON_test();
    void do_SimulationDumpWorkflowGraphJSON_test();

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
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("simulation_dump_workflow_execution_test");

    std::unique_ptr<wrench::Simulation> simulation = std::unique_ptr<wrench::Simulation>(new wrench::Simulation());

    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);

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

    nlohmann::json expected_json = R"(
        [
            {
                "compute": {
                    "end": -1.0,
                    "start": -1.0
                },
                "execution_host": {
                    "cores": 20,
                    "flop_rate": 1.0,
                    "hostname": "host2",
                    "memory": 20.0
                },
                "failed": -1.0,
                "num_cores_allocated": 10,
                "read": {
                    "end": -1.0,
                    "start": -1.0
                },
                "task_id": "task1",
                "terminated": -1.0,
                "whole_task": {
                    "end": -1.0,
                    "start": 2.0
                },
                "write": {
                    "end": -1.0,
                    "start": -1.0
                }
            },
            {
                "compute": {
                    "end": -1.0,
                    "start": -1.0
                },
                "execution_host": {
                    "cores": 10,
                    "flop_rate": 1.0,
                    "hostname": "host1",
                    "memory": 10.0
                },
                "failed": -1.0,
                "num_cores_allocated": 8,
                "read": {
                    "end": -1.0,
                    "start": -1.0
                },
                "task_id": "task1",
                "terminated": -1.0,
                "whole_task": {
                    "end": -1.0,
                    "start": 1.0
                },
                "write": {
                    "end": -1.0,
                    "start": -1.0
                }
            },
            {
                "compute": {
                    "end": -1.0,
                    "start": -1.0
                },
                "execution_host": {
                    "cores": 20,
                    "flop_rate": 1.0,
                    "hostname": "host2",
                    "memory": 20.0
                },
                "failed": -1.0,
                "num_cores_allocated": 20,
                "read": {
                    "end": -1.0,
                    "start": -1.0
                },
                "task_id": "task2",
                "terminated": -1.0,
                "whole_task": {
                    "end": -1.0,
                    "start": 3.0
                },
                "write": {
                    "end": -1.0,
                    "start": -1.0
                }
            }
        ]
    )"_json;

    std::string json_file_path("/tmp/workflow_data.json");
    EXPECT_THROW(simulation->dumpWorkflowExecutionJSON(nullptr, json_file_path), std::invalid_argument);
    EXPECT_THROW(simulation->dumpWorkflowExecutionJSON(workflow.get(), ""), std::invalid_argument);

    EXPECT_NO_THROW(simulation->dumpWorkflowExecutionJSON(workflow.get(), json_file_path));

    std::ifstream json_file(json_file_path);
    nlohmann::json result_json;
    json_file >> result_json;

    auto compare_ids = [] (nlohmann::json &lhs, nlohmann::json &rhs) -> bool {
        return lhs["whole_task"]["start"] < rhs["whole_task"]["start"];
    };

    /*
     * nlohmann::json doesn't maintain order when you push_back json objects into a vector so, before
     * comparing results with expected values, we sort the json lists so the test is deterministic.
     */
    std::sort(result_json.begin(), result_json.end(), compare_ids);
    std::sort(expected_json.begin(), expected_json.end(), compare_ids);

    EXPECT_TRUE(result_json == expected_json);

    free(argv[0]);
    free(argv);
}

TEST_F(SimulationDumpJSONTest, SimulationDumpWorkflowExecutionJSONTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpWorkflowExecutionJSON_test);
}

void SimulationDumpJSONTest::do_SimulationDumpWorkflowGraphJSON_test() {
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("simulation_dump_workflow_execution_test");

    std::unique_ptr<wrench::Simulation> simulation = std::unique_ptr<wrench::Simulation>(new wrench::Simulation());

    simulation->init(&argc, argv);

    EXPECT_THROW(simulation->dumpWorkflowGraphJSON(nullptr, "/tmp/file.json"), std::invalid_argument);
    EXPECT_THROW(simulation->dumpWorkflowGraphJSON((wrench::Workflow *)&argc, ""), std::invalid_argument);

    const std::string GRAPH_OUTPUT_FILE("/tmp/workflow_graph_data.json");
    std::ifstream graph_json_file;

    /*
     * nlohmann::json doesn't maintain order when you push_back json objects into a vector so, before
     * comparing results with expected values, we sort the json lists so the test is deterministic.
     */
    auto compare_links = [ ] (const nlohmann::json &lhs, const nlohmann::json &rhs) -> bool {
        if (lhs["source"] < rhs["source"]) {
            return true;
        }

        if (lhs["source"] > rhs["source"]) {
            return false;
        }

        if (lhs["target"] < rhs["target"]) {
            return true;
        }

        if (lhs["target"] > rhs["target"]) {
            return false;
        }

        return false;
    };


    auto compare_nodes = [ ] (const nlohmann::json &lhs, const nlohmann::json &rhs) -> bool {
        return lhs["id"] < rhs["id"];
    };


    // Generate a workflow with two independent tasks. Both tasks each have one input file and one output file.
    std::unique_ptr<wrench::Workflow> independent_tasks_workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

    t1 = independent_tasks_workflow->addTask("task1", 1.0, 1, 1, 1.0, 0);
    t1->addInputFile(independent_tasks_workflow->addFile("task1_input", 1.0));
    t1->addOutputFile(independent_tasks_workflow->addFile("task1_output", 1.0));

    t2 = independent_tasks_workflow->addTask("task2", 1.0, 1, 1, 1.0, 0);
    t2->addInputFile(independent_tasks_workflow->addFile("task2_input", 1.0));
    t2->addOutputFile(independent_tasks_workflow->addFile("task2_output", 1.0));

    EXPECT_NO_THROW(simulation->dumpWorkflowGraphJSON(independent_tasks_workflow.get(), GRAPH_OUTPUT_FILE));

    nlohmann::json result_json1;
    graph_json_file = std::ifstream(GRAPH_OUTPUT_FILE);
    graph_json_file >> result_json1;

    auto expected_json1 = R"(
    {
        "links": [
            {
                "source": "task1_input",
                "target": "task1"
            },
            {
                "source": "task1",
                "target": "task1_output"
            },
            {
                "source": "task2_input",
                "target": "task2"
            },
            {
                "source": "task2",
                "target": "task2_output"
            }
        ],
        "nodes": [
            {
                "flops": 1.0,
                "id": "task1",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "flops": 1.0,
                "id": "task2",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "id": "task1_input",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task1_output",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task2_input",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task2_output",
                "size": 1.0,
                "type": "file"
            }
        ]
    })"_json;

    std::sort(result_json1["links"].begin(), result_json1["links"].end(), compare_links);
    std::sort(result_json1["nodes"].begin(), result_json1["nodes"].end(), compare_nodes);

    std::sort(expected_json1["links"].begin(), expected_json1["links"].end(), compare_links);
    std::sort(expected_json1["nodes"].begin(), expected_json1["nodes"].end(), compare_nodes);

    EXPECT_TRUE(result_json1 == expected_json1);

    // Generate a workflow with two tasks, two input files, and four output files. Both tasks use both input files and produce two output files each.
    std::unique_ptr<wrench::Workflow> two_tasks_use_all_files_workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

    t1 = two_tasks_use_all_files_workflow->addTask("task1", 1.0, 1, 1, 1.0, 0);
    t1->addInputFile(two_tasks_use_all_files_workflow->addFile("input_file1", 1));
    t1->addInputFile(two_tasks_use_all_files_workflow->addFile("input_file2", 2));
    t1->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file1", 1));
    t1->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file2", 2));

    t2 = two_tasks_use_all_files_workflow->addTask("task2", 1.0, 1, 1, 1.0, 0);
    for (auto &file : t1->getInputFiles()) {
        t2->addInputFile(file);
    }

    t2->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file3", 1));
    t2->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file4", 1));

    EXPECT_NO_THROW(simulation->dumpWorkflowGraphJSON(two_tasks_use_all_files_workflow.get(), GRAPH_OUTPUT_FILE));

    nlohmann::json result_json2;
    graph_json_file = std::ifstream(GRAPH_OUTPUT_FILE);
    graph_json_file >> result_json2;

    auto expected_json2 = R"(
    {
        "links": [
            {
                "source": "input_file1",
                "target": "task1"
            },
            {
                "source": "input_file2",
                "target": "task1"
            },
            {
                "source": "task1",
                "target": "output_file1"
            },
            {
                "source": "task1",
                "target": "output_file2"
            },
            {
                "source": "input_file1",
                "target": "task2"
            },
            {
                "source": "input_file2",
                "target": "task2"
            },
            {
                "source": "task2",
                "target": "output_file3"
            },
            {
                "source": "task2",
                "target": "output_file4"
            }
        ],
        "nodes": [
            {
                "flops": 1.0,
                "id": "task1",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "flops": 1.0,
                "id": "task2",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "id": "input_file1",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "input_file2",
                "size": 2.0,
                "type": "file"
            },
            {
                "id": "output_file1",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "output_file2",
                "size": 2.0,
                "type": "file"
            },
            {
                "id": "output_file3",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "output_file4",
                "size": 1.0,
                "type": "file"
            }
        ]
    })"_json;

    std::sort(result_json2["links"].begin(), result_json2["links"].end(), compare_links);
    std::sort(result_json2["nodes"].begin(), result_json2["nodes"].end(), compare_nodes);

    std::sort(expected_json2["links"].begin(), expected_json2["links"].end(), compare_links);
    std::sort(expected_json2["nodes"].begin(), expected_json2["nodes"].end(), compare_nodes);

    EXPECT_TRUE(result_json2 == expected_json2);

    // Generate a workflow where one task forks into two tasks, then those two tasks join into one.
    std::unique_ptr<wrench::Workflow> fork_join_workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

    t1 = fork_join_workflow->addTask("task1", 1.0, 1, 1, 1.0, 0);
    t1->addInputFile(fork_join_workflow->addFile("task1_input", 1.0));
    t1->addOutputFile(fork_join_workflow->addFile("task1_output1", 1.0));
    t1->addOutputFile(fork_join_workflow->addFile("task1_output2", 1.0));

    t2 = fork_join_workflow->addTask("task2", 1.0, 1, 1, 1.0, 0);
    t2->addInputFile(fork_join_workflow->getFileByID("task1_output1"));
    t2->addOutputFile(fork_join_workflow->addFile("task2_output1", 1.0));
    fork_join_workflow->addControlDependency(t1, t2);

    t3 = fork_join_workflow->addTask("task3", 1.0, 1, 1, 1.0, 0);
    t3->addInputFile(fork_join_workflow->getFileByID("task1_output2"));
    t3->addOutputFile(fork_join_workflow->addFile("task3_output1", 1.0));
    fork_join_workflow->addControlDependency(t1, t3);

    t4 = fork_join_workflow->addTask("task4", 1.0, 1, 1, 1.0, 0);
    t4->addInputFile(fork_join_workflow->getFileByID("task2_output1"));
    t4->addInputFile(fork_join_workflow->getFileByID("task3_output1"));
    t4->addOutputFile(fork_join_workflow->addFile("task4_output1", 1.0));
    fork_join_workflow->addControlDependency(t2, t4);
    fork_join_workflow->addControlDependency(t3, t4);

    EXPECT_NO_THROW(simulation->dumpWorkflowGraphJSON(fork_join_workflow.get(), GRAPH_OUTPUT_FILE));

    nlohmann::json result_json3;
    graph_json_file = std::ifstream(GRAPH_OUTPUT_FILE);

    graph_json_file >> result_json3;

    auto expected_json3 = R"(
    {
        "links": [
            {
                "source": "task1_input",
                "target": "task1"
            },
            {
                "source": "task1",
                "target": "task1_output1"
            },
            {
                "source": "task1",
                "target": "task1_output2"
            },
            {
                "source": "task1_output1",
                "target": "task2"
            },
            {
                "source": "task2",
                "target": "task2_output1"
            },
            {
                "source": "task1_output2",
                "target": "task3"
            },
            {
                "source": "task3",
                "target": "task3_output1"
            },
            {
                "source": "task2_output1",
                "target": "task4"
            },
            {
                "source": "task3_output1",
                "target": "task4"
            },
            {
                "source": "task4",
                "target": "task4_output1"
            }
        ],
        "nodes": [
            {
                "flops": 1.0,
                "id": "task1",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "flops": 1.0,
                "id": "task2",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "flops": 1.0,
                "id": "task3",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "flops": 1.0,
                "id": "task4",
                "max_cores": 1,
                "memory": 0.0,
                "min_cores": 1,
                "parallel_efficiency": 1.0,
                "type": "task"
            },
            {
                "id": "task1_input",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task1_output1",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task1_output2",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task2_output1",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task3_output1",
                "size": 1.0,
                "type": "file"
            },
            {
                "id": "task4_output1",
                "size": 1.0,
                "type": "file"
            }
        ]
    })"_json;


    std::sort(result_json3["links"].begin(), result_json3["links"].end(), compare_links);
    std::sort(result_json3["nodes"].begin(), result_json3["nodes"].end(), compare_nodes);

    std::sort(expected_json3["links"].begin(), expected_json3["links"].end(), compare_links);
    std::sort(expected_json3["nodes"].begin(), expected_json3["nodes"].end(), compare_nodes);

    EXPECT_TRUE(result_json3 == expected_json3);

}

TEST_F(SimulationDumpJSONTest, SimulationDumpWorkflowGraphJSONTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpWorkflowGraphJSON_test);
}