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
#include "../../include/UniqueTmpPathPrefix.h"

#include <fstream>

#include <algorithm>

/**********************************************************************/
/**                    SimulationDumpJSONTest                        **/
/**********************************************************************/

class SimulationDumpJSONTest : public ::testing::Test {

public:

    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::StorageService> ss1;
    std::shared_ptr<wrench::StorageService> ss2;

    std::shared_ptr<wrench::StorageService> client_storage_service;
    std::shared_ptr<wrench::StorageService> server_storage_service;

    std::shared_ptr<wrench::WorkflowTask> t1 = nullptr;
    std::shared_ptr<wrench::WorkflowTask> t2 = nullptr;
    std::shared_ptr<wrench::WorkflowTask> t3 = nullptr;
    std::shared_ptr<wrench::WorkflowTask> t4 = nullptr;

    void do_SimulationDumpWorkflowExecutionJSON_test();
    void do_SimulationDumpWorkflowGraphJSON_test();
    void do_SimulationSearchForHostUtilizationGraphLayout_test();
    void do_SimulationDumpHostEnergyConsumptionJSON_test();
    void do_SimulationDumpPlatformGraphJSON_test();
    void do_SimulationDumpPlatformGraphJSONBrokenRouting_test();
    void do_SimulationDumpLinkUsageJSON_test();
    void do_SimulationDumpDiskOperationsJSON_test();
    void do_SimulationDumpUnifiedJSON_test();

protected:

    ~SimulationDumpJSONTest() {
//        std::cerr << "WORKFLOW = " << workflow.get() << "\n";
        if (workflow.get()) workflow->clear();
    }

