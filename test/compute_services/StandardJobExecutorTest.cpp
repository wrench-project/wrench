/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <random>
#include <wrench-dev.h>

#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "../../src/wrench/helper_services/standard_job_executor/StandardJobExecutorMessage.h"


#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(standard_job_executor_test, "Log category for Simple StandardJobExecutorTest");

#define EPSILON 0.05

class StandardJobExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    wrench::Simulation *simulation;

    void do_StandardJobExecutorConstructorTest_test();

    void do_OneSingleCoreTaskTest_test();

    void do_OneSingleCoreTaskBogusPreFileCopyTest_test();

    void do_OneSingleCoreTaskMissingFileTest_test();

    void do_OneMultiCoreTaskTestCase1_test();

    void do_OneMultiCoreTaskTestCase2_test();

    void do_OneMultiCoreTaskTestCase3_test();

    void do_DependentTasksTest_test();

    void do_TwoMultiCoreTasksTest_test();

    void do_NoTaskTest_test();

    void do_MultiHostTest_test();

    void do_JobTerminationTestDuringAComputation_test();

    void do_JobTerminationTestDuringATransfer_test();

    void do_JobTerminationTestAtRandomTimes_test();

    void do_WorkUnit_test();

    static bool isJustABitGreater(double base, double variable, double epsilon) {
        return ((variable > base) && (variable < base + epsilon));
    }

    static bool isJustABitGreaterThanOrEqual(double base, double variable, double epsilon) {
        return ((variable >= base) && (variable < base + epsilon));
    }

protected:
    StandardJobExecutorTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101b\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1024B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"1us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;
};

/**********************************************************************/
/**  DO CONSTRUCTOR TEST                                             **/
/**********************************************************************/

class StandardJobExecutorConstructorTestWMS : public wrench::WMS {

public:
    StandardJobExecutorConstructorTestWMS(StandardJobExecutorTest *test,
                                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                          std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::shared_ptr<wrench::StandardJobExecutor> executor;

        // Create a sequential task that lasts one hour
        wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 3600, 1, 1, 0);
        task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
        task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

        // Create a StandardJob with some pre-copies, post-copies and post-deletions (not useful, but this is testing after all)
        auto job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                this->test->storage_service1)},
                        {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                this->test->storage_service2)}
                },
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2),
                        wrench::FileLocation::LOCATION(this->test->storage_service1))},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2))});

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();
        double thread_startup_overhead = 10.0;

        // Create a bogus StandardJobExecutor (invalid host)
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair("bogus", std::make_tuple(2, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error("Should not be able to create a standard job executor with a bogus host");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (nullptr job)
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            nullptr,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(2, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error("Should not be able to create a standard job executor with a nullptr job");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (no compute resources specified)
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error("Should not be able to create a standard job executor with no compute resources");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (no cores resources specified)
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(0, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error(
                    "Should not be able to create a standard job executor with zero cores on a resource");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (too many cores specified)
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(100, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error(
                    "Should not be able to create a standard job executor with more cores than available on a resource");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (negative RAM specified)
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(wrench::ComputeService::ALL_CORES, -1))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error(
                    "Should not be able to create a standard job executor with negative RAM on a resource");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (too much RAM specified)
        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair("Host4", std::make_tuple(wrench::ComputeService::ALL_CORES, 2048))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error(
                    "Should not be able to create a standard job executor with more RAM than available on a resource");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (not enough Cores specified)
        this->getWorkflow()->removeTask(task);
        wrench::WorkflowTask *task_too_many_cores = this->getWorkflow()->addTask("task_too_many_cores", 3600, 20, 20, 0);
        task_too_many_cores->addInputFile(this->getWorkflow()->getFileByID("input_file"));
        task_too_many_cores->addOutputFile(this->getWorkflow()->getFileByID("output_file"));
//        // Forget the previous job!
        job_manager->forgetJob(job);

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
        job = job_manager->createStandardJob(
                {task_too_many_cores},
                {
                        {*(task_too_many_cores->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                this->test->storage_service1)},
                        {*(task_too_many_cores->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                this->test->storage_service2)}
                },
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2),
                        wrench::FileLocation::LOCATION(this->test->storage_service1))},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2))});

        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair("Host4", std::make_tuple(wrench::ComputeService::ALL_CORES, 100.00))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error(
                    "Should not be able to create a standard job executor with insufficient RAM to run the job");
        } catch (std::invalid_argument &e) {
        }

        // Create a bogus StandardJobExecutor (not enough RAM specified)
        this->getWorkflow()->removeTask(task_too_many_cores);
        wrench::WorkflowTask *task_too_much_ram = this->getWorkflow()->addTask("task_too_much_ram", 3600, 1, 1, 500.00);
        task_too_much_ram->addInputFile(this->getWorkflow()->getFileByID("input_file"));
        task_too_much_ram->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

        // Forget the previous job!
        job_manager->forgetJob(job);

        // Create a StandardJob
        job = job_manager->createStandardJob(
                {task_too_much_ram},
                {
                        {*(task_too_much_ram->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                this->test->storage_service1)},
                        {*(task_too_much_ram->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                this->test->storage_service2)}
                },
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2),
                        wrench::FileLocation::LOCATION(this->test->storage_service1))},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2))});

        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair("Host4", std::make_tuple(wrench::ComputeService::ALL_CORES, 100.00))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            throw std::runtime_error(
                    "Should not be able to create a standard job executor with insufficient RAM to run the job");
        } catch (std::invalid_argument &e) {
        }

        // Finally do one that works

        // Forget the previous job!
        job_manager->forgetJob(job);

        this->getWorkflow()->removeTask(task_too_much_ram);
        task = this->getWorkflow()->addTask("task", 3600, 1, 1, 0);
        task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
        task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
        job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                this->test->storage_service1)},
                        {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                this->test->storage_service2)}
                },
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2),
                        wrench::FileLocation::LOCATION(this->test->storage_service1))},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("output_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2))});

        try {
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair("Host1", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                     wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Should be able to create a valid standard job executor!");
        }

        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        job_manager->forgetJob(job);

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, ConstructorTest) {
    DO_TEST_WITH_FORK(do_StandardJobExecutorConstructorTest_test);
}

void StandardJobExecutorTest::do_StandardJobExecutorConstructorTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname1 = "Host1";
    std::string hostname2 = "Host2";

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname2, {"/"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new StandardJobExecutorConstructorTestWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, hostname1)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  ONE SINGLE-CORE TASK SIMULATION TEST ON ONE HOST                **/
/**********************************************************************/

class OneSingleCoreTaskTestWMS : public wrench::WMS {

public:
    OneSingleCoreTaskTestWMS(StandardJobExecutorTest *test,
                             const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                             const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                             std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task that lasts one hour
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 3600, 1, 1, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                    this->test->storage_service2)}
                    },
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("output_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2),
                            wrench::FileLocation::LOCATION(this->test->storage_service1))},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("output_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))});

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();
            double thread_startup_overhead = 10.0;

            // Create a StandardJobExecutor that will run stuff on one host and one core
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            "Host1",
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(2, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after = wrench::S4U_Simulation::getClock();

            double observed_duration = after - before;

            double expected_duration = task->getFlops() + 1 * thread_startup_overhead;

            // Does the job completion time make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(before + expected_duration, after, 0.2)) {
                throw std::runtime_error("Unexpected job completion time (should be around " +
                                         std::to_string(before + expected_duration) + " but is " +
                                         std::to_string(after) + ")");
            }

            // Doe the task-stored time information look good
            if (!StandardJobExecutorTest::isJustABitGreaterThanOrEqual(before, task->getStartDate(), EPSILON)) {
//          std::cerr << "START: " << task->getStartDate() << std::endl;
                throw std::runtime_error(
                        "Case 1: Unexpected task start date: " + std::to_string(task->getStartDate()) + "| " +
                        "before: " + std::to_string(before));
            }

            // Note that we have to subtract the last thread startup overhead (for file deletions)
            if (!StandardJobExecutorTest::isJustABitGreater(task->getEndDate(), after, EPSILON)) {
                throw std::runtime_error(
                        "Case 1: Unexpected task end date: " + std::to_string(task->getEndDate()) +
                        " (expected: " + std::to_string(after) + ")");
            }

            // Has the output file been copied back to storage_service1?
            if (not wrench::StorageService::lookupFile(this->getWorkflow()->getFileByID("output_file"),
                                                       wrench::FileLocation::LOCATION(this->test->storage_service1))) {
                throw std::runtime_error("The output file has not been copied back to the specified storage service");
            }

            // Has the output file been erased from storage_service2?
            if (wrench::StorageService::lookupFile(this->getWorkflow()->getFileByID("output_file"),
                                                   wrench::FileLocation::LOCATION(this->test->storage_service2))) {
                throw std::runtime_error("The output file has not been erased from the specified storage service");
            }

//        this->getWorkflow()->removeTask(task);
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneSingleCoreTaskTest) {
    DO_TEST_WITH_FORK(do_OneSingleCoreTaskTest_test);
}

