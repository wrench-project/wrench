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

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

#define EPSILON 0.05

WRENCH_LOG_CATEGORY(bare_metal_compute_service_test_standard_jobs, "Log category for BareMetalComputeServiceTestStandardJobs");


class BareMetalComputeServiceTestStandardJobs : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
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
    wrench::WorkflowTask *task7;
    wrench::WorkflowTask *task8;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_UnsupportedStandardJobs_test();

    void do_BogusNumCores_test();

    void do_TwoSingleCoreTasks_test();

    void do_TwoDualCoreTasksCase1_test();

    void do_TwoDualCoreTasksCase2_test();

    void do_TwoDualCoreTasksCase3_test();

    void do_JobTermination_test();

    void do_NonSubmittedJobTermination_test();

    void do_CompletedJobTermination_test();

    void do_ShutdownComputeServiceWhileJobIsRunning_test();

    void do_ShutdownStorageServiceBeforeJobIsSubmitted_test();

protected:
    BareMetalComputeServiceTestStandardJobs() {

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
        task7 = workflow->addTask("task_7_8s_1_to_4_cores", 8.0, 1, 4, 0);
        task8 = workflow->addTask("task_8_8s_2_to_4_cores", 8.0, 2, 4, 0);

        // Add file-task dependencies
        task1->addInputFile(input_file);
        task2->addInputFile(input_file);
        task3->addInputFile(input_file);
        task4->addInputFile(input_file);
        task5->addInputFile(input_file);
        task6->addInputFile(input_file);
        task7->addInputFile(input_file);
        task8->addInputFile(input_file);


        task1->addOutputFile(output_file1);
        task2->addOutputFile(output_file2);
        task3->addOutputFile(output_file3);
        task4->addOutputFile(output_file4);
        task5->addOutputFile(output_file5);
        task6->addOutputFile(output_file6);


        // Create a one-host dual-core platform file
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
                          "       </host>"
                          "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
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

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  UNSUPPORTED JOB TYPE TEST                                       **/
/**********************************************************************/

class MulticoreComputeServiceUnsupportedJobTypeTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceUnsupportedJobTypeTestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                     const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                                     const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>> file_locations;
        file_locations[test->input_file] = wrench::FileLocation::LOCATION(this->test->storage_service);
        file_locations[test->output_file1] = wrench::FileLocation::LOCATION(this->test->storage_service);
        file_locations[test->output_file2] = wrench::FileLocation::LOCATION(this->test->storage_service);
        auto two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, file_locations);

        // Submit the 2-task job for execution
        try {
            job_manager->submitJob(two_task_job, this->test->compute_service);
            throw std::runtime_error(
                    "Should not be able to submit a standard job to a compute service that does not support them");
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::JobTypeNotSupported>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got expected exception, but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: 'JobTypeNotSupported");
            }
            if (cause->getJob() != two_task_job) {
                throw std::runtime_error(
                        "Got the expected exception and failure cause, but the failure cause does not point to the right job");
            }
            if (cause->getComputeService() != this->test->compute_service) {
                throw std::runtime_error(
                        "Got the expected exception and failure cause, but the failure cause does not point to the right compute service");
            }
            std::string error_msg = cause->toString();
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, UnsupportedStandardJobs) {
    DO_TEST_WITH_FORK(do_UnsupportedStandardJobs_test);
}

