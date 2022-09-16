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
#include <thread>
#include <chrono>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/services/Service.h"

WRENCH_LOG_CATEGORY(virtualized_cluster_service_test, "Log category for VirtualizedClusterServiceTest");

#define EPSILON 0.01

class VirtualizedClusterServiceTest : public ::testing::Test {

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
    std::shared_ptr<wrench::VirtualizedClusterComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::CloudComputeService> cloud_compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;

    void do_ConstructorTest_test();

    void do_StandardJobTaskTest_test();

    void do_StandardJobTaskWithCustomVMNameTest_test();

    void do_VMMigrationTest_test();

    void do_NumCoresTest_test();

    void do_StopAllVMsTest_test();

    void do_ShutdownVMTest_test();

    void do_ShutdownVMAndThenShutdownServiceTest_test();

    void do_SubmitToVMTest_test();

    void do_VMStartShutdownStartShutdown_test();

    void do_VMShutdownWhileJobIsRunning_test();

    void do_VMComputeServiceStopWhileJobIsRunning_test();

protected:
    ~VirtualizedClusterServiceTest() {
        workflow->clear();
    }

    VirtualizedClusterServiceTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

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

        // Add file-task1 dependencies
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
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
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
};

/**********************************************************************/
/**  CONSTRUCTOR TEST                                                **/
/**********************************************************************/

TEST_F(VirtualizedClusterServiceTest, ConstructorTest) {
    DO_TEST_WITH_FORK(do_ConstructorTest_test);
}