    SimulationDumpJSONTest() {

        // platform without energy consumption information
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"host1\" speed=\"1f\" core=\"10\"> "
                          "         <prop id=\"ram\" value=\"10B\"/>"
                          "       </host>"
                          "       <host id=\"host2\" speed=\"1f\" core=\"20\"> "
                          "          <prop id=\"ram\" value=\"20B\"/> "
                          "       </host> "
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                          "       <route src=\"host1\" dst=\"host2\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

        // platform with energy consumption information
        std::string xml2 = "<?xml version='1.0'?>"
                           "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                           "<platform version=\"4.1\">"
                           "<zone id=\"AS0\" routing=\"Full\">"

                           "<host id=\"host1\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"1\" core=\"1\" >"
                           "<prop id=\"wattage_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
                           "<prop id=\"wattage_off\" value=\"10\" />"
                           "</host>"

                           "<host id=\"host2\" speed=\"100.0Mf,50.0Mf,20.0Mf\" pstate=\"0\" core=\"1\" >"
                           "<prop id=\"wattage_per_state\" value=\"100.0:200.0, 93.0:170.0, 90.0:150.0\" />"
                           "<prop id=\"wattage_off\" value=\"10\" />"
                           "</host>"

                           "</zone>"
                           "</platform>";

        FILE *platform_file2 = fopen(platform_file_path2.c_str(), "w");
        fprintf(platform_file2, "%s", xml2.c_str());
        fclose(platform_file2);

        // 3 host platform with an asymmetrical route between host1 and host3
        std::string xml3 = "<?xml version='1.0'?>"
                           "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                           "<platform version=\"4.1\"> "
                           "   <zone id=\"AS0\" routing=\"Full\"> "
                           "       <host id=\"host1\" speed=\"1f\" core=\"10\"> "
                           "         <prop id=\"ram\" value=\"10B\"/>"
                           "       </host>"
                           "       <host id=\"host2\" speed=\"1f\" core=\"20\"> "
                           "          <prop id=\"ram\" value=\"20B\"/> "
                           "       </host> "
                           "       <host id=\"host3\" speed=\"1f\" core=\"20\"> "
                           "          <prop id=\"ram\" value=\"20B\"/> "
                           "       </host> "
                           "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                           "       <link id=\"2\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                           "       <link id=\"3\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                           "       <route src=\"host1\" dst=\"host2\"> <link_ctn id=\"1\"/> </route>"
                           "       <route src=\"host2\" dst=\"host3\"> <link_ctn id=\"2\"/> </route>"
                           "       <route src=\"host1\" dst=\"host3\" symmetrical=\"NO\"> <link_ctn id=\"1\"/> <link_ctn id=\"2\"/> </route>"
                           "       <route src=\"host3\" dst=\"host1\" symmetrical=\"NO\"> <link_ctn id=\"3\"/> </route>"
                           "   </zone> "
                           "</platform>";
        FILE *platform_file3 = fopen(platform_file_path3.c_str(), "w");
        fprintf(platform_file3, "%s", xml3.c_str());
        fclose(platform_file3);

        // 3 host platform with full routing but not all connections specified
        std::string xml3_broken = "<?xml version='1.0'?>"
                                  "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                                  "<platform version=\"4.1\"> "
                                  "   <zone id=\"AS0\" routing=\"Full\"> "
                                  "       <host id=\"host1\" speed=\"1f\" core=\"10\"> "
                                  "         <prop id=\"ram\" value=\"10B\"/>"
                                  "       </host>"
                                  "       <host id=\"host2\" speed=\"1f\" core=\"20\"> "
                                  "          <prop id=\"ram\" value=\"20B\"/> "
                                  "       </host> "
                                  "       <host id=\"host3\" speed=\"1f\" core=\"20\"> "
                                  "          <prop id=\"ram\" value=\"20B\"/> "
                                  "       </host> "
                                  "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                                  "       <link id=\"2\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                                  "       <link id=\"3\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                                  "       <route src=\"host1\" dst=\"host2\"> <link_ctn id=\"1\"/> </route>"
                                  "       <route src=\"host2\" dst=\"host3\"> <link_ctn id=\"2\"/> </route>"
                                  "   </zone> "
                                  "</platform>";
        FILE *platform_file3_broken = fopen(platform_file_path3_broken.c_str(), "w");
        fprintf(platform_file3_broken, "%s", xml3_broken.c_str());
        fclose(platform_file3_broken);


        // platform with cluster
        std::string xml4 = "<?xml version='1.0'?>"
                           "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                           "<platform version=\"4.1\">"
                           "   <zone id=\"AS0\" routing=\"Full\">"
                           "     <!-- effective bandwidth = 1250 MBps -->"
                           "     <cluster id=\"hpc.edu\" prefix=\"hpc.edu/node_\" suffix=\"\" radical=\"0-3"
                           "\" core=\"1\" speed=\"1000Gf\" bw=\"1288.6597MBps\" lat=\"10us\" router_id=\"hpc_gateway\">"
                           "         <prop id=\"ram\" value=\"80000000000B\"/>"
                           "        </cluster>"
                           "      <zone id=\"AS2\" routing=\"Full\">"
                           "          <host id=\"storage_db.edu\" speed=\"1000Gf\"/>"
                           "      </zone>"
                           "      <zone id=\"AS3\" routing=\"Full\">"
                           "          <host id=\"my_lab_computer.edu\" speed=\"1000Gf\" core=\"1\"/>"
                           "      </zone>"
                           "      <!-- effective bandwidth = 125 MBps -->"
                           "      <link id=\"link1\" bandwidth=\"128.8659MBps\" latency=\"100us\"/>"
                           "      <zoneRoute src=\"AS2\" dst=\"hpc.edu\" gw_src=\"storage_db.edu\" gw_dst=\"hpc_gateway\">"
                           "        <link_ctn id=\"link1\"/>"
                           "      </zoneRoute>"
                           "      <zoneRoute src=\"AS3\" dst=\"hpc.edu\" gw_src=\"my_lab_computer.edu\" gw_dst=\"hpc_gateway\">"
                           "        <link_ctn id=\"link1\"/>"
                           "      </zoneRoute>"
                           "      <zoneRoute src=\"AS3\" dst=\"AS2\" gw_src=\"my_lab_computer.edu\" gw_dst=\"storage_db.edu\">"
                           "        <link_ctn id=\"link1\"/>"
                           "      </zoneRoute>"
                           "   </zone>"
                           "</platform>";

        FILE *platform_file4 = fopen(platform_file_path4.c_str(), "w");
        fprintf(platform_file4, "%s", xml4.c_str());
        fclose(platform_file4);

        // platform for Link Usage
        std::string xml5 = "<?xml version='1.0'?>"
                           "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                           "<platform version=\"4.1\"> "
                           "   <zone id=\"AS0\" routing=\"Full\"> "
                           "       <host id=\"host1\" speed=\"1f\" core=\"10\"> "
                           "         <prop id=\"ram\" value=\"10B\"/>"
                           "         <disk id=\"large_disk\" read_bw=\"100000TBps\" write_bw=\"100000TBps\">"
                           "                            <prop id=\"size\" value=\"5000GiB\"/>"
                           "                            <prop id=\"mount\" value=\"/\"/>"
                           "         </disk>"
                           "       </host>"
                           "       <host id=\"host2\" speed=\"1f\" core=\"20\"> "
                           "          <prop id=\"ram\" value=\"20B\"/> "
                           "          <disk id=\"large_disk1\" read_bw=\"100000TBps\" write_bw=\"100000TBps\">"
                           "                            <prop id=\"size\" value=\"5000GiB\"/>"
                           "                            <prop id=\"mount\" value=\"/\"/>"
                           "       </disk>"
                           "       </host>"
                           "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                           "       <route src=\"host1\" dst=\"host2\"> <link_ctn id=\"1\"/> </route>"
                           "   </zone> "
                           "</platform>";
        FILE *platform_file5 = fopen(platform_file_path5.c_str(), "w");
        fprintf(platform_file5, "%s", xml5.c_str());
        fclose(platform_file5);

        // platform for Disk Operations
        std::string xml6 = "<?xml version='1.0'?>"
                           "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                           "<platform version=\"4.1\"> "
                           "   <zone id=\"AS0\" routing=\"Full\"> "
                           "       <host id=\"host1\" speed=\"1f\" core=\"1\"> "
                           "         <disk id=\"large_disk\" read_bw=\"1MBps\" write_bw=\"1MBps\">"
                           "                            <prop id=\"size\" value=\"5000GiB\"/>"
                           "                            <prop id=\"mount\" value=\"/\"/>"
                           "         </disk>"
                           "       </host>"
                           "       <host id=\"host2\" speed=\"1f\" core=\"2\"> "
                           "          <disk id=\"large_disk1\" read_bw=\"2MBps\" write_bw=\"2MBps\">"
                           "                            <prop id=\"size\" value=\"5000GiB\"/>"
                           "                            <prop id=\"mount\" value=\"/\"/>"
                           "       </disk>"
                           "       </host>"
                           "       <link id=\"1\" bandwidth=\"1000Tbps\" latency=\"1us\"/>"
                           "       <route src=\"host1\" dst=\"host2\"> <link_ctn id=\"1\"/> </route>"
                           "       <route src=\"host1\" dst=\"host1\"> <link_ctn id=\"1\"/> </route>"
                           "       <route src=\"host2\" dst=\"host2\"> <link_ctn id=\"1\"/> </route>"
                           "   </zone> "
                           "</platform>";
        FILE *platform_file6 = fopen(platform_file_path6.c_str(), "w");
        fprintf(platform_file6, "%s", xml6.c_str());
        fclose(platform_file6);

        workflow = wrench::Workflow::createWorkflow();
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::string platform_file_path2 = UNIQUE_TMP_PATH_PREFIX + "platform2.xml";
    std::string platform_file_path3 = UNIQUE_TMP_PATH_PREFIX + "platform3.xml";
    std::string platform_file_path3_broken = UNIQUE_TMP_PATH_PREFIX + "platform3_broken.xml";
    std::string platform_file_path4 = UNIQUE_TMP_PATH_PREFIX + "platform4.xml";
    std::string platform_file_path5 = UNIQUE_TMP_PATH_PREFIX + "platform5.xml";
    std::string platform_file_path6 = UNIQUE_TMP_PATH_PREFIX + "platform6.xml";
    std::string execution_data_json_file_path = UNIQUE_TMP_PATH_PREFIX + "workflow_data.json";
    std::string workflow_graph_json_file_path = UNIQUE_TMP_PATH_PREFIX + "workflow_graph_data.json";
    std::string energy_consumption_data_file_path = UNIQUE_TMP_PATH_PREFIX + "energy_consumption.json";
    std::string platform_graph_json_file_path = UNIQUE_TMP_PATH_PREFIX + "platform_graph.json";
    std::string link_usage_json_file_path = UNIQUE_TMP_PATH_PREFIX + "link_usage.json";
    std::string disk_operations_json_file_path = UNIQUE_TMP_PATH_PREFIX + "disk_operations.json";
    std::string unified_json_file_path = UNIQUE_TMP_PATH_PREFIX + "unified_output.json";

};

// some comparison functions to be used when sorting lists of JSON objects so that the tests are deterministic
bool compareStartTimes(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return lhs["whole_task"]["start"] < rhs["whole_task"]["start"];
}

bool compareTaskIDs(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return lhs["task_id"] < rhs["task_id"];
}

bool compareObjects(const nlohmann::json &lhs, const nlohmann::json &rhs){
    bool test = true;
    for (auto& x : lhs.items()){
        if (lhs[x.key()] != rhs[x.key()]){
            test = false;
        }
    }
    return test;
}


bool compareNodes(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return lhs["id"] < rhs["id"];
}

bool compareLinks(const nlohmann::json &lhs, const nlohmann::json &rhs) {
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
}

/**********************************************************************/
/**          SimulationDumpWorkflowExecutionJSONTest                 **/
/**********************************************************************/

void SimulationDumpJSONTest::do_SimulationDumpWorkflowExecutionJSON_test() {
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);

    workflow = wrench::Workflow::createWorkflow();

    t1 = workflow->addTask("task1", 1, 1, 1, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 0);