void BareMetalComputeServiceTestStandardJobs::do_UnsupportedStandardJobs_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))}, "",
                                                {{{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "false"}}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new MulticoreComputeServiceUnsupportedJobTypeTestWMS(
                    this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  TWO BOGUS NUM CORES TEST                                        **/
/**********************************************************************/

class MulticoreComputeServiceBogusNumCoresTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceBogusNumCoresTestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        // Create a 1-task job
        std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>> file_locations;
        file_locations[this->test->input_file] = wrench::FileLocation::SCRATCH;
        file_locations[this->test->output_file3] = wrench::FileLocation::SCRATCH;
        auto two_core_task_job = job_manager->createStandardJob({this->test->task3},
                                                                file_locations,
                                                                {std::make_tuple(this->test->input_file,
                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                                 wrench::FileLocation::SCRATCH)},
                                                                {}, {});

        // Submit the 1-task job for execution with too few cores
        try {
            job_manager->submitJob(two_core_task_job, this->test->compute_service,
                                   {{"task_3_10s_2cores", "1"}});
            throw std::runtime_error("Should not be able to submit a job asking for 1 core for a 2-core tasks");
        } catch (std::invalid_argument &e) {
        }

        // Submit the 1-task job for execution with too many cores
        try {
            job_manager->submitJob(two_core_task_job, this->test->compute_service,
                                   {{"task_3_10s_2cores", "3"}});
            throw std::runtime_error("Should not be able to submit a job asking for 3 core for a 2-core tasks");
        } catch (std::invalid_argument &e) {
        }


        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, BogusNumCores) {
    DO_TEST_WITH_FORK(do_BogusNumCores_test);
}

void BareMetalComputeServiceTestStandardJobs::do_BogusNumCores_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "DualCoreHost";

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"}, {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}}))); //scratch space of size 101

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new MulticoreComputeServiceBogusNumCoresTestWMS(
                    this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  TWO SINGLE CORE TASKS TEST                                      **/
/**********************************************************************/

class MulticoreComputeServiceTwoSingleCoreTasksTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceTwoSingleCoreTasksTestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                     const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                     const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>> file_locations;
        file_locations[this->test->input_file] = wrench::FileLocation::SCRATCH;
        file_locations[this->test->output_file1] = wrench::FileLocation::SCRATCH;
        file_locations[this->test->output_file2] = wrench::FileLocation::SCRATCH;


        auto two_task_job = job_manager->createStandardJob(
                {this->test->task1, this->test->task2},
                file_locations,
                {std::make_tuple(this->test->input_file,
                                 wrench::FileLocation::LOCATION(this->test->storage_service),
                                 wrench::FileLocation::SCRATCH)},
                {}, {});

        // Submit the 2-task job for execution
        job_manager->submitJob(two_task_job, this->test->compute_service);

        // Wait for a workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event))  {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check completion states and times
        if ((this->test->task1->getState() != wrench::WorkflowTask::COMPLETED) ||
            (this->test->task2->getState() != wrench::WorkflowTask::COMPLETED)) {
            throw std::runtime_error("Unexpected task states");
        }

        double task1_end_date = this->test->task1->getEndDate();
        double task2_end_date = this->test->task2->getEndDate();
        double delta = std::abs(task1_end_date - task2_end_date);
        if (delta > 0.1) {
            throw std::runtime_error("Task completion times should be about 0.0 seconds apart but they are " +
                                     std::to_string(delta) + " apart.");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, TwoSingleCoreTasks) {
    DO_TEST_WITH_FORK(do_TwoSingleCoreTasks_test);
}

void BareMetalComputeServiceTestStandardJobs::do_TwoSingleCoreTasks_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(
                    hostname,
                    {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                    {"/scratch"},
                    {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}}))); //scratch space of size 101

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new MulticoreComputeServiceTwoSingleCoreTasksTestWMS(
                    this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  TWO DUAL-CORE TASKS TEST #1                                     **/
/**********************************************************************/

class MulticoreComputeServiceTwoDualCoreTasksCase1TestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceTwoDualCoreTasksCase1TestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                        const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>> file_locations;
        file_locations[this->test->input_file] = wrench::FileLocation::LOCATION(this->test->storage_service);
        file_locations[this->test->output_file3] = wrench::FileLocation::SCRATCH;
        file_locations[this->test->output_file4] = wrench::FileLocation::SCRATCH;
        auto two_task_job = job_manager->createStandardJob({this->test->task3, this->test->task4},
                                                           file_locations,
                                                           {std::make_tuple(this->test->input_file,
                                                                            wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                            wrench::FileLocation::SCRATCH)},
                                                           {}, {});

        // Submit the 2-task job for execution
        job_manager->submitJob(two_task_job, this->test->compute_service);

        // Wait for the job completion
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check completion states and times
        if ((this->test->task3->getState() != wrench::WorkflowTask::COMPLETED) ||
            (this->test->task4->getState() != wrench::WorkflowTask::COMPLETED)) {
            throw std::runtime_error("Unexpected task states");
        }

        double task3_end_date = this->test->task3->getEndDate();
        double task4_end_date = this->test->task4->getEndDate();
        double delta_task3 = std::abs(task3_end_date - 10.0);
        double delta_task4 = std::abs(task4_end_date - 10.0);
        if ((delta_task3 > EPSILON) || (delta_task4 > EPSILON)) {
            throw std::runtime_error("Unexpected task completion times " + std::to_string(task3_end_date) + " and " +
                                     std::to_string(task4_end_date) + ".");
        }

        return 0;
    }
};


