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
#include <numeric>
#include <thread>
#include <chrono>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(virtualized_cluster_service_broken_test, "Log category for VirtualizedClusterServiceBrokenTest");

#define EPSILON 0.01

class VirtualizedClusterServiceBrokenTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowFile *output_file4;
    wrench::WorkflowFile *output_file5;
    wrench::WorkflowFile *output_file6;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;
    wrench::WorkflowTask *task5;
    wrench::WorkflowTask *task6;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;

    void do_VMMigrationTest_test();

protected:
    VirtualizedClusterServiceBrokenTest() {

        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create the files
        input_file = workflow->addFile("input_file", 10.0);
        output_file1 = workflow->addFile("output_file1", 10.0);
        output_file2 = workflow->addFile("output_file2", 10.0);
        output_file3 = workflow->addFile("output_file3", 10.0);
        output_file4 = workflow->addFile("output_file4", 10.0);
        output_file5 = workflow->addFile("output_file5", 10.0);
        output_file6 = workflow->addFile("output_file6", 10.0);

        // Create the tasks
        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0, 0);
        task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0, 0);
        task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 1.0, 0);
        task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 1.0, 0);
        task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 1.0, 0);
        task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 1.0, 0);

        // Add file-task dependencies
        task1->addInputFile(input_file);
        task2->addInputFile(input_file);
        task3->addInputFile(input_file);
        task4->addInputFile(input_file);
        task5->addInputFile(input_file);
        task6->addInputFile(input_file);

        task1->addOutputFile(output_file1);
        task2->addOutputFile(output_file2);
        task3->addOutputFile(output_file3);
        task4->addOutputFile(output_file4);
        task5->addOutputFile(output_file5);
        task6->addOutputFile(output_file6);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"TinyHost\" speed=\"1f\" core=\"1\" >"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"TinyHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"DualCoreHost\" dst=\"TinyHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
};



/**********************************************************************/
/**                   VM MIGRATION SIMULATION TEST                   **/
/**********************************************************************/

class VirtualizedClusterVMMigrationBrokenTestWMS : public wrench::WMS {

public:
    VirtualizedClusterVMMigrationBrokenTestWMS(VirtualizedClusterServiceBrokenTest *test,
                                         const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                         const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                         std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceBrokenTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        auto cs = *(this->getAvailableComputeServices<wrench::VirtualizedClusterComputeService>().begin());

        // Create a 2-task job
        wrench::StandardJob *two_task_job = job_manager->createStandardJob(
                {this->test->task1, this->test->task2}, {},
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(this->test->storage_service),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the 2-task job for execution
        try {

            std::string src_host = "QuadCoreHost";
            auto vm_name = cs->createVM(2, 10);


            try {
                cs->startVM("NON-EXISTENT", src_host);
                throw std::runtime_error("Shouldn't be able to start a bogus VM");
            } catch (std::invalid_argument &e) {}

            try {
                cs->startVM(vm_name, "NON-EXISTENT");
                throw std::runtime_error("Shouldn't be able to start a VM on a bogus host");
            } catch (std::invalid_argument &e) {}

            auto vm_cs = cs->startVM(vm_name, src_host);

            try {
                cs->startVM(vm_name, src_host);
                throw std::runtime_error("Shouldn't be able to start a VM that is not DOWN");
            } catch (wrench::WorkflowExecutionException &e) {}



            job_manager->submitJob(two_task_job, vm_cs);


            // migrating the VM
 //           wrench::Simulation::sleep(0.01); // TODO Without this sleep, the test hangs! This is being investigated...

            try { // try a bogus one for coverage
                cs->migrateVM("NON-EXISTENT", "DualCoreHost");
                throw std::runtime_error("Should not be able to migrate a non-existent VM");
            } catch (std::invalid_argument &e) {}

            try { // try a bogus one for coverage
                cs->migrateVM(vm_name, "TinyHost");
                throw std::runtime_error("Should not be able to migrate a VM to a host without sufficient resources");
            } catch (wrench::WorkflowExecutionException &e) {}

            cs->migrateVM(vm_name, "DualCoreHost");

        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceBrokenTest, Broken) {
    DO_TEST_WITH_FORK(do_VMMigrationTest_test);
}

void VirtualizedClusterServiceBrokenTest::do_VMMigrationTest_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));


    // Create a Virtualized Cluster Service with no hosts
    std::vector<std::string> nothing;
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::VirtualizedClusterComputeService(hostname, nothing, "/scratch",
                                                         {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,
                                                                  "false"}})), std::invalid_argument);

    // Create a Virtualized Cluster Service
    std::vector<std::string> execution_hosts = wrench::Simulation::getHostnameList();

    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, "/scratch",
                                                         {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,
                                                                  "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new VirtualizedClusterVMMigrationBrokenTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    simulation->stageFile(input_file, storage_service);
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}




