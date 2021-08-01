/**
 * Copyright (c) 2018-2019. The WRENCH Team.
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

WRENCH_LOG_CATEGORY(htcondor_service_test, "Log category for HTCondorServiceTest");


class HTCondorServiceTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *input_file2;
    wrench::WorkflowFile *input_file3;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;
    wrench::WorkflowTask *task5;
    wrench::WorkflowTask *task6;
    wrench::WorkflowTask *task7;
    //wrench::WorkflowTask *task8;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;

    void do_StandardJobTaskTest_test();
    void do_PilotJobTaskTest_test();
    void do_SimpleServiceTest_test();
    void do_GridUniverseTest_test();
    void do_NoStandardJobSupportTest_test();
    void do_NoGridUniverseSupportTest_test();
    void do_NoPilotJobSupportTest_test();

protected:
    HTCondorServiceTest() {

        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        //Creating separate workflow for grid universe
        workflow_unique_ptr1 = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        grid_workflow = workflow_unique_ptr1.get();

        // Create the files
        input_file = workflow->addFile("input_file", 10.0);
        input_file2 = grid_workflow->addFile("input_file2", 6500000000.0);
        input_file3 = grid_workflow->addFile("input_file3", 10.0);
        output_file1 = workflow->addFile("output_file1", 10.0);
        output_file2 = workflow->addFile("output_file2", 10.0);
        output_file3 = workflow->addFile("output_file3", 10.0);

        // Create the tasks
        task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 0);
        task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 0);
        task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 0);
        task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 0);
        task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 0);
        task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 0);
        task7 = grid_workflow->addTask("grid_task1", 10.0, 1, 1, 0);
        //task8 = grid_workflow->addTask("grid_task2", 10.0, 1, 1, 0);


        // Add file-task dependencies
        task1->addInputFile(input_file);
        task2->addInputFile(input_file);
        task3->addInputFile(input_file);
        task4->addInputFile(input_file);
        task5->addInputFile(input_file);
        task6->addInputFile(input_file);
        task7->addInputFile(input_file2);
        //task8->addInputFile(input_file3);

        task1->addOutputFile(output_file1);


        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000000B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000000B\"/>"
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

        //Grid Universe Platform file.
        std::string xml1 = "<?xml version='1.0'?>"
                           "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                           "<platform version=\"4.1\"> "
                           "   <zone id=\"AS0\" routing=\"Full\"> "
                           "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\" > "
                           "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"100GB\"/>"
                           "             <prop id=\"mount\" value=\"/\"/>"
                           "          </disk>"
                           "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"1000000GB\"/>"
                           "             <prop id=\"mount\" value=\"/scratch\"/>"
                           "          </disk>"
                           "       </host>"
                           "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\" > "
                           "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"100GB\"/>"
                           "             <prop id=\"mount\" value=\"/\"/>"
                           "          </disk>"
                           "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"1000000GB\"/>"
                           "             <prop id=\"mount\" value=\"/scratch\"/>"
                           "          </disk>"
                           "       </host>"
                           "       <host id=\"BatchHost1\" speed=\"1f\" core=\"10\"> "
                           "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"100GB\"/>"
                           "             <prop id=\"mount\" value=\"/\"/>"
                           "          </disk>"
                           "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"1000000GB\"/>"
                           "             <prop id=\"mount\" value=\"/scratch\"/>"
                           "          </disk>"
                           "       </host>"
                           "       <host id=\"BatchHost2\" speed=\"1f\" core=\"10\"> "
                           "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"100GB\"/>"
                           "             <prop id=\"mount\" value=\"/\"/>"
                           "          </disk>"
                           "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                           "             <prop id=\"size\" value=\"1000000GB\"/>"
                           "             <prop id=\"mount\" value=\"/scratch\"/>"
                           "          </disk>"
                           "       </host>"
                           "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                           "       <link id=\"2\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                           "       <link id=\"3\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                           "       <link id=\"4\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                           "       <link id=\"5\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                           "       <link id=\"6\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                           "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
                           "       <route src=\"BatchHost1\" dst=\"BatchHost2\"> <link_ctn id=\"2\"/> </route>"
                           "       <route src=\"DualCoreHost\" dst=\"BatchHost1\"> <link_ctn id=\"3\"/> </route>"
                           "       <route src=\"DualCoreHost\" dst=\"BatchHost2\"> <link_ctn id=\"4\"/> </route>"
                           "       <route src=\"QuadCoreHost\" dst=\"BatchHost1\"> <link_ctn id=\"5\"/> </route>"
                           "       <route src=\"QuadCoreHost\" dst=\"BatchHost2\"> <link_ctn id=\"6\"/> </route>"
                           "   </zone> "
                           "</platform>";

        FILE *platform_file1 = fopen(platform_file_path1.c_str(), "w");
        fprintf(platform_file1, "%s", xml1.c_str());
        fclose(platform_file1);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::string platform_file_path1 = UNIQUE_TMP_PATH_PREFIX + "platform1.xml";
    wrench::Workflow *workflow;
    wrench::Workflow *grid_workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr1;
};

/**********************************************************************/
/**  STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class HTCondorStandardJobTestWMS : public wrench::WMS {

public:
    HTCondorStandardJobTestWMS(HTCondorServiceTest *test,
                               const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                               const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                               std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob(
                {this->test->task1},
                {},
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(this->test->storage_service),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});


        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(two_task_job, this->test->compute_service);
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        return 0;
    }
};

TEST_F(HTCondorServiceTest, HTCondorStandardJobTest) {
    DO_TEST_WITH_FORK(do_StandardJobTaskTest_test);
}

void HTCondorServiceTest::do_StandardJobTaskTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
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

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;
    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    // Create a HTCondor Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(compute_services),
                    {
                        {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                        {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                     })));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new HTCondorStandardJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  PILOT JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class HTCondorPilotJobTestWMS : public wrench::WMS {

public:
    HTCondorPilotJobTestWMS(HTCondorServiceTest *test,
                            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Submit a pilot job
        auto pilot_job = job_manager->createPilotJob();
        std::map<std::string, std::string> service_specific_arguments;
        service_specific_arguments["-N"] = "1";
        service_specific_arguments["-c"] = "1";
        service_specific_arguments["-t"] = "3600";

        job_manager->submitJob(pilot_job, this->test->compute_service, service_specific_arguments);

        {
            // Wait for the job start
            auto event = this->getWorkflow()->waitForNextExecutionEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Did not get the expected PilotJobStartEvent (got " + event->toString() + ")");
            }
        }

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob(
                {this->test->task1},
                {},
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(this->test->storage_service),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(two_task_job, pilot_job->getComputeService());
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        return 0;
    }
};

TEST_F(HTCondorServiceTest, HTCondorPilotJobTest) {
    DO_TEST_WITH_FORK(do_PilotJobTaskTest_test);
}

void HTCondorServiceTest::do_PilotJobTaskTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
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

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;
    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    // Create a HTCondor Service
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(hostname, "local", std::move(compute_services),
                                               {{wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "true"}})),
                 std::invalid_argument);

    compute_services.insert(new wrench::BatchComputeService(execution_host,
                                                            {execution_host},
                                                            "/scratch"));

    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(hostname, "local", std::move(compute_services),
                                               {{wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "true"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new HTCondorPilotJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  SIMPLE SERVICE TESTS                                            **/