TEST_F(BareMetalComputeServiceTestStandardJobs, TwoDualCoreTasksCase1) {
    DO_TEST_WITH_FORK(do_TwoDualCoreTasksCase1_test);
}

void BareMetalComputeServiceTestStandardJobs::do_TwoDualCoreTasksCase1_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair("DualCoreHost", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"}, {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms;
    ASSERT_NO_THROW(wms = simulation->add(
            new MulticoreComputeServiceTwoDualCoreTasksCase1TestWMS(
                    this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));

    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  TWO DUAL-CORE TASKS TEST #2                                     **/
/**********************************************************************/

class MulticoreComputeServiceTwoDualCoreTasksCase2TestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceTwoDualCoreTasksCase2TestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                        const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>> file_locations;
        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob({this->test->task5, this->test->task6},
                                                           file_locations,
                                                           {std::make_tuple(this->test->input_file,
                                                                            wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                            wrench::FileLocation::SCRATCH)},
                                                           {}, {});

        // Submit the 2-task job for execution
        job_manager->submitJob(two_task_job, this->test->compute_service);

        // Wait for the job completion
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check completion states and times
        if ((this->test->task5->getState() != wrench::WorkflowTask::COMPLETED) ||
            (this->test->task6->getState() != wrench::WorkflowTask::COMPLETED)) {
            throw std::runtime_error("Unexpected task states");
        }

        double task5_end_date = this->test->task5->getEndDate();
        double task6_end_date = this->test->task6->getEndDate();

        /*
         * while task 5 and 6 are running, each thread gets 4/5 of a core.
         * So task6 will finish at time (12/2) * (5/4) = 7.50.
         * Started at time 7.50, task5 is alone and has 10-6=4 seconds of work
         * left with each thread getting a core, so it completes at time 11.50.
         */
        double delta_task5 = std::abs(task5_end_date - 11.50);
        double delta_task6 = std::abs(task6_end_date - 7.50);


//      task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 1.0, 0);
//      task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 1.0, 0);
//
        if (delta_task5 > EPSILON) {
            throw std::runtime_error("Unexpected task5 end date " + std::to_string(task5_end_date) + " (should be 10.0)");
        }

        if (delta_task6 > EPSILON) {
            throw std::runtime_error("Unexpected task6 end date " + std::to_string(task6_end_date) + " (should be 12.0)");
        }

        return 0;
    }
};


TEST_F(BareMetalComputeServiceTestStandardJobs, TwoDualCoreTasksCase2) {
    DO_TEST_WITH_FORK(do_TwoDualCoreTasksCase2_test);
}