void StandardJobExecutorTest::do_OneSingleCoreTaskTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname1 = "Host1";
//    std::string hostname2 = "Host2";

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk2"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new OneSingleCoreTaskTestWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, hostname1)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/****************************************************************************/
/**  ONE SINGLE-CORE TASK SIMULATION TEST ON ONE HOST: BOGUS PRE FILE COPY **/
/****************************************************************************/

class OneSingleCoreTaskBogusPreFileCopyTestWMS : public wrench::WMS {

public:
    OneSingleCoreTaskBogusPreFileCopyTestWMS(StandardJobExecutorTest *test,
                                             const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                             const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                             std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // WARNING: The StandardJobExecutor unique_ptr is declared here, so that
        // it's not automatically freed after the next basic block is over. In the internals
        // of WRENCH, this is typically take care in various ways (e.g., keep a list of "finished" executors)
        std::shared_ptr<wrench::StandardJobExecutor> executor;

        {
            // Create a sequential task that lasts one hour and requires 1 cores
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 3600, 1, 1, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                    this->test->storage_service2)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2),
                            wrench::FileLocation::LOCATION(this->test->storage_service1))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("output_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))});


            std::string my_mailbox = "test_callback_mailbox";

            // Create a StandardJobExecutor that will run stuff on one host and two core
            double thread_startup_overhead = 10.0;
            try {
                executor = std::shared_ptr<wrench::StandardJobExecutor>(new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        wrench::Simulation::getHostnameList()[0],
                        job,
                        {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                        std::make_tuple(2, wrench::ComputeService::ALL_RAM))},
                        nullptr,
                        false,
                        nullptr,
                        {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
            } catch (std::runtime_error &e) {
                throw std::runtime_error("Should have been able to create standard job executor!");
            }
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorFailedMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(msg->cause);
            if (not cause) {
                throw std::runtime_error("Unexpected failure cause:  " +
                                         msg->cause->toString() + " (expected: FileNotFound)");
            }

            if (cause->getFile() != this->getWorkflow()->getFileByID("input_file")) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
            }
            if (cause->getLocation()->getStorageService() != this->test->storage_service2) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
            }

//        this->test->storage_service2->deleteFile(workflow->getFileByID("input_file"));
//        this->getWorkflow()->removeTask(task);

        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneSingleCoreTaskBogusPreFileCopyTest) {
    DO_TEST_WITH_FORK(do_OneSingleCoreTaskBogusPreFileCopyTest_test);
}