/**********************************************************************/

class HTCondorSimpleServiceTestWMS : public wrench::WMS {

public:
    HTCondorSimpleServiceTestWMS(HTCondorServiceTest *test,
                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                 const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                 std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());
        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob(
                {this->test->task1}, {},
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(htcondor_cs->getLocalStorageService()),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(two_task_job, this->test->compute_service);
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        this->test->compute_service->stop();

        return 0;
    }
};

TEST_F(HTCondorServiceTest, HTCondorSimpleServiceTest) {
    DO_TEST_WITH_FORK(do_SimpleServiceTest_test);
}

void HTCondorServiceTest::do_SimpleServiceTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(hostname, {"/"})));

    {
        // Create list of invalid compute services
        std::set<wrench::ComputeService *> invalid_compute_services;
        std::string execution_host = wrench::Simulation::getHostnameList()[1];
        std::vector<std::string> execution_hosts;
        execution_hosts.push_back(execution_host);
        invalid_compute_services.insert(new wrench::CloudComputeService(hostname, execution_hosts,
                                                                        "/scratch"));

        // Create a HTCondor Service
        ASSERT_THROW(simulation->add(new wrench::HTCondorComputeService(hostname, "", {})), std::runtime_error);
        ASSERT_THROW(compute_service = simulation->add(
                new wrench::HTCondorComputeService(hostname, "local", std::move(invalid_compute_services),
                                                   {{wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})),
                     std::invalid_argument);
    }

    //Create manager w/o compute services, it will fail.
    ASSERT_THROW(auto test = new wrench::HTCondorCentralManagerService(hostname, {}),std::invalid_argument);

    // Create list of valid compute services
    std::set<wrench::ComputeService *> compute_services;
    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::VirtualizedClusterComputeService(hostname, execution_hosts,
                                                                         "/scratch"));

    // Create a HTCondor Service
    ASSERT_THROW(simulation->add(new wrench::HTCondorComputeService(hostname, "", {})), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(hostname, "local", std::move(compute_services),
                                               {{wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service)->setLocalStorageService(storage_service);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new HTCondorSimpleServiceTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**          STANDARD JOB SUBMISSION TEST FOR GRID UNIVERSE          **/
/**********************************************************************/

class HTCondorGridUniverseTestWMS : public wrench::WMS {

public:
    HTCondorGridUniverseTestWMS(HTCondorServiceTest *test,
                               const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                               const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                               std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();


        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());
        std::shared_ptr<wrench::StandardJob>grid_job = job_manager->createStandardJob(
                {this->test->task7},
                {},
                {std::make_tuple(this->test->input_file2,
                                 wrench::FileLocation::LOCATION(htcondor_cs->getLocalStorageService()),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});


        std::map<std::string, std::string> test_service_specs;
        test_service_specs.insert(std::pair<std::string, std::string>("universe","grid"));

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(grid_job, this->test->compute_service, test_service_specs);
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        this->test->compute_service->stop();

        return 0;
    }
};

TEST_F(HTCondorServiceTest, HTCondorGridUniverseTest) {
DO_TEST_WITH_FORK(do_GridUniverseTest_test);
}

void HTCondorServiceTest::do_GridUniverseTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path1));


    // Get a hostname
    std::string hostname = "DualCoreHost";
    //std::string batchhostname = "BatchHost1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    //ASSERT_NO_THROW(storage_service2 = simulation->add(
    //       new wrench::SimpleStorageService(batchhostname, {"/"})));

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;
    std::set<wrench::ComputeService *> test1_cs;
    std::set<wrench::ComputeService *> test2_cs;
    std::set<wrench::ComputeService *> empty_services;

    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    test1_cs.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    test2_cs.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));





    auto batch_service = new wrench::BatchComputeService("BatchHost1",
                                                         {"BatchHost1", "BatchHost2"},
                                                         "/scratch",
                                                         {
                                                                        {wrench::BatchComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                                                                        });




    // Create a Grid universe HTCondor Service w/o batch service
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(test1_cs),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                    },
                    {},
                    {})), std::invalid_argument);

    //Test deconstructor.
    ASSERT_NO_THROW(auto *test_compute = new wrench::HTCondorComputeService(
            hostname, "local", std::move(test2_cs),
            {
                    {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                    {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                    {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
            },
            {},
            batch_service);

            delete test_compute);

    //supports standard jobs w/ no compute service that does.
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(empty_services),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                    },
                    {},
                    {})), std::invalid_argument);

    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(compute_services),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                    },
                    {},
                    batch_service)));



    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service)->setLocalStorageService(storage_service);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new HTCondorGridUniverseTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(grid_workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file2, storage_service));
    ASSERT_NO_THROW(simulation->stageFile(input_file3, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

/**************************************************************************************/
/**          STANDARD JOB SUBMISSION TEST WHERE STANDARD JOBS NOT SUPPORTED          **/
/**************************************************************************************/

class HTCondorNoStandardJobTestWMS : public wrench::WMS {

public:
    HTCondorNoStandardJobTestWMS(HTCondorServiceTest *test,
                                const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();


        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());
        std::shared_ptr<wrench::StandardJob>grid_job = job_manager->createStandardJob(
                {this->test->task7},
                {},
                {std::make_tuple(this->test->input_file2,
                                 wrench::FileLocation::LOCATION(htcondor_cs->getLocalStorageService()),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});


        std::map<std::string, std::string> test_service_specs;
        test_service_specs.insert(std::pair<std::string, std::string>("universe","grid"));

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(grid_job, this->test->compute_service, test_service_specs);
        } catch (wrench::WorkflowExecutionException &e) {
            return 0;
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        this->test->compute_service->stop();

        return 1;
    }
};

TEST_F(HTCondorServiceTest, NoStandardJobSupportTest) {
    DO_TEST_WITH_FORK(do_NoStandardJobSupportTest_test);
}

void HTCondorServiceTest::do_NoStandardJobSupportTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path1));


    // Get a hostname
    std::string hostname = "DualCoreHost";
    //std::string batchhostname = "BatchHost1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    //ASSERT_NO_THROW(storage_service2 = simulation->add(
    //       new wrench::SimpleStorageService(batchhostname, {"/"})));

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;

    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));







    auto batch_service = new wrench::BatchComputeService("BatchHost1",
                                                         {"BatchHost1", "BatchHost2"},
                                                         "/scratch",
                                                         {
                                                                 {wrench::BatchComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                                                         });





    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(compute_services),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                    },
                    {},
                    batch_service)));



    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service)->setLocalStorageService(storage_service);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new HTCondorNoStandardJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(grid_workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file2, storage_service));
    ASSERT_NO_THROW(simulation->stageFile(input_file3, storage_service));

    // Fails because it will try to run a standard job on a system that does not support standard jobs.
    // WMS is set to return 0 if it fails as intended.
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