    t1->setStartDate(1.0);
    t1->setEndDate(2.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresAllocated(8);

    t1->setStartDate(2.0);
    t1->setEndDate(3.0);
    t1->setExecutionHost("host2");
    t1->setNumCoresAllocated(10);

    t2->setStartDate(3.0);
    t2->setEndDate(4.0);
    t2->setExecutionHost("host2");
    t2->setNumCoresAllocated(20);

    nlohmann::json expected_json = R"(
    {
        "workflow_execution": {
            "tasks": [
                {
                    "compute": {
                        "end": -1.0,
                        "start": -1.0
                    },
                    "execution_host": {
                        "cores": 20,
                        "flop_rate": 1.0,
                        "hostname": "host2",
                        "memory_manager_service": 20.0
                    },
                    "failed": -1.0,
                    "num_cores_allocated": 10,
                    "read": null,
                    "task_id": "task1",
                    "color": "",
                    "terminated": -1.0,
                    "whole_task": {
                        "end": 3.0,
                        "start": 2.0
                    },
                    "write": null
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
                        "memory_manager_service": 10.0
                    },
                    "failed": -1.0,
                    "num_cores_allocated": 8,
                    "read": null,
                    "task_id": "task1",
                    "color": "",
                    "terminated": -1.0,
                    "whole_task": {
                        "end": 2.0,
                        "start": 1.0
                    },
                    "write": null
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
                        "memory_manager_service": 20.0
                    },
                    "failed": -1.0,
                    "num_cores_allocated": 20,
                    "read": null,
                    "task_id": "task2",
                    "color": "",
                    "terminated": -1.0,
                    "whole_task": {
                        "end": 4.0,
                        "start": 3.0
                    },
                    "write": null
                }
            ]
        }
    }
    )"_json;

    EXPECT_THROW(simulation->getOutput().dumpWorkflowExecutionJSON(nullptr, execution_data_json_file_path), std::invalid_argument);
    EXPECT_THROW(simulation->getOutput().dumpWorkflowExecutionJSON(workflow, ""), std::invalid_argument);

    EXPECT_NO_THROW(simulation->getOutput().dumpWorkflowExecutionJSON(workflow, execution_data_json_file_path, false));

    std::ifstream json_file(execution_data_json_file_path);
    nlohmann::json result_json;
    json_file >> result_json;


    /*
     * nlohmann::json doesn't maintain order when you push_back json objects into a vector so, before
     * comparing results with expected values, we sort the json lists so the test is deterministic.
     */

    std::sort(result_json["workflow_execution"]["tasks"].begin(), result_json["workflow_execution"]["tasks"].end(), compareStartTimes);
    std::sort(expected_json["workflow_execution"]["tasks"].begin(), expected_json["workflow_execution"]["tasks"].end(), compareStartTimes);

    EXPECT_TRUE(result_json == expected_json);

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

TEST_F(SimulationDumpJSONTest, SimulationDumpWorkflowExecutionJSONTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpWorkflowExecutionJSON_test);
}

/**********************************************************************/
/**         SimulationSearchForHostUtilizationGraphLayout            **/
/**********************************************************************/
void SimulationDumpJSONTest::do_SimulationSearchForHostUtilizationGraphLayout_test() {
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);

    std::ifstream json_file;


    workflow = wrench::Workflow::createWorkflow();

    t1 = workflow->addTask("task1", 1, 1, 1, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 0);

    /*
     * Two tasks run in parallel on a single host. Both use 5 out of the 10 cores.
     * We expect the following vertical positions to be set:
     *  - task1: 0 (uses cores 0-4)
     *  - task2: 5 (uses cores 5-9)
     */
    t1->setStartDate(1.0);
    t1->setEndDate(2.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresAllocated(5);

    t2->setStartDate(1.0);
    t2->setEndDate(2.0);
    t2->setExecutionHost("host1");
    t2->setNumCoresAllocated(5);

    nlohmann::json expected_json1 = R"(
    {
        "task1": 0,
        "task2": 5
    }
    )"_json;


    EXPECT_NO_THROW(simulation->getOutput().dumpWorkflowExecutionJSON(workflow, execution_data_json_file_path, true));

    json_file = std::ifstream("host_utilization_layout.json");
    nlohmann::json result_json1;
    json_file >> result_json1;


    EXPECT_TRUE(compareObjects(result_json1,expected_json1));

    workflow->clear();

    workflow = wrench::Workflow::createWorkflow();

    t1 = workflow->addTask("task1", 1, 1, 1, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 0);

    /*
     * Two tasks run, one after the other on host1. task2 starts at the same time that task1 ends.
     * Both tasks should have a vertical position of 0.
     */
    t1->setStartDate(1.0);
    t1->setEndDate(2.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresAllocated(5);

    t2->setStartDate(2.0);
    t2->setEndDate(3.0);
    t2->setExecutionHost("host1");
    t2->setNumCoresAllocated(5);

    nlohmann::json expected_json2 = R"(
    {
        "task1": 0,
        "task2": 0
    }
    )"_json;


    EXPECT_NO_THROW(simulation->getOutput().dumpWorkflowExecutionJSON(workflow, execution_data_json_file_path, true));

    json_file = std::ifstream("host_utilization_layout.json");
    nlohmann::json result_json2;
    json_file >> result_json2;


    EXPECT_TRUE(compareObjects(result_json2,expected_json2));

    workflow->clear();

    workflow = wrench::Workflow::createWorkflow();

    t1 = workflow->addTask("task1", 1, 1, 1, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 0);
    t3 = workflow->addTask("task3", 1, 1, 1, 0);
    t4 = workflow->addTask("task4", 1, 1, 1, 0);

    /*
     * Two hosts run two tasks each. We expect the following vertical positions to be set:
     *  - task1: 0
     *  - task2: 5
     *  - task3: 0
     *  - task4: 1
     */
    t1->setStartDate(1.0);
    t1->setEndDate(2.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresAllocated(5);

    t2->setStartDate(1.0);
    t2->setEndDate(2.0);
    t2->setExecutionHost("host1");
    t2->setNumCoresAllocated(5);

    t3->setStartDate(1.0);
    t3->setEndDate(2.0);
    t3->setExecutionHost("host2");
    t3->setNumCoresAllocated(1);

    t4->setStartDate(1.0);
    t4->setEndDate(2.0);
    t4->setExecutionHost("host2");
    t4->setNumCoresAllocated(1);


    nlohmann::json expected_json3 = R"(
    {
        "task1": 0,
        "task2": 5,
        "task3": 0,
        "task4": 1
    }
    )"_json;


    EXPECT_NO_THROW(simulation->getOutput().dumpWorkflowExecutionJSON(workflow, execution_data_json_file_path, true));

    json_file = std::ifstream("host_utilization_layout.json");
    nlohmann::json result_json3;
    json_file >> result_json3;


    EXPECT_TRUE(compareObjects(result_json3,expected_json3));

    workflow->clear();

    workflow = wrench::Workflow::createWorkflow();

    t1 = workflow->addTask("task1", 1, 1, 1, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 0);

    /*
     * An execution that has no layout because we were possibly oversubscribed. std::runtime_error should be thrown.
     */
    t1->setStartDate(1.0);
    t1->setEndDate(2.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresAllocated(10);

    t2->setStartDate(1.0);
    t2->setEndDate(2.0);
    t2->setExecutionHost("host1");
    t2->setNumCoresAllocated(10);

    //EXPECT_THROW(simulation->getOutput().dumpWorkflowExecutionJSON(workflow, execution_data_json_file_path, true), std::runtime_error);

    workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}


TEST_F(SimulationDumpJSONTest, SimulationSearchForHostUtilizationGraphLayoutTest) {
    DO_TEST_WITH_FORK(do_SimulationSearchForHostUtilizationGraphLayout_test);
}



/**********************************************************************/
/**         SimulationDumpWorkflowGraphJSONTest                      **/
/**********************************************************************/