void BareMetalComputeServiceTestStandardJobs::do_TwoDualCoreTasksCase2_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "QuadCoreHost";

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair("QuadCoreHost",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"}, {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new MulticoreComputeServiceTwoDualCoreTasksCase2TestWMS(
                    this,  {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}




/**********************************************************************/
/**  TWO DUAL-CORE TASKS TEST #3                                     **/
/**********************************************************************/

class BareMetalComputeServiceTwoDualCoreTasksCase3TestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceTwoDualCoreTasksCase3TestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                        const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        auto two_task_job_1 = job_manager->createStandardJob({this->test->task5, this->test->task6},
                                                             (std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>>){},
                                                             {std::make_tuple(
                                                                     this->test->input_file,
                                                                     wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                     wrench::FileLocation::SCRATCH)},
                                                             {}, {});

        // Submit the 2-task job for execution (WRONG CS-specific arguments)
        try {
            job_manager->submitJob(two_task_job_1, this->test->compute_service,
                                   {{"whatever", "QuadCoreHost:2"}});
            throw std::runtime_error("Should not be able to use wrongly formatted service-specific arguments");
        } catch (std::invalid_argument &e) {
        }

        // Submit the 2-task job for execution (WRONG CS-specific arguments)
        try {
            job_manager->submitJob(two_task_job_1, this->test->compute_service,
                                   {{"task_6_10s_1_to_2_cores", "QuadCoreHost:2:bogus"}});
            throw std::runtime_error("Should not be able to use wrongly formatted service-specific arguments");
        } catch (std::invalid_argument &e) {
        }

        // Submit the 2-task job for execution (WRONG CS-specific arguments)
        try {
            job_manager->submitJob(two_task_job_1, this->test->compute_service,
                                   {{"task_6_10s_1_to_2_cores", "QuadCoreHost:whatever"}});
            throw std::runtime_error("Should not be able to use wrongly formatted service-specific arguments");
        } catch (std::invalid_argument &e) {
        }

        // Submit the 2-task job for execution (WRONG CS-specific arguments)
        try {
            job_manager->submitJob(two_task_job_1, this->test->compute_service,
                                   {{"task_6_10s_1_to_2_cores", "whatever"}});
            throw std::runtime_error("Should not be able to use wrongly formatted service-specific arguments");
        } catch (std::invalid_argument &e) {
        }


        // Submit the 2-task job for execution
        // service-specific args format testing: "hostname:num_cores", and "num_cores"
        // both tasks should run in parallel, using 2 of the 4 cores each
        std::cerr << "SUBMITTING FOR REAL\n";
        job_manager->submitJob(two_task_job_1, this->test->compute_service,
                               {{"task_6_10s_1_to_2_cores","QuadCoreHost:2"},{"task_5_30s_1_to_3_cores","2"}});
        std::cerr << "SUBMITTED FOR REAL\n";

        // Wait for the job completion
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check completion states and times
        if ((this->test->task5->getState() != wrench::WorkflowTask::COMPLETED) ||
            (this->test->task6->getState() != wrench::WorkflowTask::COMPLETED)) {
            throw std::runtime_error("Unexpected task states");
        }

        double task5_end_date = this->test->task5->getEndDate();
        double task6_end_date = this->test->task6->getEndDate();

        // Check the each task ran using 2 cores
        if (this->test->task5->getNumCoresAllocated() != 2) {
            throw std::runtime_error("It looks like task5 didn't run with 2 cores according to in-task info");
        }
        if (this->test->task6->getNumCoresAllocated() != 2) {
            throw std::runtime_error("It looks like task6 didn't run with 2 cores according to in-task info");
        }

        /*
         * Check that each task was happy on 2 cores
         */
        double delta_task5 = std::abs(task5_end_date - 15.00);
        double delta_task6 = std::abs(task6_end_date - 6.00);

        if (delta_task5 > EPSILON) {
            throw std::runtime_error("Unexpected task5 end date " + std::to_string(task5_end_date) + " (should be 10.0)");
        }

        if (delta_task6 > EPSILON) {
            throw std::runtime_error("Unexpected task6 end date " + std::to_string(task6_end_date) + " (should be 12.0)");
        }


        // create and submit another 2-task job for execution
        // service-specific args format testing: "hostname", "" <- that's an empty string
        // both tasks should run in parallel, use 4 cores each, thus oversubscribing
        auto two_task_job_2 = job_manager->createStandardJob({this->test->task7, this->test->task8},
                                                             (std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>>){},
                                                             {std::make_tuple(this->test->input_file,
                                                                              wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                              wrench::FileLocation::SCRATCH)},
                                                             {}, {});

        job_manager->submitJob(two_task_job_2, this->test->compute_service,
                               {{"task_7_8s_1_to_4_cores","QuadCoreHost"},{"task_8_8s_2_to_4_cores",""}});

        // Wait for the job completion
        double two_task_job_2_completion_date = 0;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            two_task_job_2_completion_date = wrench::Simulation::getCurrentSimulatedDate();
            // success, do nothing for now
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check that each task ran using 4 cores
        if (this->test->task7->getNumCoresAllocated() != 4) {
            throw std::runtime_error("It looks like task7 didn't run with 4 cores according to in-task info");
        }
        if (this->test->task8->getNumCoresAllocated() != 4) {
            throw std::runtime_error("It looks like task8 didn't run with 4 cores according to in-task info");
        }

        // the standard job is expected take about 4 seconds, since each task would run for 2 seconds if the
        // compute host wasn't oversubscribed
        double two_task_job_2_duration = two_task_job_2_completion_date - two_task_job_2->getSubmitDate();
        if (two_task_job_2_duration < (4.0 - EPSILON) || two_task_job_2_duration > (4.0 + EPSILON)) {
            throw std::runtime_error("two_task_job_2 should have taken about 4 seconds, but did not");
        }


        return 0;
    }
};