/************************************************************************************************/
/**          GRID UNIVERSE JOB SUBMISSION TEST WHERE GRID UNIVERSE JOBS NOT SUPPORTED          **/
/************************************************************************************************/

class HTCondorNoGridUniverseJobTestWMS : public wrench::WMS {

public:
    HTCondorNoGridUniverseJobTestWMS(HTCondorServiceTest *test,
                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                 const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                 std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();


        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());
        std::shared_ptr<wrench::StandardJob>grid_job = job_manager->createStandardJob(
                {this->test->task7},
                {},
                {std::make_tuple(this->test->input_file2,
                                 wrench::FileLocation::LOCATION(htcondor_cs->getLocalStorageService()),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});


        std::map<std::string, std::string> test_service_specs;
        test_service_specs.insert(std::pair<std::string, std::string>("universe","grid"));

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(grid_job, this->test->compute_service, test_service_specs);
        } catch (wrench::WorkflowExecutionException &e) {
            return 0;
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        this->test->compute_service->stop();

        return 1;
    }
};

TEST_F(HTCondorServiceTest, NoGridUniverseSupportTest) {
    DO_TEST_WITH_FORK(do_NoGridUniverseSupportTest_test);
}

void HTCondorServiceTest::do_NoGridUniverseSupportTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path1));


    // Get a hostname
    std::string hostname = "DualCoreHost";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;

    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    auto batch_service = new wrench::BatchComputeService("BatchHost1",
                                                         {"BatchHost1", "BatchHost2"},
                                                         "/scratch",
                                                         {
                                                                 {wrench::BatchComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                                                         });

    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(compute_services),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "false"},
                    },
                    {},
                    batch_service)));



    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service)->setLocalStorageService(storage_service);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new HTCondorNoGridUniverseJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(grid_workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file2, storage_service));
    ASSERT_NO_THROW(simulation->stageFile(input_file3, storage_service));

    // Fails because it will try to run a grid job on a system that does not support standard jobs, WMS is set up to
    //return 0 in this case and 1 if it successfully completes.
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