void StandardJobExecutorTest::do_OneSingleCoreTaskBogusPreFileCopyTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_tess");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname1 = "Host1";
    std::string hostname2 = "Host2";

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname2, {"/"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new OneSingleCoreTaskBogusPreFileCopyTestWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, hostname1)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/*************************************************************************/
/**  ONE SINGLE-CORE TASK SIMULATION TEST ON ONE HOST: MISSING FILE **/
/*************************************************************************/

class OneSingleCoreTaskMissingFileTestWMS : public wrench::WMS {

public:
    OneSingleCoreTaskMissingFileTestWMS(StandardJobExecutorTest *test,
                                        const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // WARNING: The StandardJobExecutor unique_ptr is declared here, so that
        // it's not automatically freed after the next basic block is over. In the internals
        // of WRENCH, this is typically take care in various ways (e.g., keep a list of "finished" executors)
        std::shared_ptr<wrench::StandardJobExecutor> executor;

        {
            // Create a sequential task that lasts one hour and requires 1 cores
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 3600, 1, 1, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service2)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                    this->test->storage_service2)}
                    },
                    {},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("output_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))});

            job->getFileLocations(); // coverage
            job->getPriority(); // coverage
            job->getMinimumRequiredNumCores(); // coverage

            std::string my_mailbox = "test_callback_mailbox";

            // Create a StandardJobExecutor that will run stuff on one host and two core
            double thread_startup_overhead = 10.0;

            try {
                executor = std::shared_ptr<wrench::StandardJobExecutor>(new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        wrench::Simulation::getHostnameList()[1],
                        job,
                        {std::make_pair(wrench::Simulation::getHostnameList()[1],
                                        std::make_tuple(2, wrench::ComputeService::ALL_RAM))},
                        nullptr,
                        false,
                        nullptr,
                        {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
            } catch (std::runtime_error &e) {
                throw std::runtime_error("Should have been able to create standard job executor!");
            }
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorFailedMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            auto cause = std::dynamic_pointer_cast<wrench::FileNotFound>(msg->cause);
            if (not cause) {
                throw std::runtime_error("Unexpected failure cause type: " +
                                         msg->cause->toString() + " (expected: FileNotFound)");
            }

            std::string error_msg = cause->toString();
            if (cause->getFile() != this->getWorkflow()->getFileByID("input_file")) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
            }
            if (cause->getLocation()->getStorageService() != this->test->storage_service2) {
                throw std::runtime_error(
                        "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
            }

//        this->getWorkflow()->removeTask(task);

        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneSingleCoreTaskMissingFileTest) {
    DO_TEST_WITH_FORK(do_OneSingleCoreTaskMissingFileTest_test);
}

void StandardJobExecutorTest::do_OneSingleCoreTaskMissingFileTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname1 = "Host1";
    std::string hostname2 = "Host2";

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"})));

    // Create another Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname2, {"/"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new OneSingleCoreTaskMissingFileTestWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, hostname1)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  STANDARD JOB WITH DEPENDENT TASKS                               **/
/**********************************************************************/

class DependentTasksTestWMS : public wrench::WMS {

public:
    DependentTasksTestWMS(StandardJobExecutorTest *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {

            //   t1 -> f1 -> t2 -> f2 -> t4
            //         f1 -> t3 -> f3 -> t4

            // Create two workflow files
            wrench::WorkflowFile *f1 = this->getWorkflow()->addFile("f1", 1.0);
            wrench::WorkflowFile *f2 = this->getWorkflow()->addFile("f2", 1.0);
            wrench::WorkflowFile *f3 = this->getWorkflow()->addFile("f3", 1.0);

            // Create sequential tasks
            wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("t1", 100, 1, 1, 0);
            wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("t2", 100, 1, 1, 0);
            wrench::WorkflowTask *t3 = this->getWorkflow()->addTask("t3", 150, 1, 1, 0);
            wrench::WorkflowTask *t4 = this->getWorkflow()->addTask("t4", 100, 1, 1, 0);

            t1->addOutputFile(f1);
            t2->addInputFile(f1);
            t3->addInputFile(f1);
            t2->addOutputFile(f2);
            t3->addOutputFile(f3);
            t4->addInputFile(f2);
            t4->addInputFile(f3);

            // Create a BOGUS StandardJob (just for testing)
            try {
                wrench::StandardJob *job = job_manager->createStandardJob(
                        {t1, t2, t4},
                        {},
                        {},
                        {},
                        {});
                throw std::runtime_error("Should not be able to create a standard job with t1, t2, t3 only");
            } catch (std::invalid_argument &e) {
            }


            // Create a StandardJob
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {t1, t2, t3, t4},
                    {},
                    {},
                    {},
                    {});

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();
            double thread_startup_overhead = 0.0;

            // Create a StandardJobExecutor that will run stuff on one host and 2 cores
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(2, wrench::ComputeService::ALL_RAM))},
                            this->test->storage_service1, // This should be a scratch space of a compute service, but since this
                            //standard job executor is being created direclty (not by any Compute Service), we pass a dummy storage as a scratch space
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                    thread_startup_overhead)}}, {}

                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            if (!StandardJobExecutorTest::isJustABitGreater(100, t1->getEndDate(), EPSILON)) {
                throw std::runtime_error("Unexpected completion time for t1: " +
                                         std::to_string(t1->getEndDate()) + "(should be 100)");
            }
            if (!StandardJobExecutorTest::isJustABitGreater(200, t2->getEndDate(), EPSILON)) {
                throw std::runtime_error("Unexpected completion time for t2: " +
                                         std::to_string(t2->getEndDate()) + "(should be 200)");
            }
            if (!StandardJobExecutorTest::isJustABitGreater(250, t3->getEndDate(), EPSILON)) {
                throw std::runtime_error("Unexpected completion time for t3: " +
                                         std::to_string(t3->getEndDate()) + "(should be 250)");
            }
            if (!StandardJobExecutorTest::isJustABitGreater(350, t4->getEndDate(), EPSILON)) {
                throw std::runtime_error("Unexpected completion time for t4: " +
                                         std::to_string(t4->getEndDate()) + "(should be 350)");
            }

            if ((t1->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED) ||
                (t2->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED) ||
                (t3->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED) ||
                (t4->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED)) {
                throw std::runtime_error("Unexpected task states!");
            }

