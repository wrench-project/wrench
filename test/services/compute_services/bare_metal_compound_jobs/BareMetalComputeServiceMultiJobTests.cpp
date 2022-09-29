/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(bare_metal_compute_service_multi_job_test, "Log category for BareMetalComputeServiceMultiJob test");

class BareMetalComputeServiceActionMultiJobTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file;
    std::shared_ptr<wrench::WorkflowTask> task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service3 = nullptr;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service = nullptr;

    void do_DAGOfJobs_test();

protected:
    ~BareMetalComputeServiceActionMultiJobTest() {
        workflow->clear();
    }

    BareMetalComputeServiceActionMultiJobTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"

                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1000B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"100MBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;
};


/**********************************************************************/
/**  DAG OF JOBS TEST                                               **/
/**********************************************************************/

class DAGOfJobsTestWMS : public wrench::ExecutionController {
public:
    DAGOfJobsTestWMS(
            BareMetalComputeServiceActionMultiJobTest *test,
            std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {}

private:
    BareMetalComputeServiceActionMultiJobTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a DAG of jobs
        auto job1 = job_manager->createCompoundJob("job1");
        auto job2 = job_manager->createCompoundJob("job2");
        auto job3 = job_manager->createCompoundJob("job3");
        auto job4 = job_manager->createCompoundJob("job4");

        auto action1 = job1->addSleepAction("job1_sleep", 10.);
        auto action2 = job2->addSleepAction("job2_sleep", 10.);
        auto action3 = job3->addSleepAction("job3_sleep", 10.);
        auto action4 = job4->addSleepAction("job4_sleep", 10.);

        job1->addChildJob(job2);
        job1->addChildJob(job3);
        job4->addParentJob(job3);// coverage by doing the reverse

        job4->getParentJobs();  // coverage
        job3->getChildrenJobs();// coverage

        // Add an invalid dependency for testing
        try {
            job1->addParentJob(job4);
            throw std::runtime_error("Shouldn't be able to add a cycle when adding a bogus parent!");
        } catch (std::invalid_argument &e) {}
        try {
            job4->addChildJob(job1);
            throw std::runtime_error("Shouldn't be able to add a cycle when adding a bogus child!");
        } catch (std::invalid_argument &e) {}

        // Submit all the jobs, in whatever order
        job_manager->submitJob(job4, this->test->compute_service);
        job_manager->submitJob(job2, this->test->compute_service);
        job_manager->submitJob(job3, this->test->compute_service);
        job_manager->submitJob(job1, this->test->compute_service);

        // Wait for events
        for (int i = 0; i < 4; i++) {
            this->waitForNextEvent();
        }

        // Check on action sequencing
        if (action2->getStartDate() < action1->getEndDate()) {
            throw std::runtime_error("Action 2 shouldn't start before Action 1 ends");
        }
        if (action3->getStartDate() < action1->getEndDate()) {
            throw std::runtime_error("Action 3 shouldn't start before Action 1 ends");
        }
        if (action4->getStartDate() < action3->getEndDate()) {
            throw std::runtime_error("Action 4 shouldn't start before Action 3 ends");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceActionMultiJobTest, DAGOfJobs) {
    DO_TEST_WITH_FORK(do_DAGOfJobs_test);
}

void BareMetalComputeServiceActionMultiJobTest::do_DAGOfJobs_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("multi_action_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("Host3",
                                                                {std::make_pair("Host4",
                                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                wrench::ComputeService::ALL_RAM))},
                                                                {"/scratch"},
                                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
                            new DAGOfJobsTestWMS(this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