TEST_F(BareMetalComputeServiceTestStandardJobs, TwoDualCoreTasksCase3) {
    DO_TEST_WITH_FORK(do_TwoDualCoreTasksCase3_test);
}

void BareMetalComputeServiceTestStandardJobs::do_TwoDualCoreTasksCase3_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "QuadCoreHost";

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair("QuadCoreHost", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceTwoDualCoreTasksCase3TestWMS(
                    this,  {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  JOB TERMINATION TEST                                            **/
/**********************************************************************/

class BareMetalComputeServiceJobTerminationTestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceJobTerminationTestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                 const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2},
                                                           (std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>>){},
                                                           {std::make_tuple(this->test->input_file,
                                                                            wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                            wrench::FileLocation::SCRATCH)},
                                                           {}, {});

        // Submit the 2-task job for execution
        job_manager->submitJob(two_task_job, this->test->compute_service);

        // Immediately terminate it
        try {
            job_manager->terminateJob(two_task_job);
        } catch (std::exception &e) {
            throw std::runtime_error("Unexpected exception while terminating job: " + std::string(e.what()));
        }

        // Check that the job's state has been updated
        if (two_task_job->getState() != wrench::StandardJob::TERMINATED) {
            throw std::runtime_error("Terminated Standard Job is not in TERMINATED state");
        }

        // Check that task states make sense
        if ((this->test->task1->getState() != wrench::WorkflowTask::READY) ||
            (this->test->task2->getState() != wrench::WorkflowTask::READY)) {
            throw std::runtime_error("Tasks in a TERMINATED job should be in the READY state but instead (" +
                                     wrench::WorkflowTask::stateToString(this->test->task1->getState()) + ", " +
                                     wrench::WorkflowTask::stateToString(this->test->task2->getState()) + ")");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, JobTermination) {
    DO_TEST_WITH_FORK(do_JobTermination_test);
}

void BareMetalComputeServiceTestStandardJobs::do_JobTermination_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceJobTerminationTestWMS(
                    this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    ASSERT_EQ(this->task1->getState(), wrench::WorkflowTask::READY);
    ASSERT_EQ(this->task2->getState(), wrench::WorkflowTask::READY);

    ASSERT_EQ(this->task1->getFailureCount(), 0);
    ASSERT_EQ(this->task2->getFailureCount(), 0);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  NOT SUBMITTED JOB TERMINATION TEST                              **/
/**********************************************************************/

class BareMetalComputeServiceNonSubmittedJobTerminationTestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceNonSubmittedJobTerminationTestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                             const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                                             const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2});

        // Try to terminate it now (which is stupid)
        try {
            job_manager->terminateJob(two_task_job);
            throw std::runtime_error("Trying to terminate a non-submitted job should have raised an exception!");
        } catch (wrench::ExecutionException &e) {

            auto cause = std::dynamic_pointer_cast<wrench::NotAllowed>(e.getCause());
            if (not cause) {
                throw std::runtime_error(
                        "Got an expected exception but an unexpected failure cause: " +
                        e.getCause()->toString() + " (expected: JobCannotBeTerminated)");
            }
            if (cause->getService() != nullptr) {
                throw std::runtime_error(
                        "Got the expected exception and failure cause, but the failure cause does not point to the right service (should be nullptr)");
            }
            cause->toString();
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, NonSubmittedJobTermination) {
    DO_TEST_WITH_FORK(do_NonSubmittedJobTermination_test);
}

