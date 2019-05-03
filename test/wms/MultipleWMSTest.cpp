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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class MultipleWMSTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;

    void do_deferredWMSStartOneWMS_test();

    void do_deferredWMSStartTwoWMS_test();

protected:
    MultipleWMSTest() {
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

    wrench::Workflow *createWorkflow() {
        wrench::Workflow *workflow;
        wrench::WorkflowFile *input_file;
        wrench::WorkflowFile *output_file1;
        wrench::WorkflowFile *output_file2;
        wrench::WorkflowTask *task1;
        wrench::WorkflowTask *task2;

        // Create the simplest workflow
        workflow = new wrench::Workflow();
        workflow_unique_ptrs.push_back(std::unique_ptr<wrench::Workflow>(workflow));

        // Create the files
        input_file = workflow->addFile("input_file", 10.0);
        output_file1 = workflow->addFile("output_file1", 10.0);
        output_file2 = workflow->addFile("output_file2", 10.0);

        // Create the tasks
        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0, 0);
        task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0, 0);

        // Add file-task dependencies
        task1->addInputFile(input_file);
        task2->addInputFile(input_file);

        task1->addOutputFile(output_file1);
        task2->addOutputFile(output_file2);

        return workflow;
    }
    std::vector<std::unique_ptr<wrench::Workflow>> workflow_unique_ptrs;
    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  DEFERRED WMS START TIME WITH ONE WMS ON ONE HOST                **/
/**********************************************************************/

class DeferredWMSStartTestWMS : public wrench::WMS {

public:
    DeferredWMSStartTestWMS(MultipleWMSTest *test,
                            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test"
            ) {
        this->test = test;
    }

private:

    MultipleWMSTest *test;

    int main() {
        // check for deferred start
        checkDeferredStart();


        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get the file registry service
        auto file_registry_service = this->getAvailableFileRegistryService();

        std::set<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>, std::shared_ptr<wrench::StorageService>>> pre_copies = {};
        for (auto it : this->getWorkflow()->getInputFiles()) {
            std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::StorageService>, std::shared_ptr<wrench::StorageService>> each_copy =
                    std::make_tuple(it.second, this->test->storage_service, wrench::ComputeService::SCRATCH);
            pre_copies.insert(each_copy);
        }

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob(this->getWorkflow()->getTasks(), {},
                                                           pre_copies,
                                                           {}, {});

        // Submit the 2-task job for execution
        try {
            auto cs = *this->getAvailableComputeServices<wrench::CloudComputeService>().begin();
            auto vm_name = cs->createVM(2, 100);
            auto vm_cs = cs->startVM(vm_name);
            job_manager->submitJob(two_task_job, vm_cs);
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

TEST_F(MultipleWMSTest, DeferredWMSStartTestWMS) {
    DO_TEST_WITH_FORK(do_deferredWMSStartOneWMS_test);
    DO_TEST_WITH_FORK(do_deferredWMSStartTwoWMS_test);
}

void MultipleWMSTest::do_deferredWMSStartOneWMS_test() {
    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("multiple_wms_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, 100.0)));

    // Create a Cloud Service
    std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::CloudComputeService(
            hostname, execution_hosts, 100.0,
            {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

    // Create a WMS
    wrench::Workflow *workflow = this->createWorkflow();
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new DeferredWMSStartTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow, 100));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFiles(workflow->getInputFiles(), storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Simulation trace
    ASSERT_GT(simulation->getCurrentSimulatedDate(), 100);

    delete simulation;
    free(argv[0]);
    free(argv);
}

void MultipleWMSTest::do_deferredWMSStartTwoWMS_test() {
    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("multiple_wms_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, 100.0)));

    // Create a Cloud Service
    std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::CloudComputeService(hostname, execution_hosts, 100.0,
                                            {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

    // Create a WMS
    wrench::Workflow *workflow = this->createWorkflow();
    std::shared_ptr<wrench::WMS> wms1 = nullptr;
    ASSERT_NO_THROW(wms1 = simulation->add(
            new DeferredWMSStartTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms1->addWorkflow(workflow, 100));

    // Create a second WMS
    wrench::Workflow *workflow2 = this->createWorkflow();
    std::shared_ptr<wrench::WMS> wms2 = nullptr;
    ASSERT_NO_THROW(wms2 = simulation->add(
            new DeferredWMSStartTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms2->addWorkflow(workflow2, 10000));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFiles(workflow->getInputFiles(), storage_service));
    ASSERT_NO_THROW(simulation->stageFiles(workflow2->getInputFiles(), storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Simulation trace
    ASSERT_GT(simulation->getCurrentSimulatedDate(), 1000);

    delete simulation;
    free(argv[0]);
    free(argv);
}
