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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(virtualized_cluster_service_test, "Log category for VirtualizedClusterServiceTest");

#define EPSILON 0.001

class VirtualizedClusterServiceTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowFile *output_file4;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;
    wrench::WorkflowTask *task5;
    wrench::WorkflowTask *task6;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;

    void do_StandardJobTaskTest_test();

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
    VirtualizedClusterServiceTest() {

        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create the files
        input_file = workflow->addFile("input_file", 10.0);
        output_file1 = workflow->addFile("output_file1", 10.0);
        output_file2 = workflow->addFile("output_file2", 10.0);
        output_file3 = workflow->addFile("output_file3", 10.0);
        output_file4 = workflow->addFile("output_file4", 10.0);

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
        task5->addOutputFile(output_file3);
        task6->addOutputFile(output_file4);

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

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
};

/**********************************************************************/
/**  STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class CloudStandardJobTestWMS : public wrench::WMS {

public:
    CloudStandardJobTestWMS(VirtualizedClusterServiceTest *test,
                            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
        auto cs = std::dynamic_pointer_cast<wrench::CloudComputeService>(this->test->compute_service);


        // Non-existent VM operations (for coverage)
        try {
            cs->startVM("NON_EXISTENT_VM");
            cs->shutdownVM("NON_EXISTENT_VM");
            cs->suspendVM("NON_EXISTENT_VM");
            cs->resumeVM("NON_EXISTENT_VM");
            cs->isVMRunning("NON_EXISTENT_VM");
            cs->isVMDown("NON_EXISTENT_VM");
            cs->isVMSuspended("NON_EXISTENT_VM");
            throw std::runtime_error("Shouldn't be able to interact with a non-existent VM");
        } catch (std::invalid_argument &e) {
            // do nothing, since it is the expected behavior
        }

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a 2-task job
        wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {},
                                                                           {std::make_tuple(this->test->input_file,
                                                                                            this->test->storage_service,
                                                                                            wrench::ComputeService::SCRATCH)},
                                                                           {}, {});


        // Try to submit the job directly to the CloudComputeService (which fails)
        try {
            job_manager->submitJob(two_task_job, cs);
            throw std::runtime_error("Should not be able to submit a job directly to the Cloud service");
        } catch (wrench::WorkflowExecutionException &e) {
            if (e.getCause()->getCauseType() != wrench::FailureCause::JOB_TYPE_NOT_SUPPORTED) {
                throw std::runtime_error("Invalid failure cause (should be JOB_TYPE_NOT_SUPPORTED");
            }

        }

        // Create and start a VM
        auto vm_name = cs->createVM(2, 10);

        // Check the state
        if (not cs->isVMDown(vm_name)) {
          throw std::runtime_error("A just created VM should be down");
        }

        auto vm_cs = cs->startVM(vm_name);

        // Check the state
        if (not cs->isVMRunning(vm_name)) {
            throw std::runtime_error("A just started VM should be running");
        }

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(two_task_job, vm_cs);
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                // success, do nothing for now
                break;
            }
            default: {
                throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
        }

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, CloudStandardJobTestWMS) {
    DO_TEST_WITH_FORK(do_StandardJobTaskTest_test);
}

void VirtualizedClusterServiceTest::do_StandardJobTaskTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("cloud_service_test");

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
            new wrench::CloudComputeService(hostname, execution_hosts, 100,
                                     {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new CloudStandardJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

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

/**********************************************************************/
/**                   VM MIGRATION SIMULATION TEST                   **/
/**********************************************************************/

