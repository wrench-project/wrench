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

#include "wrench/workflow/job/PilotJob.h"
#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"


class BareMetalComputeServiceTestPilotJobs : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowFile *output_file4;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_UnsupportedPilotJobs_test();

    void do_OnePilotJobNoTimeoutWaitForExpiration_test();

    void do_OnePilotJobNoTimeoutShutdownService_test();

    void do_NonSubmittedPilotJobTermination_test();

    void do_IdlePilotJobTermination_test();

    void do_NonIdlePilotJobTermination_test();


protected:
    BareMetalComputeServiceTestPilotJobs() {

        // Create the simplest workflow
        workflow = new wrench::Workflow();

        // Create the files
        input_file = workflow->addFile("input_file", 10.0);
        output_file1 = workflow->addFile("output_file1", 10.0);

        // Create one task
        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0, 0);

        // Add file-task dependencies
        task1->addInputFile(input_file);

        task1->addOutputFile(output_file1);


        // Create a one-host dual-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100\"/>"
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
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  UNSUPPORTED PILOT JOB                                           **/
/**********************************************************************/

class BareMetalComputeServiceUnsupportedPilotJobsTestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceUnsupportedPilotJobsTestWMS(BareMetalComputeServiceTestPilotJobs *test,
                                                       const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                       const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                       std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestPilotJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a pilot job
        wrench::PilotJob *pilot_job = job_manager->createPilotJob();

        // Submit a pilot job
        try {
            job_manager->submitJob(pilot_job, this->test->compute_service);
            throw std::runtime_error(
                    "Should not be able to submit a pilot job to a compute service that does not support them");
        } catch (wrench::WorkflowExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::JobTypeNotSupported>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Did get the expected exception but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: JobTypeNotSupported)");
            }
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestPilotJobs, UnsupportedPilotJobs) {
    DO_TEST_WITH_FORK(do_UnsupportedPilotJobs_test);
}

void BareMetalComputeServiceTestPilotJobs::do_UnsupportedPilotJobs_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                0,
                                                {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceUnsupportedPilotJobsTestWMS(
                    this,  {
                            compute_service
                    }, {
                            storage_service
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));



    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

