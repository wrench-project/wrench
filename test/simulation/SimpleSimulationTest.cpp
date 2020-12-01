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

class SimpleSimulationTest : public ::testing::Test {

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
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    void do_getReadyTasksTest_test();

protected:

    SimpleSimulationTest() {
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
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\" > "
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

class SimpleSimulationReadyTasksTestWMS : public wrench::WMS {

public:
    SimpleSimulationReadyTasksTestWMS(SimpleSimulationTest *test,
                                      const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                      const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    SimpleSimulationTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get the scheduler pointers just for coverage
        // Get the scheduler pointers just for coverage
        if (this->getPilotJobScheduler() != nullptr) {
            throw std::runtime_error("getPilotJobScheduler() should return nullptr");
        }
        if (this->getStandardJobScheduler() != nullptr) {
            throw std::runtime_error("getStandardJobScheduler() should return nullptr");
        }

        // Get a file registry service
        auto file_registry_service = this->getAvailableFileRegistryService();

        std::vector<wrench::WorkflowTask *> tasks = this->test->workflow->getReadyTasks();
        if (tasks.size() != 5) {
            throw std::runtime_error("Should have five tasks ready to run, due to dependencies");
        }

        std::map<std::string, std::vector<wrench::WorkflowTask *>> clustered_tasks = this->test->workflow->getReadyClusters();
        if (clustered_tasks.size() != 3) {
            throw std::runtime_error("Should have exactly three clusters");
        }

        // Create a bogus standard job with an empty task list for coverage
        try {
            job_manager->createStandardJob({}, {});
            throw std::runtime_error("Should not be able to create a job with an empty task list");
        } catch (std::invalid_argument &e) {
        }

        // Get pointer to cloud service
        auto cs = *(this->getAvailableComputeServices<wrench::CloudComputeService>().begin());

        // Create a bogus VM
        try {
            cs->createVM(1000,10000000);
            throw std::runtime_error("Should not be able to create a VM that exceeds the capacity of all hosts on the service");
        } catch (wrench::WorkflowExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Unexpected failure cause: " + e.getCause()->toString() +
                                         "(Was expecting NotEnoughResources");
            }
        }

        // Create a 2-core VM
        auto vm_name = cs->createVM(2, 10);

        // Start the VM
        auto vm_cs = cs->startVM(vm_name);
        // Try to, wrongly, start it again
        try {
            cs->startVM(vm_name);
            throw std::runtime_error("Should not be able to start an already-started VM");
        } catch (wrench::WorkflowExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NotAllowed>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Unexpected Failure Cause " + e.getCause()->toString() +
                                         " (Expected: NotAllowed");
            }
        }

        std::shared_ptr<wrench::StandardJob> one_task_jobs[5];
        int job_index = 0;
        for (auto task : tasks) {
            try {
                one_task_jobs[job_index] = job_manager->createStandardJob({task}, {{this->test->input_file,
                                                                                    wrench::FileLocation::LOCATION(this->test->storage_service)}},
                                                                          {}, {}, {});

                if (one_task_jobs[job_index]->getNumTasks() != 1) {
                    throw std::runtime_error("A one-task job should say it has one task");
                }
                if (one_task_jobs[job_index]->getNumCompletedTasks() != 0) {
                    throw std::runtime_error("A one-task job that hasn't even started should not say it has a completed task");
                }

                job_manager->submitJob(one_task_jobs[job_index], vm_cs);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(e.what());
            }

            // Get the job's service-specific arguments (coverage)
            one_task_jobs[job_index]->getServiceSpecificArguments();

            // Get the job submit date
            double job_submit_date = one_task_jobs[job_index]->getSubmitDate();
            if (wrench::Simulation::getCurrentSimulatedDate() - job_submit_date > 1.0) {
                throw std::runtime_error("Job submit date is likely wrong");
            }

            job_index++;
        }

        // Try to create and submit a job with tasks that are pending, which should fail
        auto bogus_job = job_manager->createStandardJob({*(tasks.begin())}, {}, {}, {}, {});
        try {
            job_manager->submitJob(bogus_job, vm_cs);
            throw std::runtime_error("Should not be able to create a job with PENDING tasks");
        } catch (std::invalid_argument &e) {
        }

        // For coverage
        wrench::Simulation::getLinknameList();
        wrench::Simulation::getLinkBandwidth("1");
        wrench::Simulation::getLinkUsage("1");
        // Wait for workflow execution events
        for (auto const & task : tasks) {
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

        for (int i = 0; i < 5; i++) {
            if (one_task_jobs[i]->getNumCompletedTasks() != 1) {
                throw std::runtime_error("A job with one completed task should say it has one completed task");
            }
        }

        {
            // Try to create and submit a job with tasks that are completed, which should fail
            auto bogus_job = job_manager->createStandardJob({*(++tasks.begin())}, {}, {}, {}, {});
            try {
                job_manager->submitJob(bogus_job, vm_cs);
                throw std::runtime_error("Should not be able to create a job with PENDING tasks");
            } catch (std::invalid_argument &e) {
            }
        }

        // For coverage,
        unsigned long num_cores = wrench::Simulation::getNumCores();
        if (num_cores != 2) {
            throw std::runtime_error("Unexpected number of cores!");
        }
        double flop_rate = wrench::Simulation::getFlopRate();
        if (flop_rate != 1.0) {
            throw std::runtime_error("Unexpected flop rate");
        }
        wrench::Simulation::compute(1 / flop_rate);

        std::map<std::string, std::vector<std::string>> clusters =
                wrench::Simulation::getHostnameListByCluster();

        cs->suspend();
        cs->suspend();
        cs->resume();
        cs->stop();
        cs->suspend();

        try {
            cs->resume();
            throw std::runtime_error("Should not be able to resume a service that's down");
        } catch (wrench::WorkflowExecutionException &e) {
        }

        data_movement_manager->kill();
        job_manager->kill();



        return 0;
    }
};