void SimulationDumpJSONTest::do_SimulationDumpWorkflowGraphJSON_test() {
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    EXPECT_THROW(simulation->getOutput().dumpWorkflowGraphJSON(nullptr, UNIQUE_TMP_PATH_PREFIX + "file.json"), std::invalid_argument);

    std::ifstream graph_json_file;

    // Generate a workflow with two independent tasks. Both tasks each have one input file and one output file.
    auto independent_tasks_workflow = wrench::Workflow::createWorkflow();

    t1 = independent_tasks_workflow->addTask("task1", 1.0, 1, 1, 0);
    t1->addInputFile(independent_tasks_workflow->addFile("task1_input", 1.0));
    t1->addOutputFile(independent_tasks_workflow->addFile("task1_output", 1.0));

    t2 = independent_tasks_workflow->addTask("task2", 1.0, 1, 1, 0);
    t2->addInputFile(independent_tasks_workflow->addFile("task2_input", 1.0));
    t2->addOutputFile(independent_tasks_workflow->addFile("task2_output", 1.0));

    EXPECT_NO_THROW(simulation->getOutput().dumpWorkflowGraphJSON(independent_tasks_workflow, workflow_graph_json_file_path));

    nlohmann::json result_json1;
    graph_json_file = std::ifstream(workflow_graph_json_file_path);
    graph_json_file >> result_json1;

    auto expected_json1 = R"(
    {
        "workflow_graph": {
            "edges": [
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
            "vertices": [
                {
                    "flops": 1.0,
                    "id": "task1",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
                    "type": "task"
                },
                {
                    "flops": 1.0,
                    "id": "task2",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
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
        }
    })"_json;

    /*
     * nlohmann::json doesn't maintain order when you push_back json objects into a vector so, before
     * comparing results with expected values, we sort the json lists so the test is deterministic.
     */
    std::sort(result_json1["workflow_graph"]["edges"].begin(), result_json1["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(result_json1["workflow_graph"]["vertices"].begin(), result_json1["workflow_graph"]["vertices"].end(), compareNodes);

    std::sort(expected_json1["workflow_graph"]["edges"].begin(), expected_json1["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(expected_json1["workflow_graph"]["vertices"].begin(), expected_json1["workflow_graph"]["vertices"].end(), compareNodes);

    EXPECT_TRUE(result_json1 == expected_json1);

    independent_tasks_workflow->clear();

    // Generate a workflow with two tasks, two input files, and four output files. Both tasks use both input files and produce two output files each.
    auto two_tasks_use_all_files_workflow = wrench::Workflow::createWorkflow();

    t1 = two_tasks_use_all_files_workflow->addTask("task1", 1.0, 1, 1,  0);
    t1->addInputFile(two_tasks_use_all_files_workflow->addFile("input_file1", 1));
    t1->addInputFile(two_tasks_use_all_files_workflow->addFile("input_file2", 2));
    t1->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file1", 1));
    t1->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file2", 2));

    t2 = two_tasks_use_all_files_workflow->addTask("task2", 1.0, 1, 1, 0);
    for (auto &file : t1->getInputFiles()) {
        t2->addInputFile(file);
    }

    t2->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file3", 1));
    t2->addOutputFile(two_tasks_use_all_files_workflow->addFile("output_file4", 1));

    EXPECT_NO_THROW(simulation->getOutput().dumpWorkflowGraphJSON(two_tasks_use_all_files_workflow, workflow_graph_json_file_path));

    nlohmann::json result_json2;
    graph_json_file = std::ifstream(workflow_graph_json_file_path);
    graph_json_file >> result_json2;

    auto expected_json2 = R"(
    {
        "workflow_graph": {
            "edges": [
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
            "vertices": [
                {
                    "flops": 1.0,
                    "id": "task1",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
                    "type": "task"
                },
                {
                    "flops": 1.0,
                    "id": "task2",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
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
        }
    })"_json;

    /*
     * nlohmann::json doesn't maintain order when you push_back json objects into a vector so, before
     * comparing results with expected values, we sort the json lists so the test is deterministic.
     */
    std::sort(result_json2["workflow_graph"]["edges"].begin(), result_json2["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(result_json2["workflow_graph"]["vertices"].begin(), result_json2["workflow_graph"]["vertices"].end(), compareNodes);

    std::sort(expected_json2["workflow_graph"]["edges"].begin(), expected_json2["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(expected_json2["workflow_graph"]["vertices"].begin(), expected_json2["workflow_graph"]["vertices"].end(), compareNodes);

    EXPECT_TRUE(result_json2 == expected_json2);

    two_tasks_use_all_files_workflow->clear();

    // Generate a workflow where one task1 forks into two tasks, then those two tasks join into one.
    auto fork_join_workflow = wrench::Workflow::createWorkflow();

    t1 = fork_join_workflow->addTask("task1", 1.0, 1, 1, 0);
    t1->addInputFile(fork_join_workflow->addFile("task1_input", 1.0));
    t1->addOutputFile(fork_join_workflow->addFile("task1_output1", 1.0));
    t1->addOutputFile(fork_join_workflow->addFile("task1_output2", 1.0));

    t2 = fork_join_workflow->addTask("task2", 1.0, 1, 1, 0);
    t2->addInputFile(fork_join_workflow->getFileByID("task1_output1"));
    t2->addOutputFile(fork_join_workflow->addFile("task2_output1", 1.0));
    fork_join_workflow->addControlDependency(t1, t2);

    t3 = fork_join_workflow->addTask("task3", 1.0, 1, 1, 0);
    t3->addInputFile(fork_join_workflow->getFileByID("task1_output2"));
    t3->addOutputFile(fork_join_workflow->addFile("task3_output1", 1.0));
    fork_join_workflow->addControlDependency(t1, t3);

    t4 = fork_join_workflow->addTask("task4", 1.0, 1, 1, 0);
    t4->addInputFile(fork_join_workflow->getFileByID("task2_output1"));
    t4->addInputFile(fork_join_workflow->getFileByID("task3_output1"));
    t4->addOutputFile(fork_join_workflow->addFile("task4_output1", 1.0));
    fork_join_workflow->addControlDependency(t2, t4);
    fork_join_workflow->addControlDependency(t3, t4);

    EXPECT_NO_THROW(simulation->getOutput().dumpWorkflowGraphJSON(fork_join_workflow, workflow_graph_json_file_path));

    nlohmann::json result_json3;
    graph_json_file = std::ifstream(workflow_graph_json_file_path);

    graph_json_file >> result_json3;

    auto expected_json3 = R"(
    {
        "workflow_graph": {
            "edges": [
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
            "vertices": [
                {
                    "flops": 1.0,
                    "id": "task1",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
                    "type": "task"
                },
                {
                    "flops": 1.0,
                    "id": "task2",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
                    "type": "task"
                },
                {
                    "flops": 1.0,
                    "id": "task3",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
                    "type": "task"
                },
                {
                    "flops": 1.0,
                    "id": "task4",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
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
        }
    })"_json;

    /*
     * nlohmann::json doesn't maintain order when you push_back json objects into a vector so, before
     * comparing results with expected values, we sort the json lists so the test is deterministic.
     */
    std::sort(result_json3["workflow_graph"]["edges"].begin(), result_json3["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(result_json3["workflow_graph"]["vertices"].begin(), result_json3["workflow_graph"]["vertices"].end(), compareNodes);

    std::sort(expected_json3["workflow_graph"]["edges"].begin(), expected_json3["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(expected_json3["workflow_graph"]["vertices"].begin(), expected_json3["workflow_graph"]["vertices"].end(), compareNodes);

    EXPECT_TRUE(result_json3 == expected_json3);

    fork_join_workflow->clear();

}

TEST_F(SimulationDumpJSONTest, SimulationDumpWorkflowGraphJSONTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpWorkflowGraphJSON_test);
}

/**********************************************************************/
/**         SimulationDumpHostEnergyConsumptionJSONTest              **/
/**********************************************************************/
class SimulationOutputDumpEnergyConsumptionTestWMS : public wrench::ExecutionController {
public:
    SimulationOutputDumpEnergyConsumptionTestWMS(SimulationDumpJSONTest *test,
                                                 std::string &hostname) :
            wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationDumpJSONTest *test;

    int main() {

        // default pstate is set to 1, change it back to 0
        // at time 0.0, the pstate is set to 1 as specified in the platform file,
        // then again at time 0.0, it is set to pstate 0 and only one timestamp should
        // be generated for this host at time 0.0
        this->simulation->setPstate(this->getHostname(), 0);

        const std::vector<std::string> hostnames = wrench::Simulation::getHostnameList();
        const double TWO_SECOND_PERIOD = 2.0;

        auto em = this->createEnergyMeter(hostnames, TWO_SECOND_PERIOD);

        const double MEGAFLOP = 1000.0 * 1000.0;
        wrench::S4U_Simulation::compute(6.0 * 100.0 * MEGAFLOP); // compute for 6 seconds


        return 0;
    }

};

TEST_F(SimulationDumpJSONTest, SimulationDumpEnergyConsumptionTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpHostEnergyConsumptionJSON_test);
}

// some comparison functions to be used when sorting lists of JSON objects so that the tests are deterministic
bool comparePstate(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return lhs["pstate"] < rhs["pstate"];
}

bool compareTime(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return lhs["time"] < rhs["time"];
}

bool compareHostname(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return lhs["hostname"] < rhs["hostname"];
}

void SimulationDumpJSONTest::do_SimulationDumpHostEnergyConsumptionJSON_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **)calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-energy-simulation");
//    argv[2] = strdup("--wrench-full-log");

    workflow = wrench::Workflow::createWorkflow();

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path2));

    // get the single host
    std::string host = wrench::Simulation::getHostnameList()[0];

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new SimulationOutputDumpEnergyConsumptionTestWMS(
                    this, host
            )
    ));

    EXPECT_NO_THROW(simulation->launch());

    EXPECT_THROW(simulation->getOutput().dumpHostEnergyConsumptionJSON(""), std::invalid_argument);

    ASSERT_NO_THROW(simulation->getOutput().dumpHostEnergyConsumptionJSON(this->energy_consumption_data_file_path));

//    simulation->getOutput().dumpUnifiedJSON(workflow, "/tmp/energy_unified.json", false, true, false, true, false, false, false);

    nlohmann::json expected_json = R"(
    {
        "energy_consumption": [
            {
                "consumed_energy_trace": [
                    {
                        "time": 0.0,
                        "joules": 0.0
                    },
                    {
                        "time": 2.0,
                        "joules": 400.0
                    },
                    {
                        "time": 4.0,
                        "joules": 800.0
                    },
                    {
                        "time": 6.0,
                        "joules": 1200.0
                    }
                ],
                "hostname": "host1",
                "pstate_trace": [
                    {
                        "pstate": 0,
                        "time": 0.0
                    }
                ],
                "pstates": [
                    {
                        "idle": "100.0",
                        "pstate": 0,
                        "epsilon": "100.0",
                        "all_cores": "200.0",
                        "speed": 100000000.0
                    },
                    {
                        "idle": " 93.0",
                        "pstate": 1,
                        "epsilon": " 93.0",
                        "all_cores": "170.0",
                        "speed": 50000000.0
                    },
                    {
                        "idle": " 90.0",
                        "epsilon": " 90.0",
                        "pstate": 2,
                        "all_cores": "150.0",
                        "speed": 20000000.0
                    }
                ],
                "wattage_off": "10"
            },
            {
                "consumed_energy_trace": [
                    {
                        "time": 0.0,
                        "joules": 0.0
                    },
                    {
                        "time": 2.0,
                        "joules": 200.0
                    },
                    {
                        "time": 4.0,
                        "joules": 400.0
                    }
                ],
                "hostname": "host2",
                "pstate_trace": [
                    {
                        "pstate": 0,
                        "time": 0.0
                    }
                ],
                "pstates": [
                    {
                        "idle": "100.0",
                        "epsilon": "100.0",
                        "pstate": 0,
                        "all_cores": "200.0",
                        "speed": 100000000.0
                    },
                    {
                        "idle": " 93.0",
                        "epsilon": " 93.0",
                        "pstate": 1,
                        "all_cores": "170.0",
                        "speed": 50000000.0
                    },
                    {
                        "idle": " 90.0",
                        "epsilon": " 90.0",
                        "pstate": 2,
                        "all_cores": "150.0",
                        "speed": 20000000.0
                    }
                ],
                "wattage_off": "10"
            }
        ]
    })"_json;

    std::sort(expected_json["energy_consumption"].begin(), expected_json["energy_consumption"].end(), compareHostname);
    for (size_t i = 0; i < expected_json["energy_consumption"].size(); ++i) {
        std::sort(expected_json["energy_consumption"][i]["consumed_energy_trace"].begin(), expected_json["energy_consumption"][i]["consumed_energy_trace"].end(), compareTime);
        std::sort(expected_json["energy_consumption"][i]["pstates"].begin(), expected_json["energy_consumption"][i]["pstates"].end(), comparePstate);
        std::sort(expected_json["energy_consumption"][i]["pstate_trace"].begin(), expected_json["energy_consumption"][i]["pstate_trace"].end(), comparePstate);
    }

    std::ifstream json_file(this->energy_consumption_data_file_path);
    nlohmann::json result_json;
    json_file >> result_json;


    std::sort(result_json["energy_consumption"].begin(), result_json["energy_consumption"].end(), compareHostname);
    for (size_t i = 0; i < expected_json["energy_consumption"].size(); ++i) {
        std::sort(result_json["energy_consumption"][i]["consumed_energy_trace"].begin(), result_json["energy_consumption"][i]["consumed_energy_trace"].end(), compareTime);
        std::sort(result_json["energy_consumption"][i]["pstates"].begin(), result_json["energy_consumption"][i]["pstates"].end(), comparePstate);
        std::sort(result_json["energy_consumption"][i]["pstate_trace"].begin(), result_json["energy_consumption"][i]["pstate_trace"].end(), comparePstate);
    }

    EXPECT_TRUE(expected_json == result_json);


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

// some comparison functions to be used when sorting lists of JSON objects so that the tests are deterministic
bool compareEdges(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return (lhs["source"]["type"].get<std::string>() + ":" + lhs["source"]["id"].get<std::string>() + "-" + lhs["target"]["type"].get<std::string>() + ":" + lhs["target"]["id"].get<std::string>()) <
           (rhs["source"]["type"].get<std::string>() + ":" + rhs["source"]["id"].get<std::string>() + "-" + rhs["target"]["type"].get<std::string>() + ":" + rhs["target"]["id"].get<std::string>());
}

bool compareRoutes(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return (lhs["source"].get<std::string>() + "-" + lhs["target"].get<std::string>()) <
           (rhs["source"].get<std::string>() + "-" + rhs["target"].get<std::string>());
}

/**********************************************************************/
/**               SimulationDumpLinkUsageJSONTest                    **/
/**********************************************************************/
class SimulationOutputDumpLinkUsageTestWMS : public wrench::ExecutionController {
public:
    SimulationOutputDumpLinkUsageTestWMS(SimulationDumpJSONTest *test,
                                         std::string &hostname) :
            wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationDumpJSONTest *test;

    int main() {
        //creating the bandwidth meter service
        const std::vector<std::string> linknames = wrench::Simulation::getLinknameList();
        const double TWO_SECOND_PERIOD = 2.0;
        auto em = this->createBandwidthMeter(linknames, TWO_SECOND_PERIOD);


        //Setting up storage services to accommodate data transfer.
        auto data_manager = this->createDataMovementManager();
        //copying file to force link usage.
        auto file = *(this->test->workflow->getFileMap().begin());
        data_manager->doSynchronousFileCopy(file.second,
                                            wrench::FileLocation::LOCATION(this->test->client_storage_service),
                                            wrench::FileLocation::LOCATION(this->test->server_storage_service));
        return 0;
    }
};

TEST_F(SimulationDumpJSONTest, SimulationDumpLinkUsageTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpLinkUsageJSON_test);
}

bool compareLinkname(const nlohmann::json &lhs, const nlohmann::json &rhs) {
    return lhs["linkname"] < rhs["linkname"];
}

void SimulationDumpJSONTest::do_SimulationDumpLinkUsageJSON_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **)calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));

    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path5));

    // get the single host
    std::string host = wrench::Simulation::getHostnameList()[0];
    std::set<std::shared_ptr<wrench::StorageService>> storage_services_list;
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;

    client_storage_service = simulation->add(new wrench::SimpleStorageService("host1", {"/"}, {}));
    server_storage_service = simulation->add(new wrench::SimpleStorageService("host2", {"/"}, {}));
    storage_services_list.insert(client_storage_service);
    storage_services_list.insert(server_storage_service);

    const double GB = 1000.0 * 1000.0 * 1000.0;
    //std::shared_ptr<wrench::DataFile> file = new wrench::DataFile("test_file", 10*GB);
    auto link_usage_workflow = wrench::Workflow::createWorkflow();
    std::shared_ptr<wrench::WorkflowTask> single_task;
    single_task = link_usage_workflow->addTask("dummy_task",1,1,1,8*GB);
    single_task->addInputFile(link_usage_workflow->addFile("test_file", 10*GB));

    EXPECT_NO_THROW(wms = simulation->add(
            new SimulationOutputDumpLinkUsageTestWMS(
                    this,
                    host)
    ));

    simulation->add(new wrench::FileRegistryService("host1"));
    for (auto const &file : link_usage_workflow->getInputFiles()) {
        simulation->stageFile(file, client_storage_service);
    }

    EXPECT_NO_THROW(simulation->launch());

    EXPECT_THROW(simulation->getOutput().dumpLinkUsageJSON(""), std::invalid_argument);

    EXPECT_NO_THROW(simulation->getOutput().dumpLinkUsageJSON(this->link_usage_json_file_path));
    simulation->getOutput().dumpUnifiedJSON(link_usage_workflow, "/tmp/linkusage_unified.json", false, true, true, false, false, false, true);

    nlohmann::json expected_json_link_usage = R"(
    {
        "link_usage": {
            "links": [
                {
                    "link_usage_trace": [
                        {
                            "bytes per second": 0.0,
                            "time": 0.0
                        },
                        {
                            "bytes per second": 125000000.00000001,
                            "time": 2.0
                        },
                        {
                            "bytes per second": 125000000.00000001,
                            "time": 86.0
                        }
                    ],
                    "linkname": "1"
                },
                {
                    "link_usage_trace": [
                        {
                            "bytes per second": 0.0,
                            "time": 0.0
                        },
                        {
                            "bytes per second": 0.0,
                            "time": 86.0
                        }
                    ],
                    "linkname": "__loopback__"
                }
            ]
        }
    }
    )"_json;


    std::ifstream json_file(link_usage_json_file_path);
    nlohmann::json result_json;
    json_file >> result_json;

    EXPECT_TRUE(expected_json_link_usage == result_json);

    link_usage_workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}




/**********************************************************************/
/**            SimulationDumpDiskOperationsJSONTest                  **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampDiskReadWrite class.
 * This test ensures that SimulationTimestampDiskReadWriteStart, SimulationTimestampDiskReadWriteFailure,
 * and SimulationTimestampDiskReadWriteCompletion objects are added to their respective simulation
 * traces at the appropriate times.
 */
class SimulationDumpDiskOperationsTestWMS : public wrench::ExecutionController {
public:
    SimulationDumpDiskOperationsTestWMS(SimulationDumpJSONTest *test,
                                        std::string &hostname) :
            wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationDumpJSONTest *test;

    int main() {


        auto file_1 = this->test->workflow->addFile("file_1", 1.00*1000*1000);

        wrench::StorageService::writeFile(file_1, wrench::FileLocation::LOCATION(this->test->ss1));
        wrench::StorageService::writeFile(file_1, wrench::FileLocation::LOCATION(this->test->ss2));
        wrench::StorageService::readFile(file_1, wrench::FileLocation::LOCATION(this->test->ss1));
        return 0;

    }
};

TEST_F(SimulationDumpJSONTest, SimulationDumpDiskOperationsTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpDiskOperationsJSON_test);
}

void SimulationDumpJSONTest::do_SimulationDumpDiskOperationsJSON_test(){
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path6));

    std::string host1 = "host1";
    std::string host2 = "host2";

    ASSERT_NO_THROW(ss1 = simulation->add(new wrench::SimpleStorageService(host1, {"/"},
                                                                           {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "400000"}})));

    ASSERT_NO_THROW(ss2 = simulation->add(new wrench::SimpleStorageService(host2, {"/"},
                                                                           {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "infinity"}})));

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationDumpDiskOperationsTestWMS(
            this, host1
    )));

    simulation->getOutput().enableDiskTimestamps(true);

    ASSERT_NO_THROW(simulation->launch());

    EXPECT_THROW(simulation->getOutput().dumpDiskOperationsJSON(""), std::invalid_argument);

    EXPECT_NO_THROW(simulation->getOutput().dumpDiskOperationsJSON(this->disk_operations_json_file_path));
    simulation->getOutput().dumpUnifiedJSON(workflow, "/tmp/disk_unified.json", false, true, true, false, false, true, false);

    // Performing programmatic checks of the JSON output
    std::ifstream json_file(disk_operations_json_file_path);
    nlohmann::json result_json;
    json_file >> result_json;

    for (auto const &operation : (std::vector<std::string>){"reads"}) {
        ASSERT_EQ(result_json["host1"]["/"][operation].size(), 3);

        for (int i = 0; i < 3; i++) {
            int num_bytes = (int) result_json["host1"]["/"][operation][i]["bytes"];
            double duration = (double) result_json["host1"]["/"][operation][i]["end"] -
                              (double) result_json["host1"]["/"][operation][i]["start"];
            if (i < 2) {
                ASSERT_EQ(num_bytes, 400000);
                ASSERT_TRUE(std::abs<double>(duration -0.4) < 0.0001);
            } else {
                ASSERT_EQ(num_bytes, 200000);
                ASSERT_TRUE(std::abs<double>(duration -0.2) < 0.0001);
            }
        }
    }

    for (auto const &operation : (std::vector<std::string>){"writes"}) {
        ASSERT_EQ(result_json["host2"]["/"][operation].size(), 1);

        int num_bytes = (int) result_json["host2"]["/"][operation][0]["bytes"];
        double duration = (double) result_json["host2"]["/"][operation][0]["end"] -
                          (double) result_json["host2"]["/"][operation][0]["start"];
        ASSERT_EQ(num_bytes, 1000000);
        ASSERT_TRUE(std::abs<double>(duration - 0.5) < 0.0001);

    }

    workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**         SimulationDumpPlatformGraphJSONTest                      **/