void VirtualizedClusterServiceTest::do_ConstructorTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];
    std::vector<std::string> execution_hosts = {wrench::Simulation::getHostnameList()[1]};

    ASSERT_THROW(cloud_compute_service = simulation->add(
                         new wrench::CloudComputeService(
                                 hostname, execution_hosts, {"/"},
                                 {{wrench::CloudComputeServiceProperty::VM_BOOT_OVERHEAD, "-1.0"}})),
                 std::invalid_argument);

    ASSERT_THROW(cloud_compute_service = simulation->add(
                         new wrench::CloudComputeService(
                                 hostname, execution_hosts, {"/"},
                                 {{wrench::CloudComputeServiceProperty::VM_RESOURCE_ALLOCATION_ALGORITHM, "bogus"}})),
                 std::invalid_argument);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class CloudStandardJobTestWMS : public wrench::ExecutionController {

public:
    CloudStandardJobTestWMS(VirtualizedClusterServiceTest *test,
                            std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() {
        auto cs = this->test->cloud_compute_service;

        cs->getCoreFlopRate();// coverage

        // Non-existent VM operations (for coverage)
        try {
            cs->startVM("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }
        try {
            cs->shutdownVM("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }
        try {
            cs->suspendVM("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }
        try {
            cs->resumeVM("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }
        try {
            cs->isVMRunning("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }
        try {
            cs->isVMDown("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }
        try {
            cs->isVMSuspended("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a 2-task1 job
        auto two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2},
                                                           (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                                                           {std::make_tuple(this->test->input_file,
                                                                            wrench::FileLocation::LOCATION(
                                                                                    this->test->storage_service),
                                                                            wrench::FileLocation::SCRATCH)},
                                                           {}, {});

        // Try to submit the job directly to the CloudComputeService (which fails)
        try {
            job_manager->submitJob(two_task_job, cs);
            throw std::runtime_error("Should not be able to submit a standard job directly to the Cloud service");
        } catch (std::invalid_argument &ignore) {
        }

        // Just for kicks (coverage), do the same with a pilot job
        auto pilot_job = job_manager->createPilotJob();
        try {
            job_manager->submitJob(pilot_job, cs, {});
            throw std::runtime_error("Should not be able to submit a pilot job directly to the Cloud service");
        } catch (std::invalid_argument &ignore) {
        }

        // Invalid VM creations for coverage
        try {
            auto vm_name = cs->createVM(wrench::ComputeService::ALL_CORES, 10);
            throw std::runtime_error("Should not be able to pass ALL_CORES to createVM()");
        } catch (std::invalid_argument &ignore) {}
        try {
            auto vm_name = cs->createVM(2, wrench::ComputeService::ALL_RAM);
            throw std::runtime_error("Should not be able to pass ALL_RAM to createVM()");
        } catch (std::invalid_argument &ignore) {}

        // Create a VM
        auto vm_name = cs->createVM(2, 10);

        // Check the state
        if (not cs->isVMDown(vm_name)) {
            throw std::runtime_error("A just created VM should be down");
        }

        // Try to shutdown the VM, which doesn't work
        try {
            cs->shutdownVM(vm_name);
            throw std::runtime_error("Should not be able to shutdown a non-running VM");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NotAllowed>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Invalid failure cause: " + e.getCause()->toString() +
                                         " (expected: NotAllowed)");
            }
            auto error_msg = cause->toString();// coverage
            if (cause->getService() != cs) {
                throw std::runtime_error("Failure cause does not point to the (correct) service");
            }
        }

        // Check that we cannot get the CS back
        if (cs->getVMComputeService(vm_name) != nullptr) {
            throw std::runtime_error("A non-started VM should have a nullptr compute service");
        }

        // Start the VM
        auto vm_cs = cs->startVM(vm_name);

        // Check that we can get the CS back
        auto vm_cs_should_be_same = cs->getVMComputeService(vm_name);
        if (vm_cs != vm_cs_should_be_same) {
            throw std::runtime_error("It should be possible to get the computer service of a started VM");
        }


        // Check the state
        if (not cs->isVMRunning(vm_name)) {
            throw std::runtime_error("A just started VM should be running");
        }

        // Submit the 2-task1 job for execution
        try {
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


        // Shutdown the VM, because why not
        cs->shutdownVM(vm_name);

        // Shutdown a bogus VM, for coverage
        try {
            cs->shutdownVM("bogus");
            throw std::runtime_error("Should not be able to shutdown a non-existing VM");
        } catch (std::invalid_argument &e) {}

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, CloudStandardJobTest) {
    DO_TEST_WITH_FORK(do_StandardJobTaskTest_test);
}

void VirtualizedClusterServiceTest::do_StandardJobTaskTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

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
    ASSERT_NO_THROW(cloud_compute_service = simulation->add(
                            new wrench::CloudComputeService(
                                    hostname, execution_hosts, "/scratch",
                                    {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CloudStandardJobTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/***********************************************************************************/
/**  STANDARD JOB SUBMISSION TASK SIMULATION TEST WITH CUSTOM VM NAME ON ONE HOST **/
/***********************************************************************************/

class CloudStandardJobWithCustomVMNameTestWMS : public wrench::ExecutionController {

public:
    CloudStandardJobWithCustomVMNameTestWMS(VirtualizedClusterServiceTest *test,
                                            std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() {
        auto cs = this->test->cloud_compute_service;

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create and start a VM
        auto vm_name = cs->createVM(2, 10, "my_custom_name");

        if (vm_name != "my_custom_name") {
            throw std::runtime_error("Could not create VM with the desired name");
        }

        // Try to create a VM with the same name
        try {
            auto bogus_vm_name = cs->createVM(2, 10, "my_custom_name");
            throw std::runtime_error("Should not be able to create a VM with an existing name!");
        } catch (wrench::ExecutionException &e) {}

        // Start the VM
        auto vm_cs = cs->startVM(vm_name);

        // Create a 2-task1 job
        auto two_task_job = job_manager->createStandardJob(
                {this->test->task1, this->test->task2}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(
                                         this->test->storage_service),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the 2-task1 job for execution
        try {
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

TEST_F(VirtualizedClusterServiceTest, CloudStandardJobWithCustomVMNameTestWMS) {
    DO_TEST_WITH_FORK(do_StandardJobTaskWithCustomVMNameTest_test);
}

void VirtualizedClusterServiceTest::do_StandardJobTaskWithCustomVMNameTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
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
    ASSERT_NO_THROW(cloud_compute_service = simulation->add(
                            new wrench::CloudComputeService(
                                    hostname, execution_hosts, "/scratch",
                                    {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CloudStandardJobWithCustomVMNameTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**                   VM MIGRATION SIMULATION TEST                   **/
/**********************************************************************/

class VirtualizedClusterVMMigrationTestWMS : public wrench::ExecutionController {

public:
    VirtualizedClusterVMMigrationTestWMS(VirtualizedClusterServiceTest *test,
                                         std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();
        auto cs = this->test->compute_service;

        // Create a 2-task1 job
        auto two_task_job = job_manager->createStandardJob(
                {this->test->task1, this->test->task2}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(this->test->storage_service),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the 2-task1 job for execution
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
            } catch (wrench::ExecutionException &e) {}

            job_manager->submitJob(two_task_job, vm_cs);

            // migrating the VM
            wrench::Simulation::sleep(0.01);// TODO: Without this sleep, the test hangs! This is being investigated...

            try {// try a bogus one for coverage
                cs->migrateVM("NON-EXISTENT", "DualCoreHost");
                throw std::runtime_error("Should not be able to migrate a non-existent VM");
            } catch (std::invalid_argument &e) {}

            try {// try a bogus one for coverage
                cs->migrateVM(vm_name, "TinyHost");
                throw std::runtime_error("Should not be able to migrate a VM to a host without sufficient resources");
            } catch (wrench::ExecutionException &e) {}

            // Get the runnin physical hostname
            auto hostname_pre = cs->getVMPhysicalHostname(vm_name);
            if (hostname_pre != src_host) {
                throw std::runtime_error("VM should be running on physical host " + src_host);
            }
            std::string dst_host = "DualCoreHost";
            cs->migrateVM(vm_name, dst_host);
            auto hostname_post = cs->getVMPhysicalHostname(vm_name);
            if (hostname_post != dst_host) {
                throw std::runtime_error("VM should, after migration, be running on physical host " + dst_host);
            }

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

TEST_F(VirtualizedClusterServiceTest, VirtualizedClusterVMMigrationTestWMS) {
    DO_TEST_WITH_FORK(do_VMMigrationTest_test);
}

void VirtualizedClusterServiceTest::do_VMMigrationTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
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

    // Create a Virtualized Cluster Service with no hosts
    std::vector<std::string> nothing;
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::VirtualizedClusterComputeService(hostname, nothing, "/scratch",
                                                                      {})),
                 std::invalid_argument);

    // Create a Virtualized Cluster Service
    std::vector<std::string> execution_hosts = wrench::Simulation::getHostnameList();

    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, "/scratch",
                                                                         {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new VirtualizedClusterVMMigrationTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    simulation->stageFile(input_file, storage_service);
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  NUM CORES TEST                                                  **/
/**********************************************************************/

class CloudNumCoresTestWMS : public wrench::ExecutionController {

public:
    CloudNumCoresTestWMS(VirtualizedClusterServiceTest *test,
                         std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() {
        try {
            // no VMs
            unsigned long sum_num_cores = this->test->cloud_compute_service->getTotalNumCores();

            unsigned long sum_num_idle_cores = this->test->cloud_compute_service->getTotalNumIdleCores();

            if (sum_num_cores != 6 || sum_num_idle_cores != 6) {
                throw std::runtime_error("getTotalNumCores() and getTotalNumIdleCores() should be 6 (they report " +
                                         std::to_string(sum_num_cores) + " and " + std::to_string(sum_num_idle_cores) +
                                         ")");
            }

            // create and start VM with the 2  cores and 10 bytes of RAM
            auto cs = this->test->cloud_compute_service;
            cs->startVM(cs->createVM(2, 10));

            sum_num_idle_cores = cs->getTotalNumIdleCores();

            if (sum_num_idle_cores != 4) {
                throw std::runtime_error(
                        "getTotalNumIdleCores() should be 4 (it is reported as " + std::to_string(sum_num_idle_cores) +
                        ")");
            }

            // create and start a VM with two cores
            cs->startVM(cs->createVM(2, 10));
            sum_num_idle_cores = cs->getTotalNumIdleCores();

            if (sum_num_idle_cores != 2) {
                throw std::runtime_error(
                        "getTotalNumCores() and getTotalNumIdleCores() should be 2 (it is reported as " +
                        std::to_string(sum_num_idle_cores) + ")");
            }

        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, CloudNumCoresTestWMS) {
    DO_TEST_WITH_FORK(do_NumCoresTest_test);
}

void VirtualizedClusterServiceTest::do_NumCoresTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
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
    std::vector<std::string> execution_hosts = {"QuadCoreHost", "DualCoreHost"};
    ASSERT_NO_THROW(cloud_compute_service = simulation->add(
                            new wrench::CloudComputeService(hostname, execution_hosts, "",
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new CloudNumCoresTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  STOP ALL VMS TEST                                               **/
/**********************************************************************/

class StopAllVMsTestWMS : public wrench::ExecutionController {

public:
    wrench::WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
            {wrench::ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024}};

    StopAllVMsTestWMS(VirtualizedClusterServiceTest *test,
                      std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {

        this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a bunch of VMs
        try {
            auto cs = this->test->compute_service;
            std::string execution_host = cs->getExecutionHosts()[0];

            cs->startVM(cs->createVM(1, 10));
            cs->startVM(cs->createVM(1, 10), execution_host);
            cs->startVM(cs->createVM(1, 10), execution_host);
            cs->startVM(cs->createVM(1, 10), execution_host);

        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        wrench::Simulation::sleep(10);

        // stop all VMs
        this->test->compute_service->stop();

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, StopAllVMsTestWMS) {
    DO_TEST_WITH_FORK(do_StopAllVMsTest_test);
}

void VirtualizedClusterServiceTest::do_StopAllVMsTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
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
                            new wrench::VirtualizedClusterComputeService(
                                    hostname, execution_hosts, "",
                                    {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new StopAllVMsTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  VM SHUTDOWN, START, SUSPEND, RESUME TEST                        **/
/**********************************************************************/

class ShutdownVMTestWMS : public wrench::ExecutionController {

public:
    wrench::WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, 1024}};

    ShutdownVMTestWMS(VirtualizedClusterServiceTest *test,
                      std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {

        this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() override {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a pilot job that requests 1 host, 1 code, 0 bytes, and 1 minute

        std::vector<std::string> vm_list;

        auto cs = this->test->compute_service;

        // Create  and start VMs
        try {
            std::string execution_host = cs->getExecutionHosts()[0];
            for (int i = 0; i < 4; i++) {
                auto vm_name = cs->createVM(1, 10);
                vm_list.push_back(vm_name);
                cs->startVM(vm_name);
            }
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        wrench::Simulation::sleep(1);
        // shutdown all VMs
        try {
            for (auto &vm: vm_list) {
                cs->shutdownVM(vm);
            }
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }


        // Create a one-task1 job
        std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>> file_locations;
        file_locations[this->test->input_file] = wrench::FileLocation::LOCATION(this->test->storage_service);
        file_locations[this->test->output_file1] = wrench::FileLocation::LOCATION(this->test->storage_service);
        auto job = job_manager->createStandardJob(this->test->task1, file_locations);

        // Submit a job
        try {
            job_manager->submitJob(job, cs->getVMComputeService(*(vm_list.begin())));
            throw std::runtime_error("should have thrown an exception since there are no resources available");
        } catch (wrench::ExecutionException &e) {
            // do nothing, should have thrown an exception since there are no resources available
        }

        // (Re)start VM #3
        try {
            cs->startVM(vm_list[3]);
        } catch (std::runtime_error &e) {
            throw std::runtime_error(e.what());
        }

        try {
            cs->suspendVM(vm_list[3]);
            if (not cs->isVMSuspended(vm_list[3])) {
                throw std::runtime_error("A just suspended VM should be in the suspended state");
            }
            job_manager->submitJob(job, cs->getVMComputeService(vm_list[3]));
            throw std::runtime_error("should have thrown an exception since there are no resources available");
        } catch (wrench::ExecutionException &e) {
            // do nothing, should have thrown an exception since there are no resources available
        }

        // Try to suspend it again, wrongly, for coverage
        try {
            cs->suspendVM(vm_list[3]);
            throw std::runtime_error("Should not be able to suspend a non-running VM");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NotAllowed>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Unexpected failure cause " + e.getCause()->toString() +
                                         " (Expected: NotAllowed)");
            }
        }

        try {
            cs->resumeVM(vm_list[3]);
            if (not cs->isVMRunning(vm_list[3])) {
                throw std::runtime_error("A just resumed VM should be in the running state");
            }
        } catch (std::runtime_error &e) {
            throw std::runtime_error(e.what());
        }

        try {
            cs->resumeVM(vm_list[3]);
            throw std::runtime_error("VM was already resumed, so an exception was expected!");
        } catch (wrench::ExecutionException &e) {
            // Expected
        }


        job_manager->submitJob(job, cs->getVMComputeService(vm_list[3]));

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

        // Submit a job and suspend the VM before that job finishes
        file_locations.clear();
        file_locations[this->test->input_file] = wrench::FileLocation::LOCATION(this->test->storage_service);
        file_locations[this->test->output_file2] = wrench::FileLocation::LOCATION(this->test->storage_service);
        auto other_job = job_manager->createStandardJob(this->test->task2, file_locations);

        try {
            job_manager->submitJob(other_job, cs->getVMComputeService(vm_list[3]));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to submit the other job");
        }
        double job_start_date = wrench::Simulation::getCurrentSimulatedDate();

        WRENCH_INFO("Sleeping for 5 seconds");
        wrench::Simulation::sleep(5);

        try {
            WRENCH_INFO("Suspending the one running VM (which is thus running the job)");
            cs->suspendVM(vm_list[3]);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to suspend VM");
        }

        WRENCH_INFO("Sleeping for 100 seconds");
        wrench::Simulation::sleep(100);

        try {
            WRENCH_INFO("Resuming the VM");
            cs->resumeVM(vm_list[3]);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to resume VM");
        }

        // Wait for a workflow execution event
        WRENCH_INFO("Waiting for job completion");
        std::shared_ptr<wrench::ExecutionEvent> event2;
        try {
            event2 = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event2)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event2->toString());
        }

        double job_turnaround_time = wrench::Simulation::getCurrentSimulatedDate() - job_start_date;
        if (std::abs(job_turnaround_time - 110) > EPSILON) {
            throw std::runtime_error("Unexpected job turnaround time " + std::to_string(job_turnaround_time));
        }

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, ShutdownVMTestWMS) {
    DO_TEST_WITH_FORK(do_ShutdownVMTest_test);
}

void VirtualizedClusterServiceTest::do_ShutdownVMTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

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
                            new wrench::VirtualizedClusterComputeService(
                                    hostname, execution_hosts, "",
                                    {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ShutdownVMTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  VM START-SHUTDOWN, and then SERVICE SHUTDOWN                    **/
/**********************************************************************/

class ShutdownVMAndThenShutdownServiceTestWMS : public wrench::ExecutionController {

public:
    wrench::WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, 1024}};

    ShutdownVMAndThenShutdownServiceTestWMS(VirtualizedClusterServiceTest *test,
                                            std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {

        this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a pilot job that requests 1 host, 1 code, 0 bytes, and 1 minute
        auto pilot_job = job_manager->createPilotJob();

        std::vector<std::tuple<std::string, std::shared_ptr<wrench::BareMetalComputeService>>> vm_list;

        auto cs = this->test->compute_service;

        // Create VMs
        try {
            std::string execution_host = cs->getExecutionHosts()[0];

            for (int i = 0; i < 4; i++) {
                auto vm_name = cs->createVM(1, 10);
                auto vm_cs = cs->startVM(vm_name, execution_host);
                vm_list.push_back(std::make_tuple(vm_name, vm_cs));
            }

        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // shutdown some VMs
        try {
            cs->shutdownVM(std::get<0>(vm_list.at(0)));
            cs->shutdownVM(std::get<0>(vm_list.at(2)));
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // stop service
        cs->stop();

        // Sleep a bit
        wrench::Simulation::sleep(10.0);

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, ShutdownVMAndThenShutdownServiceTestWMS) {
    DO_TEST_WITH_FORK(do_ShutdownVMAndThenShutdownServiceTest_test);
}

void VirtualizedClusterServiceTest::do_ShutdownVMAndThenShutdownServiceTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
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
                            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, "",
                                                                         {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ShutdownVMAndThenShutdownServiceTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  SUBMIT TO VM TEST                                               **/
/**********************************************************************/

class SubmitToVMTestWMS : public wrench::ExecutionController {

public:
    wrench::WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE default_messagepayload_values = {
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD, 1024},
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, 1024}};

    SubmitToVMTestWMS(VirtualizedClusterServiceTest *test,
                      std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
        this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        //        std::vector<std::tuple<std::string, std::shared_ptr<wrench::BareMetalComputeService>>> vm_list;
        std::vector<std::string> vm_list;

        auto cs = this->test->compute_service;

        // Create some VMs
        try {
            std::string execution_host = cs->getExecutionHosts()[0];

            for (int i = 0; i < 2; i++) {
                auto vm_name = cs->createVM(1, 10);
                cs->startVM(vm_name, execution_host);
                vm_list.push_back(vm_name);
            }

        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(e.what());
        }


        try {
            for (auto &vm: vm_list) {
                cs->shutdownVM(vm);
            }
            auto job1 = job_manager->createStandardJob({this->test->task1}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                                                       {std::make_tuple(this->test->input_file,
                                                                        wrench::FileLocation::LOCATION(
                                                                                this->test->storage_service),
                                                                        wrench::FileLocation::SCRATCH)},
                                                       {}, {});
            // Trying to submit to a VM that has been shutdown
            job_manager->submitJob(job1, cs->getVMComputeService(vm_list[0]));
            throw std::runtime_error("Should not be able to run job since VMs are stopped");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Invalid failure cause: " + e.getCause()->toString() +
                                         " (expected: ServiceIsDown)");
            }
            // do nothing, expected behavior
        }

        try {
            cs->startVM(vm_list[1]);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Couldn't start VM: " + e.getCause()->toString());
        }

        auto job1 = job_manager->createStandardJob({this->test->task1}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                                                   {std::make_tuple(this->test->input_file,
                                                                    wrench::FileLocation::LOCATION(
                                                                            this->test->storage_service),
                                                                    wrench::FileLocation::SCRATCH)},
                                                   {}, {});

        try {
            job_manager->submitJob(job1, cs->getVMComputeService(vm_list[1]));
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

TEST_F(VirtualizedClusterServiceTest, SubmitToVMTestWMS) {
    DO_TEST_WITH_FORK(do_SubmitToVMTest_test);
}

void VirtualizedClusterServiceTest::do_SubmitToVMTest_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

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
                            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, "/scratch")));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    ASSERT_NO_THROW(wms = simulation->add(
                            new SubmitToVMTestWMS(this, hostname)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**                VM START-SHUTDOWN-START-SHUTDOWN                  **/
/**********************************************************************/

class CloudServiceVMStartShutdownStartShutdownTestWMS : public wrench::ExecutionController {

public:
    CloudServiceVMStartShutdownStartShutdownTestWMS(VirtualizedClusterServiceTest *test,
                                                    std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() override {
        auto cloud_service = this->test->cloud_compute_service;

        // Create a VM on the Cloud Service
        auto vm_name = cloud_service->createVM(2, 1024);

        // Start the VM
        wrench::Simulation::sleep(10);
        auto vm_cs = cloud_service->startVM(vm_name);

        // Shutdown the VM
        wrench::Simulation::sleep(10);
        cloud_service->shutdownVM(vm_name);

        // Start the VM
        wrench::Simulation::sleep(10);
        vm_cs = cloud_service->startVM(vm_name);

        // Shutdown the VM
        wrench::Simulation::sleep(10);
        cloud_service->shutdownVM(vm_name);

        // Destroying the VM
        wrench::Simulation::sleep(10);
        cloud_service->destroyVM(vm_name);
        try {
            cloud_service->destroyVM(vm_name);
            throw std::runtime_error("Shouldn't be able to destroy an already destroyed VM");
        } catch (std::invalid_argument &e) {}

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, VMStartShutdownStartShutdown) {
    DO_TEST_WITH_FORK(do_VMStartShutdownStartShutdown_test);
}

void VirtualizedClusterServiceTest::do_VMStartShutdownStartShutdown_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "DualCoreHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("QuadCoreHost");

    // Create a Compute Service that has access to two hosts
    cloud_compute_service = simulation->add(
            new wrench::CloudComputeService(hostname,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(hostname, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    wms = simulation->add(
            new CloudServiceVMStartShutdownStartShutdownTestWMS(this, hostname));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(hostname));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**                VM SHUTDOWN WHILE JOB IS RUNNING                  **/
/**********************************************************************/

class CloudServiceVMShutdownWhileJobIsRunningTestWMS : public wrench::ExecutionController {

public:
    CloudServiceVMShutdownWhileJobIsRunningTestWMS(VirtualizedClusterServiceTest *test,
                                                   std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() override {
        auto cloud_service = this->test->cloud_compute_service;

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a VM on the Cloud Service
        auto vm_name = cloud_service->createVM(2, 1024);

        // Start the VM
        wrench::Simulation::sleep(10);
        auto vm_cs = cloud_service->startVM(vm_name);

        // Create a job
        auto job = job_manager->createStandardJob(
                {this->test->task1}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(
                        this->test->input_file,
                        wrench::FileLocation::LOCATION(this->test->storage_service),
                        wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the job to the vm
        job_manager->submitJob(job, vm_cs);

        wrench::Simulation::sleep(10);

        // Shutdown the VM
        cloud_service->shutdownVM(vm_name, true, wrench::ComputeService::TerminationCause::TERMINATION_JOB_KILLED);


        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);

        if (not real_event) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString() +
                                     " (should be StandardJobFailedEvent)");
        }

        auto cause = std::dynamic_pointer_cast<wrench::JobKilled>(real_event->failure_cause);
        if (not cause) {
            throw std::runtime_error(
                    "Unexpected failure cause: " + real_event->failure_cause->toString() + " (expected: JobKilled)");
        }
        if (cause->getJob() != job) {
            throw std::runtime_error("Failure cause does not point to the correct job");
        }

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, VMShutdownWhileJobIsRunning) {
    DO_TEST_WITH_FORK(do_VMShutdownWhileJobIsRunning_test);
}

void VirtualizedClusterServiceTest::do_VMShutdownWhileJobIsRunning_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "DualCoreHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("QuadCoreHost");

    // Create a Compute Service that has access to two hosts
    cloud_compute_service = simulation->add(
            new wrench::CloudComputeService(hostname,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(hostname, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    wms = simulation->add(
            new CloudServiceVMShutdownWhileJobIsRunningTestWMS(this, hostname));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(hostname));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**   VM COMPUTE SERVICE STOP SHUTDOWN WHILE JOB IS RUNNING          **/
/**********************************************************************/

class CloudServiceVMComputeServiceStopWhileJobIsRunningTestWMS : public wrench::ExecutionController {

public:
    CloudServiceVMComputeServiceStopWhileJobIsRunningTestWMS(VirtualizedClusterServiceTest *test,
                                                             std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    VirtualizedClusterServiceTest *test;

    int main() override {
        auto cloud_service = this->test->cloud_compute_service;

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a VM on the Cloud Service
        auto vm_name = cloud_service->createVM(2, 1024);

        // Start the VM
        wrench::Simulation::sleep(10);
        auto vm_cs = cloud_service->startVM(vm_name);

        // Create a job
        auto job = job_manager->createStandardJob(
                {this->test->task1}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){},
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(this->test->storage_service),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the job to the VM
        job_manager->submitJob(job, vm_cs);

        wrench::Simulation::sleep(10);

        // Stop the VM Compute Service
        vm_cs->stop(true, wrench::ComputeService::TerminationCause::TERMINATION_COMPUTE_SERVICE_TERMINATED);

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        if (not real_event) {
            throw std::runtime_error(
                    "Unexpected workflow execution event: " + event->toString() + " (should be STANDARD_JOB_FAILURE)");
        }

        auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(real_event->failure_cause);
        if (not cause) {
            throw std::runtime_error(
                    "Unexpected failure cause: " + real_event->failure_cause->toString() + " (expected: ServiceIsDown)");
        }

        wrench::Simulation::sleep(0.0001);// To let the cloud CS to update the VM state

        // Check that the VM is down, as it should
        if (not cloud_service->isVMDown(vm_name)) {
            throw std::runtime_error("VM should be down after its bare_metal_standard_jobs has been stopped");
        }

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, VMComputeServiceStopWhileJobIsRunning) {
    DO_TEST_WITH_FORK(do_VMComputeServiceStopWhileJobIsRunning_test);
}

void VirtualizedClusterServiceTest::do_VMComputeServiceStopWhileJobIsRunning_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "DualCoreHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("QuadCoreHost");

    // Create a Compute Service that has access to two hosts
    cloud_compute_service = simulation->add(
            new wrench::CloudComputeService(hostname,
                                            compute_hosts,
                                            "/scratch",
                                            {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(hostname, {"/"}));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ;
    wms = simulation->add(new CloudServiceVMComputeServiceStopWhileJobIsRunningTestWMS(
            this, hostname));

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(hostname));
    simulation->stageFile(input_file, storage_service);

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