/************************************************************************************************/
/**          PILOT JOB SUBMISSION TEST WHERE PILOT JOBS NOT SUPPORTED                          **/
/************************************************************************************************/

class HTCondorNoPilotJobTestWMS : public wrench::WMS {

public:
    HTCondorNoPilotJobTestWMS(HTCondorServiceTest *test,
                                     const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                     const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                     std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();


        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());
        std::shared_ptr<wrench::PilotJob>grid_job = job_manager->createPilotJob();


        std::map<std::string, std::string> test_service_specs;
        test_service_specs.insert(std::pair<std::string, std::string>("universe","grid"));

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(grid_job, this->test->compute_service, test_service_specs);
        } catch (wrench::WorkflowExecutionException &e) {
            return 0;
            throw std::runtime_error(e.what());
        }

        // Wait for a workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        this->test->compute_service->stop();

        return 1;
    }
};

TEST_F(HTCondorServiceTest, NoPilotJobSupportTest) {
    DO_TEST_WITH_FORK(do_NoPilotJobSupportTest_test);
}

void HTCondorServiceTest::do_NoPilotJobSupportTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path1));


    // Get a hostname
    std::string hostname = "DualCoreHost";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create list of compute services
    std::set<wrench::ComputeService *> compute_services;

    std::string execution_host = wrench::Simulation::getHostnameList()[1];
    std::vector<std::string> execution_hosts;
    execution_hosts.push_back(execution_host);
    compute_services.insert(new wrench::BareMetalComputeService(
            execution_host,
            {std::make_pair(
                    execution_host,
                    std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                    wrench::Simulation::getHostMemoryCapacity(execution_host)))},
            "/scratch"));

    auto batch_service = new wrench::BatchComputeService("BatchHost1",
                                                         {"BatchHost1", "BatchHost2"},
                                                         "/scratch",
                                                         {
                                                                 {wrench::BatchComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "true"},
                                                         });

    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::HTCondorComputeService(
                    hostname, "local", std::move(compute_services),
                    {
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"},
                            {wrench::HTCondorComputeServiceProperty::SUPPORTS_GRID_UNIVERSE, "false"},
                    },
                    {},
                    batch_service)));



    std::dynamic_pointer_cast<wrench::HTCondorComputeService>(compute_service)->setLocalStorageService(storage_service);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new HTCondorNoPilotJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(grid_workflow));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file2, storage_service));
    ASSERT_NO_THROW(simulation->stageFile(input_file3, storage_service));

    // Fails because it will try to run a grid job on a system that does not support standard jobs, WMS is set up to
    //return 0 in this case and 1 if it successfully completes.
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}