//        this->getWorkflow()->removeTask(t1);
//        this->getWorkflow()->removeTask(t2);
//        this->getWorkflow()->removeTask(t3);
//        this->getWorkflow()->removeTask(t4);
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, DependentTasksTest) {
    DO_TEST_WITH_FORK(do_DependentTasksTest_test);
}

void StandardJobExecutorTest::do_DependentTasksTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a Compute Service
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new DependentTasksTestWMS(
                    this, {compute_service}, {storage_service1}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/*******************************************************************************************/
/**  ONE MULTI-CORE TASK SIMULATION TEST : CASE 1                                         **/
/**  Case 1: Create a multicore task with perfect parallel efficiency that lasts one hour **/
/*******************************************************************************************/

class OneMultiCoreTaskTestWMSCase1 : public wrench::WMS {

public:
    OneMultiCoreTaskTestWMSCase1(StandardJobExecutorTest *test,
                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                 const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        wrench::WorkflowTask *task = this->getWorkflow()->addTask("task1", 3600, 1, 10, 0);
        task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
        task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                this->test->storage_service1)},
                        {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                this->test->storage_service1)}
                });

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        wrench::Simulation::getHostnameList()[1],
                        job,
                        {std::make_pair(wrench::Simulation::getHostnameList()[1],
                                        std::make_tuple(6, wrench::ComputeService::ALL_RAM))},
                        nullptr,
                        false,
                        nullptr,
                        {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = task->getFlops() / 6;
        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
            throw std::runtime_error(
                    "Case 1: Unexpected task duration (should be around " + std::to_string(expected_duration) +
                    " but is " +
                    std::to_string(observed_duration) + ")");
        }

        wrench::StorageService::deleteFile(this->getWorkflow()->getFileByID("output_file"),
                                           wrench::FileLocation::LOCATION(this->test->storage_service1));

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneMultiCoreTaskTestCase1) {
    DO_TEST_WITH_FORK(do_OneMultiCoreTaskTestCase1_test);
}

void StandardJobExecutorTest::do_OneMultiCoreTaskTestCase1_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_tes");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new OneMultiCoreTaskTestWMSCase1(
                    this, {compute_service}, {storage_service1}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/*******************************************************************************************/
/**  ONE MULTI-CORE TASK SIMULATION TEST : CASE 2                                         **/
/**  Case 2: Create a multicore task with 50% parallel efficiency that lasts one hour     **/
/*******************************************************************************************/

class OneMultiCoreTaskTestWMSCase2 : public wrench::WMS {

public:
    OneMultiCoreTaskTestWMSCase2(StandardJobExecutorTest *test,
                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                 const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        auto task = this->getWorkflow()->addTask("task2", 3600, 1, 10, 0);
        double parallel_efficiency = 0.5;
        task->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(0.5));
        // coverage
        if (std::dynamic_pointer_cast<wrench::ConstantEfficiencyParallelModel>(task->getParallelModel())->getEfficiency() != 0.5) {
            throw std::runtime_error("Couldn't get back the parallel efficiency for the task model");
        };
        task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
        task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