void BareMetalComputeServiceTestStandardJobs::do_NonSubmittedJobTermination_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "", {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceNonSubmittedJobTerminationTestWMS(
                    this,  {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Check completion states and times
    ASSERT_EQ(this->task1->getState(), wrench::WorkflowTask::READY);
    ASSERT_EQ(this->task2->getState(), wrench::WorkflowTask::READY);

    ASSERT_EQ(this->task1->getFailureCount(), 0);
    ASSERT_EQ(this->task2->getFailureCount(), 0);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  COMPLETED JOB TERMINATION TEST                                  **/
/**********************************************************************/

class BareMetalComputeServiceCompletedJobTerminationTestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceCompletedJobTerminationTestWMS(BareMetalComputeServiceTestStandardJobs *test,
                                                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2},
                                                           (std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>>){},
                                                           {std::make_tuple(this->test->input_file,
                                                                            wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                            wrench::FileLocation::SCRATCH)},
                                                           {}, {});

        // Submit the 2-task job for execution
        job_manager->submitJob(two_task_job, this->test->compute_service);

        // Wait for the job completion
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Try to terminate it now (which is stupid)
        try {
            job_manager->terminateJob(two_task_job);
            throw std::runtime_error("Trying to terminate a non-submitted job should have raised an exception!");
        } catch (std::exception &e) {
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, CompletedJobTermination) {
    DO_TEST_WITH_FORK(do_CompletedJobTermination_test);
}

void BareMetalComputeServiceTestStandardJobs::do_CompletedJobTermination_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceCompletedJobTerminationTestWMS(
                    this,  {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Check completion states and times
    ASSERT_EQ(this->task1->getState(), wrench::WorkflowTask::COMPLETED);
    ASSERT_EQ(this->task2->getState(), wrench::WorkflowTask::COMPLETED);

    ASSERT_EQ(this->task1->getFailureCount(), 0);
    ASSERT_EQ(this->task2->getFailureCount(), 0);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  COMPUTE SERVICE SHUTDOWN WHILE JOB IS RUNNING TEST              **/
/**********************************************************************/

class BareMetalComputeServiceShutdownComputeServiceWhileJobIsRunningTestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceShutdownComputeServiceWhileJobIsRunningTestWMS(
            BareMetalComputeServiceTestStandardJobs *test,
            const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
            std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2},
                                                           (std::map<wrench::WorkflowFile*, std::shared_ptr<wrench::FileLocation>>){},
                                                           {std::make_tuple(this->test->input_file,
                                                                            wrench::FileLocation::LOCATION(this->test->storage_service),
                                                                            wrench::FileLocation::SCRATCH)},
                                                           {}, {});

        // Submit the 2-task job for execution
        job_manager->submitJob(two_task_job, this->test->compute_service);

        // Shutdown the compute service
        this->test->compute_service->stop();

        // Wait for the job failure notification
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        if (real_event) {
            auto cause = std::dynamic_pointer_cast<wrench::JobKilled>(real_event->failure_cause);
            if (not cause) {
                throw std::runtime_error("Got a job failure event, but an unexpected failure cause: " +
                                         real_event->failure_cause->toString() + " (expected: JobKilled)");
            }
            if (cause->getService() != this->test->compute_service) {
                std::runtime_error(
                        "Got the correct failure even, a correct cause type, but the cause points to the wrong service");
            }
            if (cause->getJob() != two_task_job) {
                std::runtime_error(
                        "Got the correct failure even, a correct cause type, but the cause points to the wrong job");
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Terminate
        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, ShutdownComputeServiceWhileJobIsRunning) {
    DO_TEST_WITH_FORK(do_ShutdownComputeServiceWhileJobIsRunning_test);
}

void BareMetalComputeServiceTestStandardJobs::do_ShutdownComputeServiceWhileJobIsRunning_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch", {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceShutdownComputeServiceWhileJobIsRunningTestWMS(
                    this, {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    // Check completion states and times
    ASSERT_EQ(this->task1->getState(), wrench::WorkflowTask::READY);
    ASSERT_EQ(this->task2->getState(), wrench::WorkflowTask::READY);

    ASSERT_EQ(this->task1->getFailureCount(), 1);
    ASSERT_EQ(this->task2->getFailureCount(), 1);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  STORAGE SERVICE SHUTDOWN BEFORE JOB IS SUBMITTED TEST           **/
/**********************************************************************/

class BareMetalComputeServiceShutdownStorageServiceBeforeJobIsSubmittedTestWMS : public wrench::WMS {

public:
    BareMetalComputeServiceShutdownStorageServiceBeforeJobIsSubmittedTestWMS(
            BareMetalComputeServiceTestStandardJobs *test,
            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
            std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    BareMetalComputeServiceTestStandardJobs *test;

    int main() {

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job  manager
        auto job_manager = this->createJobManager();

        auto file_registry_service = this->getAvailableFileRegistryService();

        // Create a 2-task job
        auto two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2});

        // Shutdown the storage service
        this->test->storage_service->stop();

        // Submit the 2-task job for execution
        job_manager->submitJob(two_task_job, this->test->compute_service);

        // Wait for the job failure notification
        std::shared_ptr<wrench::ExecutionEvent> event;
        try {
            event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
        if (real_event) {
            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(real_event->failure_cause);
            if (not cause) {
                throw std::runtime_error("Got the expected job failure but unexpected failure cause: " +
                                         real_event->failure_cause->toString() + " (expected: FileNotFound)");
            }
            if (cause->getFile() != this->test->input_file) {
                throw std::runtime_error(
                        "Got the correct failure even, a correct cause type, but the cause points to the wrong file");
            }
        } else {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceTestStandardJobs, ShutdownStorageServiceBeforeJobIsSubmitted) {
    DO_TEST_WITH_FORK(do_ShutdownStorageServiceBeforeJobIsSubmitted_test);
}

void BareMetalComputeServiceTestStandardJobs::do_ShutdownStorageServiceBeforeJobIsSubmitted_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create A Storage Services
    ASSERT_NO_THROW(storage_service = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/"})));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "", {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BareMetalComputeServiceShutdownStorageServiceBeforeJobIsSubmittedTestWMS(
                    this,  {compute_service}, {storage_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry
    simulation->add(new wrench::FileRegistryService(hostname));


    // Staging the input file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    ASSERT_EQ(this->task1->getState(), wrench::WorkflowTask::READY);
    ASSERT_EQ(this->task2->getState(), wrench::WorkflowTask::READY);

    ASSERT_EQ(this->task1->getFailureCount(), 1);
    ASSERT_EQ(this->task2->getFailureCount(), 1);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
