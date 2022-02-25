/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include <wrench/job/PilotJob.h>
#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"


class BareMetalComputeServiceTestPilotJobs : public ::testing::Test {

public:
    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::DataFile> output_file1;
    std::shared_ptr<wrench::DataFile> output_file2;
    std::shared_ptr<wrench::DataFile> output_file3;
    std::shared_ptr<wrench::DataFile> output_file4;
    std::shared_ptr<wrench::WorkflowTask> task1;
    std::shared_ptr<wrench::WorkflowTask> task2;
    std::shared_ptr<wrench::WorkflowTask> task3;
    std::shared_ptr<wrench::WorkflowTask> task4;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_UnsupportedPilotJobs_test();

protected:

    ~BareMetalComputeServiceTestPilotJobs() {
        workflow->clear();
    }

    BareMetalComputeServiceTestPilotJobs() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create the files
        input_file = workflow->addFile("input_file", 10.0);
        output_file1 = workflow->addFile("output_file1", 10.0);

        // Create one task1
        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 0);

        // Add file-task1 dependencies
        task1->addInputFile(input_file);

        task1->addOutputFile(output_file1);


        // Create a one-host dual-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "       </host>"
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
/**  UNSUPPORTED PILOT JOB                                           **/
/**********************************************************************/

class BareMetalComputeServiceUnsupportedPilotJobsTestWMS : public wrench::ExecutionController {

public:
    BareMetalComputeServiceUnsupportedPilotJobsTestWMS(BareMetalComputeServiceTestPilotJobs *test,
                                                       std::string hostname) :
            wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:

    BareMetalComputeServiceTestPilotJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        // Create a pilot job
        auto pilot_job = job_manager->createPilotJob();

        // Submit a pilot job
        try {
            job_manager->submitJob(pilot_job, this->test->compute_service);
            throw std::runtime_error(
                    "Should not be able to submit a pilot job to a compute service that does not support them");
        } catch (std::invalid_argument &ignore) {
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestPilotJobs, UnsupportedPilotJobs) {
    DO_TEST_WITH_FORK(do_UnsupportedPilotJobs_test);
}

void BareMetalComputeServiceTestPilotJobs::do_UnsupportedPilotJobs_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "",
                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceUnsupportedPilotJobsTestWMS(
                    this, hostname)));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));



    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