/**********************************************************************/

TEST_F(SimulationDumpJSONTest, SimulationDumpPlatformGraphJSONTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpPlatformGraphJSON_test);
}

void SimulationDumpJSONTest::do_SimulationDumpPlatformGraphJSON_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **)calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path3));

    EXPECT_THROW(simulation->getOutput().dumpPlatformGraphJSON(""), std::invalid_argument);
    EXPECT_NO_THROW(simulation->getOutput().dumpPlatformGraphJSON(this->platform_graph_json_file_path));

    nlohmann::json expected_json = R"(
        {
            "platform": {
                "edges": [
                    {
                        "source": {
                            "id": "host1",
                                    "type": "host"
                        },
                        "target": {
                            "id": "1",
                                    "type": "link"
                        }
                    },
                    {
                        "source": {
                            "id": "1",
                                    "type": "link"
                        },
                        "target": {
                            "id": "host2",
                                    "type": "host"
                        }
                    },
                    {
                        "source": {
                            "id": "1",
                                    "type": "link"
                        },
                        "target": {
                            "id": "2",
                                    "type": "link"
                        }
                    },
                    {
                        "source": {
                            "id": "2",
                                    "type": "link"
                        },
                        "target": {
                            "id": "host3",
                                    "type": "host"
                        }
                    },
                    {
                        "source": {
                            "id": "host3",
                                    "type": "host"
                        },
                        "target": {
                            "id": "3",
                                    "type": "link"
                        }
                    },
                    {
                        "source": {
                            "id": "3",
                                    "type": "link"
                        },
                        "target": {
                            "id": "host1",
                                    "type": "host"
                        }
                    },
                    {
                        "source": {
                            "id": "host2",
                                    "type": "host"
                        },
                        "target": {
                            "id": "2",
                                    "type": "link"
                        }
                    }
                ],
                "vertices": [
                    {
                                "cluster_id": "host1",
                                "cores": 10,
                                "flop_rate": 1.0,
                                "id": "host1",
                                "memory_manager_service": 10.0,
                                "type": "host"
                    },
                    {
                                "cluster_id": "host2",
                                "cores": 20,
                                "flop_rate": 1.0,
                                "id": "host2",
                                "memory_manager_service": 20.0,
                                "type": "host"
                    },
                    {
                                "cluster_id": "host3",
                                "cores": 20,
                                "flop_rate": 1.0,
                                "id": "host3",
                                "memory_manager_service": 20.0,
                                "type": "host"
                    },
                    {
                        "bandwidth": 125000000.0,
                                "id": "1",
                                "latency": 1e-06,
                                "type": "link"
                    },
                    {
                        "bandwidth": 125000000.0,
                                "id": "2",
                                "latency": 1e-06,
                                "type": "link"
                    },
                    {
                        "bandwidth": 125000000.0,
                                "id": "3",
                                "latency": 1e-06,
                                "type": "link"
                    }
                ],
                "routes": [
                    {
                        "latency": 1e-06,
                                "route": [
                        "1"
                        ],
                        "source": "host1",
                                "target": "host2"
                    },
                    {
                        "latency": 2e-06,
                                "route": [
                        "1",
                                "2"
                        ],
                        "source": "host1",
                                "target": "host3"
                    },
                    {
                        "latency": 1e-06,
                                "route": [
                        "3"
                        ],
                        "source": "host3",
                                "target": "host1"
                    },
                    {
                        "latency": 1e-06,
                                "route": [
                        "2"
                        ],
                        "source": "host2",
                        "target": "host3"
                    }
                ]
            }
        }
    )"_json;

    std::ifstream json_file = std::ifstream(platform_graph_json_file_path);
    nlohmann::json result_json;
    json_file >> result_json;

    // sort the links (edges)
    std::sort(result_json["platform"]["edges"].begin(), result_json["platform"]["edges"].end(), compareEdges);
    std::sort(expected_json["platform"]["edges"].begin(), expected_json["platform"]["edges"].end(), compareEdges);

    // sort the nodes
    std::sort(result_json["platform"]["vertices"].begin(), result_json["platform"]["vertices"].end(), compareNodes);
    std::sort(expected_json["platform"]["vertices"].begin(), expected_json["platform"]["vertices"].end(), compareNodes);

    // sort the routes
    std::sort(result_json["platform"]["routes"].begin(), result_json["platform"]["routes"].end(), compareRoutes);
    std::sort(expected_json["platform"]["routes"].begin(), expected_json["platform"]["routes"].end(), compareRoutes);


    EXPECT_TRUE(result_json == expected_json);


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**         SimulationDumpPlatformGraphJSONTest: Broken Routing      **/
/**********************************************************************/

