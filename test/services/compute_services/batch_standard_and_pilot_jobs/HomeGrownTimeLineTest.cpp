/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <gtest/gtest.h>
#include "../src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf/NodeAvailabilityTimeLine.h"
#include "../src/wrench/services/compute/batch/batch_schedulers/homegrown/conservative_bf_core_level/CoreAvailabilityTimeLine.h"

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(availability_timeline_test, "Log category for AvailabilityTimeLineTest");


class BatchServiceAvailabilityTimeLineTest : public ::testing::Test {

public:

    void do_NodeAvailabilityTimeLineTest_test();
    void do_CoreAvailabilityTimeLineTest_test();


protected:

    ~BatchServiceAvailabilityTimeLineTest() {
        workflow->clear();
    }

    BatchServiceAvailabilityTimeLineTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "       </host>  "
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::shared_ptr<wrench::Workflow> workflow;
    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";

};


/**********************************************************************/
/**  NODE AVAILABILITY TIMELINE TEST                                 **/
/**********************************************************************/

class NodeAvailabilityTimelineTestWMS : public wrench::ExecutionController {

public:
    NodeAvailabilityTimelineTestWMS(
            BatchServiceAvailabilityTimeLineTest *test,
            std::string hostname) :
            wrench::ExecutionController(hostname, "test"), test(test) {
    }


private:
    BatchServiceAvailabilityTimeLineTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        auto tl = new wrench::NodeAvailabilityTimeLine(10);

        std::shared_ptr<wrench::CompoundJob> wj1 = job_manager->createCompoundJob("wj1");

        auto bj1 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj1, 1, 10, 5, 1, "who", 0, 0));

        tl->add(0, 10.0, bj1);

        // Note that there is check on the validity of the interval in terms of the number of nodes
//        auto wj2 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});
        std::shared_ptr<wrench::CompoundJob> wj2 = job_manager->createCompoundJob("wj2");
        auto bj2 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 2, 20, 20, 1, "who", 0, 0));
        tl->add(10.0, 30.0, bj2);

        // Can even have weird nonsensical start and end...
//        auto wj3 = std::shared_ptr<wrench::CompoundJob>((wrench::CompoundJob *)(1234), [](void *ptr){});
        std::shared_ptr<wrench::CompoundJob> wj3 = job_manager->createCompoundJob("wj3");
        auto bj3 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 3, 20, 20, 1, "who", 0, 0));
        tl->add(500.0, 30.0, bj3);

        tl->print();
        tl->clear();

        return 0;
    }
};

TEST_F(BatchServiceAvailabilityTimeLineTest, NodeAvailabilityTimeLineTest)
{
    DO_TEST_WITH_FORK(do_NodeAvailabilityTimeLineTest_test);
}

void BatchServiceAvailabilityTimeLineTest::do_NodeAvailabilityTimeLineTest_test() {

    // Create and initialize a simulation
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new NodeAvailabilityTimelineTestWMS(this, hostname)));

    simulation->launch();



    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  CORE AVAILABILITY TIMELINE TEST                                 **/
/**********************************************************************/

class CoreAvailabilityTimelineTestWMS : public wrench::ExecutionController {

public:
    CoreAvailabilityTimelineTestWMS(
            std::string hostname) :
            wrench::ExecutionController(hostname, "test") {
    }


private:


    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        auto tl = new wrench::CoreAvailabilityTimeLine(10, 6);

        auto wj1 = job_manager->createCompoundJob("wj1");

        auto bj1 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj1, 1, 10, 5, 1, "who", 0, 0));

        tl->add(0, 10.0, bj1);

        // Note that there is check on the validity of the interval in terms of the number of nodes
        auto wj2 = job_manager->createCompoundJob("wj2");
        auto bj2 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 2, 20, 20, 1, "who", 0, 0));
        tl->add(10.0, 30.0, bj2);

        // Can even have weird nonsensical start and end...
        auto wj3 = job_manager->createCompoundJob("wj3");
        auto bj3 = std::shared_ptr<wrench::BatchJob>(new wrench::BatchJob(wj2, 3, 20, 20, 1, "who", 0, 0));
        tl->add(500.0, 30.0, bj3);

        tl->print();
        tl->clear();

        return 0;
    }
};

TEST_F(BatchServiceAvailabilityTimeLineTest, CoreAvailabilityTimeLineTest)
{
    DO_TEST_WITH_FORK(do_CoreAvailabilityTimeLineTest_test);
}

void BatchServiceAvailabilityTimeLineTest::do_CoreAvailabilityTimeLineTest_test() {

    // Create and initialize a simulation
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new CoreAvailabilityTimelineTestWMS(hostname)));

    simulation->launch();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
