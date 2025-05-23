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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(dynamic_service_creation_test, "Log category for DynamicServiceCreationTest test");

class DynamicServiceCreationTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::DataFile> input_file;
    std::shared_ptr<wrench::DataFile> output_file1;
    std::shared_ptr<wrench::DataFile> output_file2;
    std::shared_ptr<wrench::DataFile> output_file3;
    std::shared_ptr<wrench::DataFile> output_file4;
    std::shared_ptr<wrench::DataFile> output_file5;
    std::shared_ptr<wrench::DataFile> output_file6;
    std::shared_ptr<wrench::WorkflowTask> task1;
    std::shared_ptr<wrench::WorkflowTask> task2;
    std::shared_ptr<wrench::WorkflowTask> task3;
    std::shared_ptr<wrench::WorkflowTask> task4;
    std::shared_ptr<wrench::WorkflowTask> task5;
    std::shared_ptr<wrench::WorkflowTask> task6;
    std::shared_ptr<wrench::SimpleStorageService> storage_service = nullptr;

    void do_getReadyTasksTest_test();
    void do_WeirdVectorBug_test();

protected:
    ~DynamicServiceCreationTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    DynamicServiceCreationTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();


        // Create the files
        input_file = wrench::Simulation::addFile("input_file", 10);
        output_file1 = wrench::Simulation::addFile("output_file1", 10);
        output_file2 = wrench::Simulation::addFile("output_file2", 10);
        output_file3 = wrench::Simulation::addFile("output_file3", 10);
        output_file4 = wrench::Simulation::addFile("output_file4", 10);
        output_file5 = wrench::Simulation::addFile("output_file5", 10);
        output_file6 = wrench::Simulation::addFile("output_file6", 10);

        // Create the tasks
        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 0);
        task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 0);
        task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 0);
        task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 0);
        task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 0);
        task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 0);
        task1->setClusterID("ID1");
        task2->setClusterID("ID1");
        task3->setClusterID("ID1");
        task4->setClusterID("ID2");
        task5->setClusterID("ID2");

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

        workflow->addControlDependency(task4, task5);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\" > "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\" > "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"500GBps\" latency=\"0us\"/>"
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
/**            GET READY TASKS SIMULATION TEST ON ONE HOST           **/
/**********************************************************************/

class DynamicServiceCreationReadyTasksTestWMS : public wrench::ExecutionController {

public:
    DynamicServiceCreationReadyTasksTestWMS(DynamicServiceCreationTest *test,
                                            std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    DynamicServiceCreationTest *test;

    int main() override {

        // Coverage
        try {
            wrench::Simulation::turnOnHost("bogus");
            throw std::runtime_error("Should not be able to turn on bogus host");
        } catch (std::invalid_argument &e) {}
        try {
            wrench::Simulation::turnOffHost("bogus");
            throw std::runtime_error("Should not be able to turn off bogus host");
        } catch (std::invalid_argument &e) {}
        try {
            wrench::Simulation::turnOnLink("bogus");
            throw std::runtime_error("Should not be able to turn on bogus link");
        } catch (std::invalid_argument &e) {}
        try {
            wrench::Simulation::turnOffLink("bogus");
            throw std::runtime_error("Should not be able to turn off bogus link");
        } catch (std::invalid_argument &e) {}


        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Dynamically create a File Registry Service on this host
        auto dynamically_created_file_registry_service = this->getSimulation()->startNewService(
                new wrench::FileRegistryService(hostname));

        // Dynamically create a Network Proximity Service on this host
        auto dynamically_created_network_proximity_service = this->getSimulation()->startNewService(
                new wrench::NetworkProximityService(hostname, {"DualCoreHost", "QuadCoreHost"}));

        // Dynamically create a Storage Service on this host
        auto dynamically_created_storage_service = this->getSimulation()->startNewService(
                wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"},
                                                                         {},
                                                                         {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 123}}));


        // Dynamically create a Cloud Service
        std::vector<std::string> execution_hosts = {"QuadCoreHost"};
        auto dynamically_created_compute_service = std::dynamic_pointer_cast<wrench::CloudComputeService>(this->getSimulation()->startNewService(
                new wrench::CloudComputeService(hostname, execution_hosts, "/scratch",
                                                {})));

        std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks = this->test->workflow->getReadyTasks();

        // Create a VM
        auto vm_name = dynamically_created_compute_service->createVM(4, 10);
        auto vm_cs = dynamically_created_compute_service->startVM(vm_name);


