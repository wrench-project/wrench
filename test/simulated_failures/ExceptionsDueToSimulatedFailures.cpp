/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class ExceptionsDueToSimulatedFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowFile *output_file4;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;
    wrench::WorkflowTask *task5;
    wrench::WorkflowTask *task6;
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service = nullptr;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    void do_HostFailureDuringSleep_test();

protected:

    ExceptionsDueToSimulatedFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create the files
//        input_file = workflow->addFile("input_file", 10.0);
//        output_file1 = workflow->addFile("output_file1", 10.0);
//        output_file2 = workflow->addFile("output_file2", 10.0);
//        output_file3 = workflow->addFile("output_file3", 10.0);
//        output_file4 = workflow->addFile("output_file4", 10.0);
//
//        // Create the tasks
//        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0, 0);
//        task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0, 0);
//        task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 1.0, 0);
//        task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 1.0, 0);
//        task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 1.0, 0);
//        task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 1.0, 0);
//        task1->setClusterID("ID1");
//        task2->setClusterID("ID1");
//        task3->setClusterID("ID1");
//        task4->setClusterID("ID2");
//        task5->setClusterID("ID2");
//
//        // Add file-task dependencies
//        task1->addInputFile(input_file);
//        task2->addInputFile(input_file);
//        task3->addInputFile(input_file);
//        task4->addInputFile(input_file);
//        task5->addInputFile(input_file);
//        task6->addInputFile(input_file);
//
//        task1->addOutputFile(output_file1);
//        task2->addOutputFile(output_file2);
//        task3->addOutputFile(output_file3);
//        task4->addOutputFile(output_file4);
//        task5->addOutputFile(output_file3);
//        task6->addOutputFile(output_file4);
//
//        workflow->addControlDependency(task4, task5);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"/> "
                          "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"/> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**                    HOST FAILURE DURING A SLEEP                   **/
/**********************************************************************/

class HostFailureDuringSleepWMS : public wrench::WMS {

public:
    HostFailureDuringSleepWMS(ExceptionsDueToSimulatedFailuresTest *test,
                                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ExceptionsDueToSimulatedFailuresTest *test;

    int main() {

        wrench::Simulation::sleep(10);

        return 0;
    }
};

#if ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 21) && (SIMGRID_VERSION_PATCH > 0))
TEST_F(ExceptionsDueToSimulatedFailuresTest, HostFailureDuringSleep) {
#else
TEST_F(ExceptionsDueToSimulatedFailuresTest, DISABLED_HostFailureDuringSleep) {
#endif
    DO_TEST_WITH_FORK(do_HostFailureDuringSleep_test);
}

void ExceptionsDueToSimulatedFailuresTest::do_HostFailureDuringSleep_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "DualCoreHost";


    // Create a WMS
    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new HostFailureDuringSleepWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


