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

class DynamicServiceCreationTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
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
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    void do_getReadyTasksTest_test();

protected:

    DynamicServiceCreationTest() {
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
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
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

class DynamicServiceCreationReadyTasksTestWMS : public wrench::WMS {

public:
    DynamicServiceCreationReadyTasksTestWMS(DynamicServiceCreationTest *test,
                                            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    DynamicServiceCreationTest *test;

    int main() {

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
        auto dynamically_created_file_registry_service = simulation->startNewService(
                new wrench::FileRegistryService(hostname));

        // Dynamically create a Network Proximity Service on this host
        auto dynamically_created_network_proximity_service = simulation->startNewService(
                new wrench::NetworkProximityService(hostname, {"DualCoreHost", "QuadCoreHost"}));

        // Dynamically create a Storage Service on this host
        auto dynamically_created_storage_service = simulation->startNewService(
                new wrench::SimpleStorageService(hostname, {"/disk2"},
                                                 {},
                                                 {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 123}}));


        // Dynamically create a Cloud Service
        std::vector<std::string> execution_hosts = {"QuadCoreHost"};
        auto dynamically_created_compute_service = std::dynamic_pointer_cast<wrench::CloudComputeService>(simulation->startNewService(
                new wrench::CloudComputeService(hostname, execution_hosts, "/scratch",
                                                { {wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));
        std::vector<wrench::WorkflowTask *> tasks = this->test->workflow->getReadyTasks();

        // Create a VM
        auto vm_name = dynamically_created_compute_service->createVM(4, 10);
        auto vm_cs = dynamically_created_compute_service->startVM(vm_name);

        std::shared_ptr<wrench::StandardJob> one_task_jobs[5];
        int job_index = 0;
        for (auto task : tasks) {
            try {
                one_task_jobs[job_index] = job_manager->createStandardJob(
                        {task},
                        {{this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service)}},
                        {},
                        {std::make_tuple(this->test->input_file,
                                         wrench::FileLocation::LOCATION(this->test->storage_service),
                                         wrench::FileLocation::LOCATION(dynamically_created_storage_service))},
                        {});

                if (one_task_jobs[job_index]->getNumTasks() != 1) {
                    throw std::runtime_error("A one-task job should say it has one task");
                }

                job_manager->submitJob(one_task_jobs[job_index], vm_cs);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(e.what());
            }

            job_index++;
        }


        // Wait for workflow execution events
        for (auto task : tasks) {
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        for (auto & j : one_task_jobs) {
            if (j->getNumCompletedTasks() != 1) {
                throw std::runtime_error("A job with one completed task should say it has one completed task");
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
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");


    std::vector<std::string> hosts = {"DualCoreHost", "QuadCoreHost"};

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "DualCoreHost";

    // Bogus startNewService() calls
    ASSERT_THROW(simulation->startNewService((wrench::ComputeService *)nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::StorageService *)nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::NetworkProximityService *)nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::FileRegistryService *)nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->startNewService((wrench::ComputeService *)666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::StorageService *)666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::NetworkProximityService *)666), std::runtime_error);
    ASSERT_THROW(simulation->startNewService((wrench::FileRegistryService *)666), std::runtime_error);

    // Create a Storage Service
    storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"},
                                             {},
                                             {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 123}}));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new DynamicServiceCreationReadyTasksTestWMS(this, {}, {storage_service}, hostname)));


    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