TEST_F(SimulationDumpJSONTest, SimulationDumpPlatformGraphJSONWithBrokenRoutingTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpPlatformGraphJSONBrokenRouting_test);
}

void SimulationDumpJSONTest::do_SimulationDumpPlatformGraphJSONBrokenRouting_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    EXPECT_NO_THROW(simulation->init(&argc, argv));
    EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path3_broken));

    EXPECT_THROW(simulation->getOutput().dumpPlatformGraphJSON(this->platform_graph_json_file_path), std::invalid_argument);
}


/**********************************************************************/
/**         SimulationDumpUnifiedJSONTest                            **/
/**********************************************************************/

TEST_F(SimulationDumpJSONTest, SimulationDumpUnifiedJSONTest) {
    DO_TEST_WITH_FORK(do_SimulationDumpUnifiedJSON_test);
}

void SimulationDumpJSONTest::do_SimulationDumpUnifiedJSON_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-energy-simulation");
//    argv[2] = strdup("--wrench-full-logs");

    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);

    workflow = wrench::Workflow::createWorkflow();

    t1 = workflow->addTask("task1", 1, 1, 1, 0);
    t2 = workflow->addTask("task2", 1, 1, 1, 0);

    t1->setStartDate(1.0);
    t1->setEndDate(2.0);
    t1->setExecutionHost("host1");
    t1->setNumCoresAllocated(8);

    t1->setStartDate(2.0);
    t1->setEndDate(3.0);
    t1->setExecutionHost("host2");
    t1->setNumCoresAllocated(10);

    t2->setStartDate(3.0);
    t2->setEndDate(4.0);
    t2->setExecutionHost("host2");
    t2->setNumCoresAllocated(20);

    nlohmann::json expected_json5 = R"(
    {
        "platform": {
            "edges": [
                {
                    "source": {
                        "id": "host1",
                        "type": "host"
                    },
                    "target": {
                        "id": "1",
                        "type": "link"
                    }
                },
                {
                    "source": {
                        "id": "1",
                        "type": "link"
                    },
                    "target": {
                        "id": "host2",
                        "type": "host"
                    }
                }
            ],
            "routes": [
                {
                    "latency": 1e-06,
                    "route": [
                        "1"
                    ],
                    "source": "host1",
                    "target": "host2"
                }
            ],
            "vertices": [
                {
                    "cluster_id": "host1",
                    "cores": 10,
                    "flop_rate": 1.0,
                    "id": "host1",
                    "memory_manager_service": 10.0,
                    "type": "host"
                },
                {
                    "cluster_id": "host2",
                    "cores": 20,
                    "flop_rate": 1.0,
                    "id": "host2",
                    "memory_manager_service": 20.0,
                    "type": "host"
                },
                {
                    "bandwidth": 125000000.0,
                    "id": "1",
                    "latency": 1e-06,
                    "type": "link"
                }
            ]
        },
        "workflow_execution": {
            "tasks": [
                {
                    "compute": {
                        "end": -1.0,
                        "start": -1.0
                    },
                    "execution_host": {
                        "cores": 20,
                        "flop_rate": 1.0,
                        "hostname": "host2",
                        "memory_manager_service": 20.0
                    },
                    "failed": -1.0,
                    "num_cores_allocated": 10,
                    "read": null,
                    "task_id": "task1",
                    "color": "",
                    "terminated": -1.0,
                    "whole_task": {
                        "end": 3.0,
                        "start": 2.0
                    },
                    "write": null
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
                        "memory_manager_service": 10.0
                    },
                    "failed": -1.0,
                    "num_cores_allocated": 8,
                    "read": null,
                    "task_id": "task1",
                    "color": "",
                    "terminated": -1.0,
                    "whole_task": {
                        "end": 2.0,
                        "start": 1.0
                    },
                    "write": null
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
                        "memory_manager_service": 20.0
                    },
                    "failed": -1.0,
                    "num_cores_allocated": 20,
                    "read": null,
                    "task_id": "task2",
                    "color": "",
                    "terminated": -1.0,
                    "whole_task": {
                        "end": 4.0,
                        "start": 3.0
                    },
                    "write": null
                }
            ]
        },
        "workflow_graph": {
            "edges": null,
            "vertices": [
                {
                    "flops": 1.0,
                    "id": "task1",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
                    "type": "task"
                },
                {
                    "flops": 1.0,
                    "id": "task2",
                    "max_cores": 1,
                    "memory_manager_service": 0.0,
                    "min_cores": 1,
                    "type": "task"
                }
            ]
        }
    }
    )"_json;

    EXPECT_NO_THROW(simulation->getOutput().dumpUnifiedJSON(workflow, unified_json_file_path, true, true, true, false, false));

    std::ifstream json_file = std::ifstream(unified_json_file_path);
    nlohmann::json result_json;
    json_file >> result_json;

    std::sort(result_json["platform"]["edges"].begin(), result_json["platform"]["edges"].end(), compareEdges);
    std::sort(expected_json5["platform"]["edges"].begin(), expected_json5["platform"]["edges"].end(), compareEdges);

    std::sort(result_json["platform"]["vertices"].begin(), result_json["platform"]["vertices"].end(), compareNodes);
    std::sort(expected_json5["platform"]["vertices"].begin(), expected_json5["platform"]["vertices"].end(), compareNodes);

    std::sort(result_json["platform"]["routes"].begin(), result_json["platform"]["routes"].end(), compareRoutes);
    std::sort(expected_json5["platform"]["routes"].begin(), expected_json5["platform"]["routes"].end(), compareRoutes);

    std::sort(result_json["workflow_execution"]["tasks"].begin(), result_json["workflow_execution"]["tasks"].end(), compareStartTimes);
    std::sort(expected_json5["workflow_execution"]["tasks"].begin(), expected_json5["workflow_execution"]["tasks"].end(), compareStartTimes);

    std::sort(result_json["workflow_graph"]["edges"].begin(), result_json["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(result_json["workflow_graph"]["vertices"].begin(), result_json["workflow_graph"]["vertices"].end(), compareNodes);

    std::sort(expected_json5["workflow_graph"]["edges"].begin(), expected_json5["workflow_graph"]["edges"].end(), compareLinks);
    std::sort(expected_json5["workflow_graph"]["vertices"].begin(), expected_json5["workflow_graph"]["vertices"].end(), compareNodes);

    EXPECT_TRUE(result_json == expected_json5);


    /*
    workflow = wrench::Workflow::createWorkflow();


    std::string host = wrench::Simulation::getHostnameList()[0];

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    EXPECT_NO_THROW(wms = simulation->add(
            new SimulationOutputDumpEnergyConsumptionTestWMS(
                    this, workflow, host
            )
    ));

    EXPECT_NO_THROW(simulation->launch());

    EXPECT_NO_THROW(simulation->getOutput().dumpUnifiedJSON(workflow, unified_json_file_path, false, false, false, true, false));

    nlohmann::json expected_json6 = R"(
    {
        "energy_consumption": [
            {
                "consumed_energy_trace": [
                    {
                        "time": 0.0,
                        "joules": 0.0
                    },
                    {
                        "time": 2.0,
                        "joules": 400.0
                    },
                    {
                        "time": 4.0,
                        "joules": 800.0
                    },
                    {
                        "time": 6.0,
                        "joules": 1200.0
                    }
                ],
                "hostname": "host1",
                "pstate_trace": [
                    {
                        "pstate": 0,
                        "time": 0.0
                    }
                ],
                "pstates": [
                    {
                        "idle": "100.0",
                        "epsilon": "100.0",
                        "pstate": 0,
                        "all_cores": "200.0",
                        "speed": 100000000.0
                    },
                    {
                        "idle": " 93.0",
                        "epsilon": " 93.0",
                        "pstate": 1,
                        "all_cores": "170.0",
                        "speed": 50000000.0
                    },
                    {
                        "idle": " 90.0",
                        "epsilon": " 90.0",
                        "pstate": 2,
                        "all_cores": "150.0",
                        "speed": 20000000.0
                    }
                ],
                "wattage_off": "10"
            },
            {
                "consumed_energy_trace": [
                    {
                        "time": 0.0,
                        "joules": 0.0
                    },
                    {
                        "time": 2.0,
                        "joules": 200.0
                    },
                    {
                        "time": 4.0,
                        "joules": 400.0
                    }
                ],
                "hostname": "host2",
                "pstate_trace": [
                    {
                        "pstate": 0,
                        "time": 0.0
                    }
                ],
                "pstates": [
                    {
                        "idle": "100.0",
                        "epsilon": "100.0",
                        "pstate": 0,
                        "all_cores": "200.0",
                        "speed": 100000000.0
                    },
                    {
                        "idle": " 93.0",
                        "epsilon": " 93.0",
                        "pstate": 1,
                        "all_cores": "170.0",
                        "speed": 50000000.0
                    },
                    {
                        "idle": " 90.0",
                        "epsilon": " 90.0",
                        "pstate": 2,
                        "all_cores": "150.0",
                        "speed": 20000000.0
                    }
                ],
                "wattage_off": "10"
            }
        ]
    })"_json;

    std::sort(expected_json6["energy_consumption"].begin(), expected_json6["energy_consumption"].end(), compareHostname);
    for (size_t i = 0; i < expected_json6["energy_consumption"].size(); ++i) {
        std::sort(expected_json6["energy_consumption"][i]["consumed_energy_trace"].begin(), expected_json6["energy_consumption"][i]["consumed_energy_trace"].end(), compareTime);
        std::sort(expected_json6["energy_consumption"][i]["pstates"].begin(), expected_json6["energy_consumption"][i]["pstates"].end(), comparePstate);
        std::sort(expected_json6["energy_consumption"][i]["pstate_trace"].begin(), expected_json6["energy_consumption"][i]["pstate_trace"].end(), comparePstate);
    }

    std::ifstream json1_file(unified_json_file_path);
    nlohmann::json result_json2;
    json1_file >> result_json2;


    std::sort(result_json2["energy_consumption"].begin(), result_json2["energy_consumption"].end(), compareHostname);
    for (size_t i = 0; i < expected_json6["energy_consumption"].size(); ++i) {
        std::sort(result_json2["energy_consumption"][i]["consumed_energy_trace"].begin(), result_json2["energy_consumption"][i]["consumed_energy_trace"].end(), compareTime);
        std::sort(result_json2["energy_consumption"][i]["pstates"].begin(), result_json2["energy_consumption"][i]["pstates"].end(), comparePstate);
        std::sort(result_json2["energy_consumption"][i]["pstate_trace"].begin(), result_json2["energy_consumption"][i]["pstate_trace"].end(), comparePstate);
    }

    EXPECT_TRUE(expected_json6 == result_json2);
    */


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}