TEST_F(SimpleSimulationTest, SimpleSimulationReadyTasksTestWMS) {
    DO_TEST_WITH_FORK(do_getReadyTasksTest_test);
}

void SimpleSimulationTest::do_getReadyTasksTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");


    // Adding services to an uninitialized simulation
    std::vector<std::string> hosts = {"DualCoreHost", "QuadCoreHost"};
    ASSERT_THROW(simulation->add(
            new wrench::CloudComputeService("DualCoreHost", hosts, "/scratch")), std::runtime_error);
    ASSERT_THROW(simulation->add(
            new wrench::SimpleStorageService("DualCoreHost", {"/"})), std::runtime_error);
    ASSERT_THROW(simulation->add(
            new wrench::NetworkProximityService("DualCoreHost", hosts)), std::runtime_error);
    ASSERT_THROW(simulation->add(
            new wrench::FileRegistryService("DualCoreHost")), std::runtime_error);

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "DualCoreHost";

    // Create a Storage Service
    ASSERT_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"}, {},
                                             {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, -1}})), std::invalid_argument);
    storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2"}, {},
                                             {{wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD, 123}}));


    // Try to get a bogus property as string or double
    ASSERT_THROW(storage_service->getPropertyValueAsString("BOGUS"), std::invalid_argument);
    ASSERT_THROW(storage_service->getPropertyValueAsDouble("BOGUS"), std::invalid_argument);
    ASSERT_THROW(storage_service->getPropertyValueAsUnsignedLong("BOGUS"), std::invalid_argument);
    ASSERT_THROW(storage_service->getPropertyValueAsBoolean("BOGUS"), std::invalid_argument);

    ASSERT_THROW(storage_service->getMessagePayloadValue("BOGUS"), std::invalid_argument);
    ASSERT_EQ(123, storage_service->getMessagePayloadValue(
            wrench::SimpleStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD));

    // Create a Cloud Service
    std::vector<std::string> execution_hosts = {"QuadCoreHost"};
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::CloudComputeService(hostname, execution_hosts, "/scratch",
                                            { {wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                                              {wrench::BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD, "0"}
                                              })));

    // Try to get the property in bogus ways, for coverage
    ASSERT_THROW(compute_service->getPropertyValueAsDouble(wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS), std::invalid_argument);
    ASSERT_THROW(compute_service->getPropertyValueAsUnsignedLong(wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS), std::invalid_argument);
    ASSERT_THROW(compute_service->getPropertyValueAsBoolean(wrench::BareMetalComputeServiceProperty::TASK_STARTUP_OVERHEAD), std::invalid_argument);

    // Try to get a message payload value, just for kicks
    ASSERT_NO_THROW(compute_service->getMessagePayloadValue(wrench::ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new SimpleSimulationReadyTasksTestWMS(this, {compute_service}, {storage_service}, hostname)));


    // BOGUS ADDS
    ASSERT_THROW(simulation->add((wrench::WMS *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->add((wrench::StorageService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->add((wrench::NetworkProximityService *) nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->add((wrench::FileRegistryService *) nullptr), std::invalid_argument);

    // Won't work without a workflow!
    ASSERT_THROW(simulation->launch(), std::runtime_error);

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Won't work due to missing file staging
    ASSERT_THROW(simulation->launch(), std::runtime_error);


    // Try to stage a file without a file registry
    ASSERT_THROW(simulation->stageFile(input_file, storage_service), std::runtime_error);

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(hostname)));

    // Set the file registry's timeout value
    file_registry_service->setNetworkTimeoutValue(100.0);
    file_registry_service->getNetworkTimeoutValue();

    // Staging an invalid file on the storage service
    ASSERT_THROW(simulation->stageFile(output_file1, storage_service), std::runtime_error);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