        // Create a StandardJob
        auto job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                this->test->storage_service1)},
                        {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                this->test->storage_service1)}
                });

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        wrench::Simulation::getHostnameList()[1],
                        job,
                        {std::make_pair(wrench::Simulation::getHostnameList()[1],
                                        std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                        nullptr,
                        false,
                        nullptr,
                        {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
        if (!msg) {
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorFailedMessage>(message);
            std::string error_msg = msg->cause->toString();
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = task->getFlops() / (10 * parallel_efficiency);

        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
            throw std::runtime_error(
                    "Case 2: Unexpected task duration (should be around " + std::to_string(expected_duration) +
                    " but is " +
                    std::to_string(observed_duration) + ")");
        }


        wrench::StorageService::deleteFile(this->getWorkflow()->getFileByID("output_file"),
                                           wrench::FileLocation::LOCATION(this->test->storage_service1));

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneMultiCoreTaskTestCase2) {
    DO_TEST_WITH_FORK(do_OneMultiCoreTaskTestCase2_test);
}

void StandardJobExecutorTest::do_OneMultiCoreTaskTestCase2_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new OneMultiCoreTaskTestWMSCase2(
                    this, {compute_service}, {storage_service1}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/*******************************************************************************************************/
/**  ONE MULTI-CORE TASK SIMULATION TEST : CASE 3                                                     **/
/**  Case 3: Create a multicore task with 50% parallel efficiency and include thread startup overhead **/
/*******************************************************************************************************/

class OneMultiCoreTaskTestWMSCase3 : public wrench::WMS {

public:
    OneMultiCoreTaskTestWMSCase3(StandardJobExecutorTest *test,
                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                 const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        auto task = this->getWorkflow()->addTask("task3", 3600, 1, 10, 0);
        double  parallel_efficiency = 0.5;
        task->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(parallel_efficiency));
        task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
        task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(
                                this->test->storage_service1)},
                        {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(
                                this->test->storage_service1)}
                });

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        double thread_startup_overhead = 14;
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        wrench::Simulation::getHostnameList()[1],
                        job,
                        {std::make_pair(wrench::Simulation::getHostnameList()[1],
                                        std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                        nullptr,
                        false,
                        nullptr,
                        {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration =
                10 * thread_startup_overhead + task->getFlops() / (10 * parallel_efficiency);

        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
            throw std::runtime_error(
                    "Case 3: Unexpected job duration (should be around " + std::to_string(expected_duration) +
                    " but is " +
                    std::to_string(observed_duration) + ")");
        }

        wrench::StorageService::deleteFile(
                this->getWorkflow()->getFileByID("output_file"),
                wrench::FileLocation::LOCATION(this->test->storage_service1));

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneMultiCoreTaskTestCase3) {
    DO_TEST_WITH_FORK(do_OneMultiCoreTaskTestCase3_test);
}

void StandardJobExecutorTest::do_OneMultiCoreTaskTestCase3_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new OneMultiCoreTaskTestWMSCase3(
                    this, {compute_service}, {storage_service1}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  TWO MULTI-CORE TASKS SIMULATION TEST ON ONE HOST               **/
/**********************************************************************/

class TwoMultiCoreTasksTestWMS : public wrench::WMS {

public:
    TwoMultiCoreTasksTestWMS(StandardJobExecutorTest *test,
                             const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                             const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                             std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        /** Case 1: Create two tasks that will run in sequence with the default scheduling options **/
        {
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task1.1", 3600, 2, 6, 0);
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task1.2", 300, 6, 6, 0);
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file"));
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));

            // Create a StandardJob with both tasks
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task1, task2},
                    {
                            {this->getWorkflow()->getFileByID("input_file"),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))}
            );

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();

            // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after = wrench::S4U_Simulation::getClock();

            double observed_duration = after - before;

            double expected_duration = task1->getFlops() / 6 + task2->getFlops() / 6;

            // Does the task completion time make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
                throw std::runtime_error(
                        "Case 1: Unexpected job duration (should be around " +
                        std::to_string(expected_duration) + " but is " +
                        std::to_string(observed_duration) + ")");
            }

            // Do individual task completion times make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6, task1->getEndDate(),
                                                            EPSILON)) {
                throw std::runtime_error("Case 1: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
            }

            if (!StandardJobExecutorTest::isJustABitGreater(task1->getFlops() / 6 + task2->getFlops() / 6,
                                                            task2->getEndDate(), EPSILON)) {
                throw std::runtime_error("Case 1: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
            }

            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);

            wrench::StorageService::deleteFile(this->getWorkflow()->getFileByID("output_file"),
                                               wrench::FileLocation::LOCATION(this->test->storage_service1));
        }

        /** Case 2: Create two tasks that will run in parallel with the default scheduling options **/
        {
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task2.1", 3600, 6, 6, 0);
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task2.2", 300, 2, 6, 0);
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file"));
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));

            // Create a StandardJob with both tasks
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task1, task2},
                    {
                            {this->getWorkflow()->getFileByID("input_file"),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))}
            );

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();

            // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after = wrench::S4U_Simulation::getClock();

            double observed_duration = after - before;

            double expected_duration = std::max(task1->getFlops() / 6, task2->getFlops() / 4);

            // Does the overall completion time make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
                throw std::runtime_error(
                        "Case 2: Unexpected job duration (should be around " +
                        std::to_string(expected_duration) + " but is " +
                        std::to_string(observed_duration) + ")");
            }

            // Do individual task completion times make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6, task1->getEndDate(),
                                                            EPSILON)) {
                throw std::runtime_error("Case 2: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
            }

            if (!StandardJobExecutorTest::isJustABitGreater(before + task2->getFlops() / 4, task2->getEndDate(),
                                                            EPSILON)) {
                throw std::runtime_error("Case 2: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
            }

            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);
            wrench::StorageService::deleteFile(
                    this->getWorkflow()->getFileByID("output_file"),
                    wrench::FileLocation::LOCATION(this->test->storage_service1));

        }

        /** Case 3: Create three tasks that will run in parallel and then sequential with the default scheduling options **/
        {
            auto task1 = this->getWorkflow()->addTask("task3.1", 3600, 6, 6, 0);
            auto task2 = this->getWorkflow()->addTask("task3.2", 400, 2, 6, 0);
            auto task3 = this->getWorkflow()->addTask("task3.3", 300, 10, 10, 0);
            double task3_parallel_efficiency = 0.6;
            task3->setParallelModel(wrench::ParallelModel::CONSTANTEFFICIENCY(task3_parallel_efficiency));
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file"));
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task3->addInputFile(this->getWorkflow()->getFileByID("input_file"));

            // Create a StandardJob with all three tasks
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task1, task2, task3},
                    {
                            {this->getWorkflow()->getFileByID("input_file"),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))}
            );

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();

            // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after = wrench::S4U_Simulation::getClock();

            double observed_duration = after - before;

            double expected_duration = std::max(task1->getFlops() / 6, task2->getFlops() / 4) +
                                       task3->getFlops() / (task3_parallel_efficiency * 10);

            // Does the job completion time make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
                throw std::runtime_error(
                        "Case 3: Unexpected job duration (should be around " +
                        std::to_string(expected_duration) + " but is " +
                        std::to_string(observed_duration) + ")");
            }

            // Do the individual task completion times make sense
            if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6.0, task1->getEndDate(),
                                                            EPSILON)) {
                throw std::runtime_error("Case 3: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
            }
            if (!StandardJobExecutorTest::isJustABitGreater(before + task2->getFlops() / 4.0, task2->getEndDate(),
                                                            EPSILON)) {
                throw std::runtime_error("Case 3: Unexpected task1 end date: " + std::to_string(task2->getEndDate()));
            }
            if (!StandardJobExecutorTest::isJustABitGreater(
                    task1->getEndDate() + task3->getFlops() / (task3_parallel_efficiency * 10.0),
                    task3->getEndDate(), EPSILON)) {
                throw std::runtime_error("Case 3: Unexpected task3 end date: " + std::to_string(task3->getEndDate()));
            }

            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);
            this->getWorkflow()->removeTask(task3);
            wrench::StorageService::deleteFile(
                    this->getWorkflow()->getFileByID("output_file"),
                    wrench::FileLocation::LOCATION(this->test->storage_service1));

        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, TwoMultiCoreTasksTest) {
    DO_TEST_WITH_FORK(do_TwoMultiCoreTasksTest_test);
}