class VirtualizedClusterVMMigrationTestWMS : public wrench::WMS {

public:
    VirtualizedClusterVMMigrationTestWMS(VirtualizedClusterServiceTest *test,
                                         const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                         const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                         std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a 2-task job
        wrench::StandardJob *two_task_job = job_manager->createStandardJob(
                {this->test->task1, this->test->task2}, {},
                {std::make_tuple(this->test->input_file, this->test->storage_service, wrench::ComputeService::SCRATCH)},
                {}, {});

        // Submit the 2-task job for execution
        try {
            auto cs = std::dynamic_pointer_cast<wrench::VirtualizedClusterComputeService>(this->test->compute_service);
            std::string src_host = cs->getExecutionHosts()[0];
            auto vm_name = cs->createVM(2, 10);
            auto vm_cs = cs->startVM(vm_name, src_host);

            job_manager->submitJob(two_task_job, vm_cs);

            // migrating the VM
            std::string dest_host = cs->getExecutionHosts()[1];
            try { // try a bogus one for coverage
              cs->migrateVM("NON-EXISTENT", dest_host);
              throw std::runtime_error("Should not be able to migrate a non-existent VM");
            } catch (std::invalid_argument &e) {
            }

            cs->migrateVM(vm_name, dest_host);

        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                // success, do nothing for now
                break;
            }
            default: {
                throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
        }

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, VirtualizedClusterVMMigrationTestWMS) {
    DO_TEST_WITH_FORK(do_VMMigrationTest_test);
}

void VirtualizedClusterServiceTest::do_VMMigrationTest_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("virtualized_cluster_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, 100.0)));


    // Create a Virtualized Cluster Service with no hosts
    std::vector<std::string> nothing;
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::VirtualizedClusterComputeService(hostname, nothing, 100.0,
                                                  {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,
                                                           "false"}})), std::invalid_argument);

    // Create a Virtualized Cluster Service
    std::vector<std::string> execution_hosts = simulation->getHostnameList();
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, 100.0,
                                                  {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,
                                                           "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new VirtualizedClusterVMMigrationTestWMS(this, {compute_service}, {storage_service}, hostname)));

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



/**********************************************************************/
/**  NUM CORES TEST                                                  **/
/**********************************************************************/

class CloudNumCoresTestWMS : public wrench::WMS {

public:
    CloudNumCoresTestWMS(VirtualizedClusterServiceTest *test,
                         const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                         const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                         std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
        try {

            // no VMs
            unsigned long sum_num_cores = this->test->compute_service->getTotalNumCores();

            unsigned long sum_num_idle_cores = this->test->compute_service->getTotalNumIdleCores();

            if (sum_num_cores != 6 || sum_num_idle_cores != 6) {
                throw std::runtime_error("getHostNumCores() and getNumIdleCores() should be 6 (they report " +
                                         std::to_string(sum_num_cores) + " and " + std::to_string(sum_num_idle_cores)+ ")");
            }

            // create and start VM with the 2  cores and 10 bytes of RAM
            auto cs = std::dynamic_pointer_cast<wrench::CloudComputeService>(this->test->compute_service);
            cs->startVM(cs->createVM(2, 10));

            sum_num_idle_cores = cs->getTotalNumIdleCores();

            if (sum_num_idle_cores != 4) {
                throw std::runtime_error("getNumIdleCores() should be 4 (it is reported as " + std::to_string(sum_num_idle_cores) + ")");
            }

            // create and start a VM with two cores
            cs->startVM(cs->createVM(2, 10));
            sum_num_idle_cores = cs->getTotalNumIdleCores();

            if (sum_num_idle_cores != 2) {
                throw std::runtime_error("getHostNumCores() and getNumIdleCores() should be 2 (it is reported as " + std::to_string(sum_num_idle_cores) + ")");
            }

        } catch (wrench::WorkflowExecutionException &e) {
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
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("cloud_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = simulation->getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, 100.0)));

    // Create a Cloud Service
    std::vector<std::string> execution_hosts = {"QuadCoreHost", "DualCoreHost"};
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::CloudComputeService(hostname, execution_hosts, 0,
                                     {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new CloudNumCoresTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  STOP ALL VMS TEST                                               **/
/**********************************************************************/

class StopAllVMsTestWMS : public wrench::WMS {

public:
    std::map<std::string, double> default_messagepayload_values = {
            {wrench::ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, 1024}
    };

    StopAllVMsTestWMS(VirtualizedClusterServiceTest *test,
                      const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                      const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

        this->test = test;
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
            auto cs = std::dynamic_pointer_cast<wrench::VirtualizedClusterComputeService>(this->test->compute_service);
            std::string execution_host = cs->getExecutionHosts()[0];

            cs->startVM(cs->createVM(1, 10));
            cs->startVM(cs->createVM(1, 10), execution_host);
            cs->startVM(cs->createVM(1, 10), execution_host);
            cs->startVM(cs->createVM(1, 10), execution_host);

        } catch (wrench::WorkflowExecutionException &e) {
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
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("virtualized_cluster_service_test");

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
            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, 0,
                                                  {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new StopAllVMsTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  VM SHUTDOWN, START, SUSPEND, RESUME TEST                        **/
/**********************************************************************/

class ShutdownVMTestWMS : public wrench::WMS {

public:
    std::map<std::string, double> default_messagepayload_values = {
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD,  1024},
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, 1024}
    };

    ShutdownVMTestWMS(VirtualizedClusterServiceTest *test,
                      const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                      const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

        this->test = test;
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
//      wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0.0, 60.0);

        std::vector<std::tuple<std::string, std::shared_ptr<wrench::BareMetalComputeService>>> vm_list;

        auto cs = std::dynamic_pointer_cast<wrench::VirtualizedClusterComputeService>(this->test->compute_service);


        // Create  and start VMs
        try {
            std::string execution_host = cs->getExecutionHosts()[0];
            for (int i=0; i < 4; i++) {
                auto vm_name = cs->createVM(1, 10);
                auto vm_cs = cs->startVM(vm_name);
                vm_list.push_back(std::make_tuple(vm_name, vm_cs));
            }
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        wrench::Simulation::sleep(1);
        // shutdown all VMs
        try {
            for (auto &vm : vm_list) {
                cs->shutdownVM(std::get<0>(vm));
            }
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Create a one-task job
        wrench::StandardJob *job = job_manager->createStandardJob(this->test->task1,
                                                                  {std::make_pair(this->test->input_file,
                                                                                  this->test->storage_service),
                                                                   std::make_pair(this->test->output_file1,
                                                                                  this->test->storage_service)});

        // Submit a job
        try {
            job_manager->submitJob(job, std::get<1>(*(vm_list.begin())));
            throw std::runtime_error("should have thrown an exception since there are no resources available");
        } catch (wrench::WorkflowExecutionException &e) {
            // do nothing, should have thrown an exception since there are no resources available
        }


        // (Re)start VM #3
        try {
            cs->startVM(std::get<0>(vm_list[3]));
        } catch (std::runtime_error &e) {
            throw std::runtime_error(e.what());
        }

        try {
            cs->suspendVM(std::get<0>(vm_list[3]));
            if (not cs->isVMSuspended(std::get<0>(vm_list[3]))) {
                throw std::runtime_error("A just suspended VM should be in the suspended state");
            }
            job_manager->submitJob(job, std::get<1>(vm_list[3]));
            throw std::runtime_error("should have thrown an exception since there are no resources available");
        } catch (wrench::WorkflowExecutionException &e) {
            // do nothing, should have thrown an exception since there are no resources available
        }

        try {
            cs->resumeVM(std::get<0>(vm_list[3]));
            if (not cs->isVMRunning(std::get<0>(vm_list[3]))) {
                throw std::runtime_error("A just resumed VM should be in the running state");
            }
        } catch (std::runtime_error &e) {
            throw std::runtime_error(e.what());
        }

        try {
            cs->resumeVM(std::get<0>(vm_list[3]));
            throw std::runtime_error("VM was already resumed, so an exception was expected!");
        } catch (wrench::WorkflowExecutionException &e) {
            // Expected
        }

        job_manager->submitJob(job, std::get<1>(vm_list[3]));

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                // success, do nothing for now
                break;
            }
            default: {
                throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
        }

        // Submit a job and suspend the VM before that job finishes
        wrench::StandardJob *other_job = job_manager->createStandardJob(this->test->task2,
                                                                        {std::make_pair(this->test->input_file,
                                                                                        this->test->storage_service),
                                                                         std::make_pair(this->test->output_file2,
                                                                                        this->test->storage_service)});

        try {
            WRENCH_INFO("Submitting a job");
            job_manager->submitJob(other_job, std::get<1>(vm_list[3]));
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Should be able to submit the other job");
        }
        double job_start_date = wrench::Simulation::getCurrentSimulatedDate();

        WRENCH_INFO("Sleeping for 5 seconds");
        wrench::Simulation::sleep(5);

        try {
            WRENCH_INFO("Suspending the one running VM (which is thus running the job)");
            cs->suspendVM(std::get<0>(vm_list[3]));
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Should be able to suspend VM");
        }


        WRENCH_INFO("Sleeping for 100 seconds");
        wrench::Simulation::sleep(100);

        try {
            WRENCH_INFO("Resuming the VM");
            cs->resumeVM(std::get<0>(vm_list[3]));
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Should be able to resume VM");
        }

        // Wait for a workflow execution event
        WRENCH_INFO("Waiting for job completion");
        std::unique_ptr<wrench::WorkflowExecutionEvent> event2;
        try {
            event2 = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event2->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                // success, do nothing for now
                break;
            }
            default: {
                throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event2->type)));
            }
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
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("virtualized_cluster_service_test");

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
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, 0,
                                                  {{wrench::VirtualizedClusterComputeServiceProperty::SUPPORTS_PILOT_JOBS, "true"}})), std::invalid_argument);

    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, 0,
                                                  {{wrench::VirtualizedClusterComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new ShutdownVMTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}



/**********************************************************************/
/**  VM START-SHUTDOWN, and then SERVICE SHUTDOWN                    **/
/**********************************************************************/

class ShutdownVMAndThenShutdownServiceTestWMS : public wrench::WMS {

public:
    std::map<std::string, double> default_messagepayload_values = {
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD,  1024},
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, 1024}
    };

    ShutdownVMAndThenShutdownServiceTestWMS(VirtualizedClusterServiceTest *test,
                                            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

        this->test = test;
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
        wrench::PilotJob *pilot_job = job_manager->createPilotJob();

        std::vector<std::tuple<std::string, std::shared_ptr<wrench::BareMetalComputeService>>> vm_list;

        auto cs = std::dynamic_pointer_cast<wrench::VirtualizedClusterComputeService>(this->test->compute_service);

        // Create VMs
        try {
            std::string execution_host = cs->getExecutionHosts()[0];

            for (int i=0; i < 4; i++) {
                WRENCH_INFO("CREATING A VM");
                auto vm_name = cs->createVM(1, 10);
                auto vm_cs = cs->startVM(vm_name, execution_host);
                vm_list.push_back(std::make_tuple(vm_name, vm_cs));
            }

        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        WRENCH_INFO("VMHAVE EEN CREATED");
        // shutdown some VMs
        try {
            WRENCH_INFO("SHUTTING DONW A VM");
            cs->shutdownVM(std::get<0>(vm_list.at(0)));
            WRENCH_INFO("SHUTTING DONW A VM");
            cs->shutdownVM(std::get<0>(vm_list.at(2)));
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        WRENCH_INFO("STOPPING SERVICE");
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
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("virtualized_cluster_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, 100.0)));

    // Create a Cloud Service
    std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, 0,
                                                  {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "false"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new ShutdownVMAndThenShutdownServiceTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  SUBMIT TO VM TEST                                               **/
/**********************************************************************/

class SubmitToVMTestWMS : public wrench::WMS {

public:
    std::map<std::string, double> default_messagepayload_values = {
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD,  1024},
            {wrench::VirtualizedClusterComputeServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, 1024}
    };

    SubmitToVMTestWMS(VirtualizedClusterServiceTest *test,
                      const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                      const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

        this->test = test;
        this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create standard jobs
        wrench::StandardJob *job1 = job_manager->createStandardJob({this->test->task1}, {},
                                                                   {std::make_tuple(this->test->input_file,
                                                                                    this->test->storage_service,
                                                                                    wrench::ComputeService::SCRATCH)},
                                                                   {}, {});
        wrench::StandardJob *job2 = job_manager->createStandardJob({this->test->task2}, {},
                                                                   {std::make_tuple(this->test->input_file,
                                                                                    this->test->storage_service,
                                                                                    wrench::ComputeService::SCRATCH)},
                                                                   {}, {});
        std::vector<std::tuple<std::string,std::shared_ptr<wrench::BareMetalComputeService>>> vm_list;

        auto cs = std::dynamic_pointer_cast<wrench::VirtualizedClusterComputeService>(this->test->compute_service);

        // Create some VMs
        try {
            std::string execution_host = cs->getExecutionHosts()[0];

            for (int i=0 ; i < 2; i++) {
                auto vm_name = cs->createVM(1, 10);
                auto vm_cs = cs->startVM(vm_name, execution_host);
                vm_list.push_back(std::make_tuple(vm_name, vm_cs));
            }

        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        try {
            for (auto &vm : vm_list) {
                cs->shutdownVM(std::get<0>(vm));
            }
            // Trying to submit to a VM that has been shutdown
            job_manager->submitJob(job1, std::get<1>(vm_list[0]));
            throw std::runtime_error("Should not be able to run job since VMs are stopped");
        } catch (wrench::WorkflowExecutionException &e) {
            if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
                throw std::runtime_error("Invalid failure cause (should be SERVICE_DOWN");
            }
            // do nothing, expected behavior
        }

        try {
            cs->startVM(std::get<0>(vm_list[1]));
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Couldn't start VM: " + e.getCause()->toString());
        }

        try {
            job_manager->submitJob(job1, std::get<1>(vm_list[1]));
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
                // success, do nothing for now
                break;
            }
            default: {
                throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
        }


        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, SubmitToVMTestWMS) {
    DO_TEST_WITH_FORK(do_SubmitToVMTest_test);
}

void VirtualizedClusterServiceTest::do_SubmitToVMTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("virtualized_cluster_service_test");

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
            new wrench::VirtualizedClusterComputeService(hostname, execution_hosts, 1000)));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new SubmitToVMTestWMS(this, {compute_service}, {storage_service}, hostname)));

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

/**********************************************************************/
/**                VM START-SHUTDOWN-START-SHUTDOWN                  **/
/**********************************************************************/

class CloudServiceVMStartShutdownStartShutdownTestWMS : public wrench::WMS {

public:
    CloudServiceVMStartShutdownStartShutdownTestWMS(VirtualizedClusterServiceTest *test,
                                                    std::string &hostname, std::shared_ptr<wrench::ComputeService> cs, std::shared_ptr<wrench::StorageService>ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() override {

        auto cloud_service = std::dynamic_pointer_cast<wrench::CloudComputeService>(this->test->compute_service);

        // Create a VM on the Cloud Service
        auto vm_name = cloud_service->createVM(2, 1024);

        // Start the VM
        wrench::Simulation::sleep(10);
        WRENCH_INFO("STARTING THE VM");
        auto vm_cs = cloud_service->startVM(vm_name);

        // Shutdown the VM
        wrench::Simulation::sleep(10);
        WRENCH_INFO("SHUTTING DOWN THE VM");
        cloud_service->shutdownVM(vm_name);

        // Start the VM
        wrench::Simulation::sleep(10);
        WRENCH_INFO("STARTING THE VM");
        vm_cs = cloud_service->startVM(vm_name);

        // Shutdown the VM
        wrench::Simulation::sleep(10);
        WRENCH_INFO("SHUTTING DOWN THE VM");
        cloud_service->shutdownVM(vm_name);

        // Destroying the VM
        wrench::Simulation::sleep(10);
        WRENCH_INFO("DESTROYING  THE VM");
        cloud_service->destroyVM(vm_name);

        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, VMStartShutdownStartShutdown) {
    DO_TEST_WITH_FORK(do_VMStartShutdownStartShutdown_test);
}

void VirtualizedClusterServiceTest::do_VMStartShutdownStartShutdown_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "DualCoreHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("QuadCoreHost");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudComputeService(hostname,
                                     compute_hosts,
                                     100.0,
                                     {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(hostname, 10000000000000.0));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new CloudServiceVMStartShutdownStartShutdownTestWMS(this, hostname, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(hostname));
    simulation->stageFiles({{input_file->getID(), input_file}}, storage_service);

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**                VM SHUTDOWN WHILE JOB IS RUNNING                  **/
/**********************************************************************/

class CloudServiceVMShutdownWhileJobIsRunningTestWMS : public wrench::WMS {

public:
    CloudServiceVMShutdownWhileJobIsRunningTestWMS(VirtualizedClusterServiceTest *test,
                                                    std::string &hostname, std::shared_ptr<wrench::ComputeService> cs, std::shared_ptr<wrench::StorageService>ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() override {

        auto cloud_service = std::dynamic_pointer_cast<wrench::CloudComputeService>(this->test->compute_service);

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        wrench::StandardJob *job = job_manager->createStandardJob({this->test->task1}, {},
                                                                   {std::make_tuple(this->test->input_file,
                                                                                    this->test->storage_service,
                                                                                    wrench::ComputeService::SCRATCH)},
                                                                   {}, {});

        // Create a VM on the Cloud Service
        auto vm_name = cloud_service->createVM(2, 1024);

        // Start the VM
        wrench::Simulation::sleep(10);
        auto vm_cs = cloud_service->startVM(vm_name);

        // Submit the job to it
        job_manager->submitJob(job, vm_cs);

        wrench::Simulation::sleep(10);

        // Shutdown the VM
        cloud_service->shutdownVM(vm_name);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE) {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)) + " (should be STANDARD_JOB_FAILURE)");
        }

        auto real_event = dynamic_cast<wrench::StandardJobFailedEvent *>(event.get());
        auto failure_cause = real_event->failure_cause;
        if (failure_cause->getCauseType() != wrench::FailureCause::JOB_KILLED) {
            throw std::runtime_error("Unexpected failure cause type: " + std::to_string((int) (failure_cause->getCauseType())) + " (should be JOB_KILLED)");
        }
        auto real_failure_cause = dynamic_cast<wrench::JobKilled *>(failure_cause.get());
        if (real_failure_cause->getJob() != job) {
            throw std::runtime_error("Failure cause does not point to the correct job");
        }
        if (real_failure_cause->getComputeService() != vm_cs) {
            throw std::runtime_error("Failure cause does not point to the correst compute service");
        }


        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, VMShutdownWhileJobIsRunning) {
    DO_TEST_WITH_FORK(do_VMShutdownWhileJobIsRunning_test);
}

void VirtualizedClusterServiceTest::do_VMShutdownWhileJobIsRunning_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "DualCoreHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("QuadCoreHost");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudComputeService(hostname,
                                     compute_hosts,
                                     100.0,
                                     {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(hostname, 10000000000000.0));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new CloudServiceVMShutdownWhileJobIsRunningTestWMS(this, hostname, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(hostname));
    simulation->stageFiles({{input_file->getID(), input_file}}, storage_service);

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}



/**********************************************************************/
/**   VM COMPUTE SERVICE STOP SHUTDOWN WHILE JOB IS RUNNING          **/
/**********************************************************************/

class CloudServiceVMComputeServiceStopWhileJobIsRunningTestWMS : public wrench::WMS {

public:
    CloudServiceVMComputeServiceStopWhileJobIsRunningTestWMS(VirtualizedClusterServiceTest *test,
                                                   std::string &hostname, std::shared_ptr<wrench::ComputeService> cs, std::shared_ptr<wrench::StorageService> ss) :
            wrench::WMS(nullptr, nullptr, {cs}, {ss}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() override {

        auto cloud_service = std::dynamic_pointer_cast<wrench::CloudComputeService>(this->test->compute_service);

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a job
        wrench::StandardJob *job = job_manager->createStandardJob({this->test->task1}, {},
                                                                  {std::make_tuple(this->test->input_file,
                                                                                   this->test->storage_service,
                                                                                   wrench::ComputeService::SCRATCH)},
                                                                  {}, {});

        // Create a VM on the Cloud Service
        auto vm_name = cloud_service->createVM(2, 1024);

        // Start the VM
        wrench::Simulation::sleep(10);
        auto vm_cs = cloud_service->startVM(vm_name);

        // Submit the job to it
        job_manager->submitJob(job, vm_cs);

        wrench::Simulation::sleep(10);

        // Stop the VM Compute Service
        vm_cs->stop();

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE) {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)) + " (should be STANDARD_JOB_FAILURE)");
        }

        auto real_event = dynamic_cast<wrench::StandardJobFailedEvent *>(event.get());
        auto failure_cause = real_event->failure_cause;
        if (failure_cause->getCauseType() != wrench::FailureCause::JOB_KILLED) {
            throw std::runtime_error("Unexpected failure cause type: " + std::to_string((int) (failure_cause->getCauseType())) + " (should be JOB_KILLED)");
        }
        auto real_failure_cause = dynamic_cast<wrench::JobKilled *>(failure_cause.get());
        if (real_failure_cause->getJob() != job) {
            throw std::runtime_error("Failure cause does not point to the correct job");
        }
        if (real_failure_cause->getComputeService() != vm_cs) {
            throw std::runtime_error("Failure cause does not point to the correct compute service");
        }

        // Check that the VM is down, as it should
        if (not cloud_service->isVMDown(vm_name)) {
            throw std::runtime_error("VM should be down after its BareMetalComputeService has been stopped");
        }



        return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, VMComputeServiceStopWhileJobIsRunning) {
    DO_TEST_WITH_FORK(do_VMComputeServiceStopWhileJobIsRunning_test);
}

void VirtualizedClusterServiceTest::do_VMComputeServiceStopWhileJobIsRunning_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    simulation->init(&argc, argv);

    // Setting up the platform
    simulation->instantiatePlatform(platform_file_path);

    // Get a hostname
    std::string hostname = "DualCoreHost";
    std::vector<std::string> compute_hosts;
    compute_hosts.push_back("QuadCoreHost");

    // Create a Compute Service that has access to two hosts
    compute_service = simulation->add(
            new wrench::CloudComputeService(hostname,
                                     compute_hosts,
                                     100.0,
                                     {}));

    // Create a Storage Service
    storage_service = simulation->add(new wrench::SimpleStorageService(hostname, 10000000000000.0));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    wms = simulation->add(new CloudServiceVMComputeServiceStopWhileJobIsRunningTestWMS(this, hostname, compute_service, storage_service));

    wms->addWorkflow(workflow);

    // Staging the input_file on the storage service
    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(hostname));
    simulation->stageFiles({{input_file->getID(), input_file}}, storage_service);

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