        std::vector<std::shared_ptr<wrench::StandardJob>> one_task_jobs;
        one_task_jobs.reserve(tasks.size());
        int job_index = 0;
        for (auto const &task: tasks) {
            try {
                one_task_jobs.push_back(job_manager->createStandardJob(
                        {task},
                        {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file)}},
                        {},
                        {std::make_tuple(wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file),
                                         wrench::FileLocation::LOCATION(dynamically_created_storage_service, this->test->input_file))},
                        {}));

                job_manager->submitJob(one_task_jobs[job_index], vm_cs);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(e.what());
            }
            job_index++;
        }

        // Wait for workflow execution events
        for (auto task: tasks) {
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        for (auto &j: one_task_jobs) {
            if (j->getNumCompletedTasks() != 1) {
                throw std::runtime_error("A job with one completed task1 should say it has one completed task1");
            }
        }

        return 0;
    }
};

TEST_F(DynamicServiceCreationTest, DynamicServiceCreationReadyTasksTestWMS) {
    DO_TEST_WITH_FORK(do_getReadyTasksTest_test);
}

void DynamicServiceCreationTest::do_getReadyTasksTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//        argv[1] = strdup("--wrench-full-log");


    std::vector<std::string> hosts = {"DualCoreHost", "QuadCoreHost"};

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "DualCoreHost";

    // Bogus startNewService() calls
    ASSERT_THROW(simulation->startNewService((wrench::ComputeService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::StorageService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::NetworkProximityService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::FileRegistryService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::ComputeService *) 666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::StorageService *) 666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::NetworkProximityService *) 666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::FileRegistryService *) 666), std::runtime_error);

    // Create a Storage Service
    storage_service = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"},
                                                                     {},
                                                                     {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 123}}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new DynamicServiceCreationReadyTasksTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service->createFile(input_file));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**            SIMGRID VECTOR RESERVE BUG                            **/
/**            HAPPENS WHEN the "touch" read() to update a file's    **/
/**            access date is done with simulate=true                **/
/**********************************************************************/

class WeirdVectorBugTestWMS : public wrench::ExecutionController {

public:
    WeirdVectorBugTestWMS(DynamicServiceCreationTest *test,
                          std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    DynamicServiceCreationTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Dynamically create a BM Service
        std::vector<std::string> execution_hosts = {"QuadCoreHost"};
        auto dynamically_created_compute_service = std::dynamic_pointer_cast<wrench::BareMetalComputeService>(this->getSimulation()->startNewService(
                new wrench::BareMetalComputeService(hostname, execution_hosts, "/scratch",
                                                    {})));

        // Dynamically create a Storage Service on this host
        auto dynamically_created_storage_service = this->getSimulation()->startNewService(
                wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"},
                                                                         {},
                                                                         {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 123}}));


        std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks;
        tasks = this->test->workflow->getReadyTasks();
//        tasks = {this->test->task1, this->test->task2};

        std::vector<std::shared_ptr<wrench::StandardJob>> one_task_jobs;
        one_task_jobs.reserve(tasks.size());
        int job_index = 0;
        for (auto const &task: tasks) {
            try {
                one_task_jobs.push_back(job_manager->createStandardJob(
                        {task},
                        {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file)}},
                        {},
                        {std::make_tuple(wrench::FileLocation::LOCATION(this->test->storage_service, this->test->input_file),
                                         wrench::FileLocation::LOCATION(dynamically_created_storage_service, this->test->input_file))},
                        {}));

                job_manager->submitJob(one_task_jobs.at(job_index), dynamically_created_compute_service);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(e.what());
            }
            job_index++;
        }

        wrench::Simulation::sleep(10000);

        // Wait for workflow execution events
        for (auto task: tasks) {
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        for (auto &j: one_task_jobs) {
            if (j->getNumCompletedTasks() != 1) {
                throw std::runtime_error("A job with one completed task1 should say it has one completed task1");
            }
        }

        return 0;
    }
};

TEST_F(DynamicServiceCreationTest, WeirdVectorBug) {
    DO_TEST_WITH_FORK(do_WeirdVectorBug_test);
}

void DynamicServiceCreationTest::do_WeirdVectorBug_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");


    std::vector<std::string> hosts = {"DualCoreHost", "QuadCoreHost"};

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "DualCoreHost";

    // Bogus startNewService() calls
    ASSERT_THROW(simulation->startNewService((wrench::ComputeService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::StorageService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::NetworkProximityService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::FileRegistryService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::ComputeService *) 666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::StorageService *) 666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::NetworkProximityService *) 666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::FileRegistryService *) 666), std::runtime_error);

    // Create a Storage Service
    storage_service = simulation->add(
            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"},
                                                                     {},
                                                                     {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 123}}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
            new WeirdVectorBugTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service->createFile(input_file));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