void StandardJobExecutorTest::do_TwoMultiCoreTasksTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname1 = "Host1";
    std::string hostname2 = "Host1";

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Services
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"})));

    // Create another Storage Services
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname2, {"/disk2"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new TwoMultiCoreTasksTestWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, hostname1)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  MULTI-HOST TEST                                                 **/
/**********************************************************************/

class MultiHostTestWMS : public wrench::WMS {

public:
    MultiHostTestWMS(StandardJobExecutorTest *test,
                     const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                     const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                     std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();


        // WARNING: The StandardJobExecutor unique_ptr is declared here, so that
        // it's not automatically freed after the next basic block is over. In the internals
        // of WRENCH, this is typically take care in various ways (e.g., keep a list of "finished" executors)
        std::shared_ptr<wrench::StandardJobExecutor> executor;

        /** Case 1: Create two tasks that will each run on a different host **/
        {
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task1.1", 3600, 6, 6, 0);
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task1.2", 3600, 6, 6, 0);
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file"));
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));

            // Create a StandardJob with both tasks
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task1, task2},
                    {
                            {this->getWorkflow()->getFileByID("input_file"),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))}
            );

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();

            // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(10, wrench::ComputeService::ALL_RAM)),
                             std::make_pair(wrench::Simulation::getHostnameList()[1],
                                            std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                throw std::runtime_error("Unexpected '" + message->getName() + "' message");
            }

            double after = wrench::S4U_Simulation::getClock();

            double observed_duration = after - before;

            double expected_duration = std::max(task1->getFlops() / 6, task2->getFlops() / 6);

            // Does the task completion time make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
                throw std::runtime_error(
                        "Case 1: Unexpected job duration (should be around " +
                        std::to_string(expected_duration) + " but is " +
                        std::to_string(observed_duration) + ")");
            }

            // Do individual task completion times make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6, task1->getEndDate(),
                                                            EPSILON)) {
                throw std::runtime_error("Case 1: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
            }

            if (!StandardJobExecutorTest::isJustABitGreater(task2->getFlops() / 6, task2->getEndDate(), EPSILON)) {
                throw std::runtime_error("Case 1: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
            }

            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);
            wrench::StorageService::deleteFile(
                    this->getWorkflow()->getFileByID("output_file"),
                    wrench::FileLocation::LOCATION(this->test->storage_service1));

        }

        /** Case 2: Create 4 tasks that will run in best fit manner **/
        {
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task2.1", 3600, 6, 6, 0);
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task2.2", 1000, 2, 2, 0);
            wrench::WorkflowTask *task3 = this->getWorkflow()->addTask("task2.3", 800, 7, 7, 0);
            wrench::WorkflowTask *task4 = this->getWorkflow()->addTask("task2.4", 600, 2, 2, 0);
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file"));
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));

            // Create a StandardJob with both tasks
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task1, task2, task3, task4},
                    {
                            {this->getWorkflow()->getFileByID("input_file"),  wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))}
            );

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();

            // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
            executor = std::shared_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            wrench::Simulation::getHostnameList()[0],
                            job,
                            {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                            std::make_tuple(10, wrench::ComputeService::ALL_RAM)),
                             std::make_pair(wrench::Simulation::getHostnameList()[1],
                                            std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Wait for a message on my mailbox_name
            std::shared_ptr<wrench::SimulationMessage> message;
            try {
                message = wrench::S4U_Mailbox::getMessage(my_mailbox);
            } catch (std::shared_ptr<wrench::NetworkError> &cause) {
                throw std::runtime_error(
                        "Network error while getting reply from StandardJobExecutor!" + cause->toString());
            }

            // Did we get the expected message?
            auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
            if (!msg) {
                auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorFailedMessage>(message);
                std::string error_msg = msg->cause->toString();

                throw std::runtime_error("Unexpected '" + msg->cause->toString() + "' error");

            }

            double after = wrench::S4U_Simulation::getClock();

            double observed_duration = after - before;

            double expected_duration = std::max(
                    std::max(std::max(task1->getFlops() / 6, task2->getFlops() / 2), task4->getFlops() / 2),
                    task3->getFlops() / 8);

            // Does the overall completion time make sense?
            if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration, EPSILON)) {
                throw std::runtime_error(
                        "Case 2: Unexpected job duration (should be around " +
                        std::to_string(expected_duration) + " but is " +
                        std::to_string(observed_duration) + ")");
            }

//        // Do individual task completion times make sense?
//        if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops()/6, task1->getEndDate())) {
//          throw std::runtime_error("Case 2: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
//        }
//
//        if (!StandardJobExecutorTest::isJustABitGreater(before + task2->getFlops()/4, task2->getEndDate())) {
//          throw std::runtime_error("Case 2: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
//        }
            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);
            this->getWorkflow()->removeTask(task3);
            this->getWorkflow()->removeTask(task4);
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, MultiHostTest) {
    DO_TEST_WITH_FORK(do_MultiHostTest_test);
}

