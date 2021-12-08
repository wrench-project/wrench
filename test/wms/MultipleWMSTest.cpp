/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
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
        std::shared_ptr<wrench::DataFile> input_file;
        std::shared_ptr<wrench::DataFile> output_file1;
        std::shared_ptr<wrench::DataFile> output_file2;
        std::shared_ptr<wrench::WorkflowTask> task1;
        std::shared_ptr<wrench::WorkflowTask> task2;

        // Create the simplest workflow
        workflow = new wrench::Workflow();
        workflow_unique_ptrs.push_back(std::unique_ptr<wrench::Workflow>(workflow));

        // Create the files
        input_file = workflow->addFile("input_file", 10.0);
        output_file1 = workflow->addFile("output_file1", 10.0);
        output_file2 = workflow->addFile("output_file2", 10.0);

        // Create the tasks
        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 0);
        task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 0);

        // Add file-task1 dependencies
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

        // Get the cloud service
        auto cs = *this->getAvailableComputeServices<wrench::CloudComputeService>().begin();

        std::vector<std::tuple<std::shared_ptr<wrench::DataFile> , std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_copies = {};
        for (auto it : this->getWorkflow()->getInputFileMap()) {
            std::tuple<std::shared_ptr<wrench::DataFile> , std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>> each_copy =
                    std::make_tuple(it.second,
                                    wrench::FileLocation::LOCATION(this->test->storage_service),
                                    wrench::FileLocation::SCRATCH);
            pre_copies.push_back(each_copy);
        }

        // Create a 2-task1 job
        auto two_task_job = job_manager->createStandardJob(this->getWorkflow()->getTasks(),
                                                           (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                                                           pre_copies,
                                                           {}, {});

        // Submit the 2-task1 job for execution
        try {
            auto cs = *this->getAvailableComputeServices<wrench::CloudComputeService>().begin();
            auto vm_name = cs->createVM(2, 100);
            auto vm_cs = cs->startVM(vm_name);
            job_manager->submitJob(two_task_job, vm_cs);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
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
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Cloud Service
    std::vector<std::string> execution_hosts = {wrench::Simulation::getHostnameList()[1]};
    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::CloudComputeService(
            hostname, execution_hosts, "/scratch",
            {})));

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
    for (auto const &f : workflow->getInputFiles()) {
        ASSERT_NO_THROW(simulation->stageFile(f, storage_service));
    }

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Simulation trace
    ASSERT_GT(wrench::Simulation::getCurrentSimulatedDate(), 100);

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

void MultipleWMSTest::do_deferredWMSStartTwoWMS_test() {
    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Cloud Service
    std::vector<std::string> execution_hosts = {wrench::Simulation::getHostnameList()[1]};
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::CloudComputeService(hostname, execution_hosts, "/scratch",
                                            {})));

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
    for (auto const &f : workflow->getInputFiles()) {
        ASSERT_NO_THROW(simulation->stageFile(f, storage_service));

    }
    for (auto const &f : workflow2->getInputFiles()) {
        ASSERT_NO_THROW(simulation->stageFile(f, storage_service));

    }

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Simulation trace
    ASSERT_GT(wrench::Simulation::getCurrentSimulatedDate(), 1000);

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