void StandardJobExecutorTest::do_MultiHostTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname,
                                                {std::make_pair(hostname,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Services
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk1"})));

    // Create another Storage Services
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/disk2"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new MultiHostTestWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  TERMINATION TEST    #1  (DURING COMPUTATION)                    **/
/**********************************************************************/

class JobTerminationTestDuringAComputationWMS : public wrench::WMS {

public:
    JobTerminationTestDuringAComputationWMS(StandardJobExecutorTest *test,
                                            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                            std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        wrench::StorageService::deleteFile(this->getWorkflow()->getFileByID("input_file"),
                                           wrench::FileLocation::LOCATION(this->test->storage_service1));
        /**  Create a 4-task job and kill it **/
        {
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task1.1", 3600, 6, 6, 0);
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task1.2", 1000, 2, 2, 0);
            wrench::WorkflowTask *task3 = this->getWorkflow()->addTask("task1.3", 800, 7, 7, 0);
            wrench::WorkflowTask *task4 = this->getWorkflow()->addTask("task1.4", 600, 2, 2, 0);
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file1"));
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task2->addOutputFile(this->getWorkflow()->getFileByID("output_file2"));

            // Create a StandardJob with both tasks
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task1, task2, task3, task4},
//                {task1},
                    {
                            {this->getWorkflow()->getFileByID("input_file"),   wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file1"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file2"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))}
            );

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();

            // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            "Host3",
                            job,
                            {std::make_pair("Host3", std::make_tuple(10, wrench::ComputeService::ALL_RAM)),
                             std::make_pair("Host4", std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Sleep 1 second
            wrench::Simulation::sleep(5);

            // Terminate the job
            executor->kill(true);

            // We should be good now, with nothing running

            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);
            this->getWorkflow()->removeTask(task3);
            this->getWorkflow()->removeTask(task4);

        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, JobTerminationTestDuringAComputation) {
    DO_TEST_WITH_FORK(do_JobTerminationTestDuringAComputation_test);
}

void StandardJobExecutorTest::do_JobTerminationTestDuringAComputation_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host3",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Services
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host4", {"/disk1"})));

    // Create another Storage Services
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService("Host4", {"/disk2"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobTerminationTestDuringAComputationWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, "Host3")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService("Host3"));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file1 = this->workflow->addFile("output_file1", 20000.0);
    wrench::WorkflowFile *output_file2 = this->workflow->addFile("output_file2", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}



/**********************************************************************/
/**  TERMINATION TEST    #2 (DURING A TRANSFER)                      **/
/**********************************************************************/

class JobTerminationTestDuringATransferWMS : public wrench::WMS {

public:
    JobTerminationTestDuringATransferWMS(StandardJobExecutorTest *test,
                                         const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                         const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                         std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        /**  Create a 4-task job and kill it **/
        {
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task1.1", 3600, 6, 6, 0);
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task1.2", 1000, 2, 2, 0);
            wrench::WorkflowTask *task3 = this->getWorkflow()->addTask("task1.3", 800, 7, 7, 0);
            wrench::WorkflowTask *task4 = this->getWorkflow()->addTask("task1.4", 600, 2, 2, 0);
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file1"));
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task2->addOutputFile(this->getWorkflow()->getFileByID("output_file2"));

            // Create a StandardJob with both tasks
            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task1, task2, task3, task4},
                    {
                            {this->getWorkflow()->getFileByID("input_file"),   wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file1"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)},
                            {this->getWorkflow()->getFileByID("output_file2"), wrench::FileLocation::LOCATION(
                                    this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))}
            );

            std::string my_mailbox = "test_callback_mailbox";

            double before = wrench::S4U_Simulation::getClock();

            // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
            std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                    new wrench::StandardJobExecutor(
                            test->simulation,
                            my_mailbox,
                            "Host3",
                            job,
                            {std::make_pair("Host3", std::make_tuple(10, wrench::ComputeService::ALL_RAM)),
                             std::make_pair("Host4", std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                            nullptr,
                            false,
                            nullptr,
                            {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                    ));
            executor->start(executor, true, false); // Daemonized, no auto-restart

            // Sleep 48.20 second
            wrench::Simulation::sleep(48.20);

            // Terminate the job
            executor->kill(true);

            // We should be good now, with nothing running

            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);
            this->getWorkflow()->removeTask(task3);
            this->getWorkflow()->removeTask(task4);
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, JobTerminationTestDuringATransfer) {
    DO_TEST_WITH_FORK(do_JobTerminationTestDuringATransfer_test);
}

void StandardJobExecutorTest::do_JobTerminationTestDuringATransfer_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host3",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Services
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host4", {"/disk1"})));

    // Create another Storage Services
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService("Host4", {"/disk2"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobTerminationTestDuringATransferWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, "Host3")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService("Host3"));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file1 = this->workflow->addFile("output_file1", 20000.0);
    wrench::WorkflowFile *output_file2 = this->workflow->addFile("output_file2", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  TERMINATION TEST    #3 (RANDOM TIME)                            **/
/**********************************************************************/

class JobTerminationTestAtRandomTimesWMS : public wrench::WMS {

public:
    JobTerminationTestAtRandomTimesWMS(StandardJobExecutorTest *test,
                                       const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                       const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                       std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        //Type of random number distribution
        std::uniform_real_distribution<double> dist(0, 600);  //(min, max)
        //Mersenne Twister: Good quality random number generator
        std::mt19937 rng;

        // Initialize with non-deterministic seed
        // rng.seed(std::random_device{}());
        // Initialize with deterministic seed!
        rng.seed(666);

        unsigned long NUM_TRIALS = 150;

        for (unsigned long trial = 0; trial < NUM_TRIALS; trial++) { WRENCH_INFO("Trial %lu", trial);

            /**  Create a 4-task job and kill it **/
            {
                wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task" + std::to_string(trial) + ".1", 3600,
                                                                           6, 6, 0);
                wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task" + std::to_string(trial) + ".2", 1000,
                                                                           2, 2, 0);
                wrench::WorkflowTask *task3 = this->getWorkflow()->addTask("task" + std::to_string(trial) + ".3", 800,
                                                                           7, 7, 0);
                wrench::WorkflowTask *task4 = this->getWorkflow()->addTask("task" + std::to_string(trial) + ".4", 600,
                                                                           2, 2, 0);
                task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
//          task1->addOutputFile(workflow->getFileByID("output_file"));

                // Create a StandardJob with both tasks
                wrench::StandardJob *job = job_manager->createStandardJob(
//                {task1, task2, task3, task4},
                        {task1, task3},
//                {task1},
                        {
                                {this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(
                                        this->test->storage_service1)},
//                          {workflow->getFileByID("output_file"), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                        },
                        {
                                std::make_tuple(this->getWorkflow()->getFileByID("input_file"),
                                                wrench::FileLocation::LOCATION(this->test->storage_service1),
                                                wrench::FileLocation::LOCATION(this->test->storage_service2))
//                        std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
//                          workflow->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
//                          wrench::FileLocation::LOCATION(this->test->storage_service2))
                        },
                        {},
                        {
                                std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                                        this->getWorkflow()->getFileByID("input_file"),
                                        wrench::FileLocation::LOCATION(this->test->storage_service2))
                        }
                );

                std::string my_mailbox = "test_callback_mailbox";

                double before = wrench::S4U_Simulation::getClock();

                // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
                std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                        new wrench::StandardJobExecutor(
                                test->simulation,
                                my_mailbox,
                                "Host3",
                                job,
                                {std::make_pair("Host3", std::make_tuple(10, wrench::ComputeService::ALL_RAM)),
                                 std::make_pair("Host4", std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                                nullptr,
                                false,
                                nullptr,
                                {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                        ));
                executor->start(executor, true, false); // Daemonized, no auto-restart

                // Sleep some random number of seconds
                double sleep_time = dist(rng);WRENCH_INFO("Sleeping for %.3lf seconds", sleep_time);
                wrench::Simulation::sleep(sleep_time);

                // Terminate the executor
                WRENCH_INFO("Killing the standard executor");
                executor->kill(true);

                // We should be good now, with nothing running
//          this->getWorkflow()->removeTask(task1);
//          this->getWorkflow()->removeTask(task2);
//          this->getWorkflow()->removeTask(task3);
//          this->getWorkflow()->removeTask(task4);
            }
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, JobTerminationTestAtRandomTimes) {
    DO_TEST_WITH_FORK(do_JobTerminationTestAtRandomTimes_test);
}

void StandardJobExecutorTest::do_JobTerminationTestAtRandomTimes_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host3",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Services
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host4", {"/disk1"})));

    // Create another Storage Services
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService("Host4", {"/disk2"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobTerminationTestAtRandomTimesWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, "Host3")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService("Host3"));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  WORK UNIT TEST (INTERNAL)                                       **/
/**********************************************************************/

TEST_F(StandardJobExecutorTest, WorkUnitTest) {
    DO_TEST_WITH_FORK(do_WorkUnit_test);
}

void StandardJobExecutorTest::do_WorkUnit_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create two WorkUnits
    std::shared_ptr<wrench::Workunit> wu1 = std::make_shared<wrench::Workunit>(
            nullptr,
            (std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>>) {},
            nullptr,
            (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>) {},
            (std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>>) {},
            (std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>>) {});
    std::shared_ptr<wrench::Workunit> wu2 = std::make_shared<wrench::Workunit>(
            nullptr,
            (std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>>) {},
            nullptr,
            (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>) {},
            (std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>>) {},
            (std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>>) {});

    ASSERT_THROW(wrench::Workunit::addDependency(wu1, nullptr), std::invalid_argument);
    ASSERT_THROW(wrench::Workunit::addDependency(nullptr, wu2), std::invalid_argument);

    ASSERT_NO_THROW(wrench::Workunit::addDependency(wu1, wu2));

    ASSERT_NO_THROW(wrench::Workunit::addDependency(wu1, wu2));

    delete simulation;

    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**  NO-TASK TEST                                                    **/
/**********************************************************************/

class NoTaskTestWMS : public wrench::WMS {

public:
    NoTaskTestWMS(StandardJobExecutorTest *test,
                  const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                  const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                  std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    StandardJobExecutorTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                {},
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("input_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service1),
                        wrench::FileLocation::LOCATION(this->test->storage_service2))},
                {},
                {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(
                        this->getWorkflow()->getFileByID("input_file"),
                        wrench::FileLocation::LOCATION(this->test->storage_service2))}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        wrench::Simulation::getHostnameList()[0],
                        job,
                        {std::make_pair(wrench::Simulation::getHostnameList()[0],
                                        std::make_tuple(10, wrench::ComputeService::ALL_RAM))},
                        nullptr,
                        false,
                        nullptr,
                        {{wrench::StandardJobExecutorProperty::TASK_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true, false); // Daemonized, no auto-restart

        // Wait for a message on my mailbox_name
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::StandardJobExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        return 0;
    }
};

TEST_F(StandardJobExecutorTest, NoTaskTest) {
    DO_TEST_WITH_FORK(do_NoTaskTest_test);
}

void StandardJobExecutorTest::do_NoTaskTest_test() {
    // Create and initialize a simulation
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname1 = "Host1";
    std::string hostname2 = "Host2";

    // Create a Compute Service (we don't use it)
    std::shared_ptr<wrench::ComputeService> compute_service;
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService(hostname1,
                                                {std::make_pair(hostname1,
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {})));
    // Create a Storage Services
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname1, {"/disk1"})));

    // Create another Storage Services
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname2, {"/"})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new NoTaskTestWMS(
                    this, {compute_service}, {storage_service1, storage_service2}, hostname1)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    simulation->add(new wrench::FileRegistryService(hostname1));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}
