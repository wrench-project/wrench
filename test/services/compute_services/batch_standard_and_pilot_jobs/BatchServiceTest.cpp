/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>

#include <memory>
#include <wrench/job/PilotJob.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "../../../include/RuntimeAssert.h"

WRENCH_LOG_CATEGORY(batch_service_test, "Log category for BatchServiceTest");

class BatchServiceTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::BatchComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::Simulation> simulation;

    void do_BogusSetupTest_test();

    void do_OneStandardJobTaskTest_test();

    void do_StandardJobFailureTest_test();

    void do_TerminateStandardJobsTest_test();

    void do_TerminatePilotJobsTest_test();

    void do_TwoStandardJobSubmissionTest_test();

    void do_MultipleStandardTaskTest_test();

    void do_PilotJobTaskTest_test();

    void do_StandardPlusPilotJobTaskTest_test();

    void do_InsufficientCoresTaskTest_test();

    void do_BestFitTaskTest_test();

    void do_FirstFitTaskTest_test();

    void do_RoundRobinTask_test();

    void do_noArgumentsJobSubmissionTest_test();

    void do_StandardJobTimeOutTaskTest_test();

    void do_PilotJobTimeOutTaskTest_test();

    void do_StandardJobInsidePilotJobTimeOutTaskTest_test();

    void do_StandardJobInsidePilotJobSucessTaskTest_test();

    void do_InsufficientCoresInsidePilotJobTaskTest_test();

    void do_DifferentBatchAlgorithmsSubmissionTest_test();

    void do_ShutdownWithPendingRunningJobsTest_test();

    void do_ResourceInformationTest_test();

protected:
    ~BatchServiceTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    BatchServiceTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"HostFast\" speed=\"100f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"HostManyCores\" speed=\"1f\" core=\"100\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"RAMHost\" speed=\"1f\" core=\"10\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1024B\" />"
                          "       </host> "
                          "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"3\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host4\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  BOGUS SETUP TEST                                                **/
/**********************************************************************/

class BogusSetupTestWMS : public wrench::ExecutionController {
public:
    BogusSetupTestWMS(BatchServiceTest *test,
                      const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Do nothing
        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, BogusSetupTest) {
#else
TEST_F(BatchServiceTest, BogusSetupTest) {
#endif
    DO_TEST_WITH_FORK(do_BogusSetupTest_test);
}

void BatchServiceTest::do_BogusSetupTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a bogus scheduling algorithm
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                                         {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "BOGUS"}})),
                 std::invalid_argument);

    // Create a Batch Service with a bogus host list
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService(hostname, {}, "",
                                                         {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"}})),
                 std::invalid_argument);

    // Create a Batch Service with a non-homogeneous (speed) host list
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService(hostname, {"Host1", "HostFast"}, "",
                                                         {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"}})),
                 std::invalid_argument);


    // Create a Batch Service with a non-homogeneous (#cores) host list
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService(hostname, {"Host1", "HostManyCores"}, "",
                                                         {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"}})),
                 std::invalid_argument);

    // Create a Batch Service with a non-homogeneous (RAM) host list
    ASSERT_THROW(compute_service = simulation->add(
                         new wrench::BatchComputeService(hostname, {"Host1", "RAMHost"}, "",
                                                         {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"}})),
                 std::invalid_argument);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  STANDARD JOB TERMINATION TEST                                   **/
/**********************************************************************/

class TerminateOneStandardJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    TerminateOneStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                             const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::shared_ptr<wrench::WorkflowTask> task1;
        std::shared_ptr<wrench::WorkflowTask> task2;
        std::shared_ptr<wrench::WorkflowTask> task3;
        std::shared_ptr<wrench::StandardJob> job1;
        std::shared_ptr<wrench::StandardJob> job2;
        std::shared_ptr<wrench::StandardJob> job3;

        // Submit job1 that should start right away
        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            task1 = this->test->workflow->addTask("task1", 60, 1, 1, 0);
            task1->addInputFile(wrench::Simulation::getFileByID("input_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            job1 = job_manager->createStandardJob(
                    {task1},
                    {{task1->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task1->getInputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "300";//time in seconds
            batch_job_args["-c"] = "10"; //number of cores per node
            try {
                job_manager->submitJob(job1, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }
        }

        // Submit job2 that will be stuck in the queue
        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            task2 = this->test->workflow->addTask("task2", 60, 1, 1, 0);
            task2->addInputFile(wrench::Simulation::getFileByID("input_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            job2 = job_manager->createStandardJob(
                    {task2},
                    {
                            {task2->getInputFiles().at(0),
                             wrench::FileLocation::LOCATION(this->test->storage_service1, task2->getInputFiles().at(0))},
                    },
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "300";//time in seconds
            batch_job_args["-c"] = "10"; //number of cores per node
            try {
                job_manager->submitJob(job2, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }
        }

        // Submit job3 that will be stuck in the queue
        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            task3 = this->test->workflow->addTask("task3", 60, 1, 1, 0);
            task3->addInputFile(wrench::Simulation::getFileByID("input_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            job3 = job_manager->createStandardJob(
                    {task3},
                    {
                            {task3->getInputFiles().at(0),
                             wrench::FileLocation::LOCATION(this->test->storage_service1, task3->getInputFiles().at(0))},
                    },
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "300";//time in seconds
            batch_job_args["-c"] = "10"; //number of cores per node
            try {
                job_manager->submitJob(job3, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }
        }

        wrench::Simulation::sleep(1);

        // Coverage
        try {
            job_manager->terminateJob((std::shared_ptr<wrench::StandardJob>) nullptr);
            throw std::runtime_error("Shouldn't be able to terminate a null job");
        } catch (std::invalid_argument &ignore) {}

        // Terminate job2 (which is pending)
        job_manager->terminateJob(job2);

        // Terminate job1 (which is running)
        job_manager->terminateJob(job1);

        // Check that job 3 completes
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

        // Check task1 states
        if (task1->getState() != wrench::WorkflowTask::State::READY) {
            throw std::runtime_error(
                    "Unexpected task1 state: " + wrench::WorkflowTask::stateToString(task1->getState()));
        }

        if (task2->getState() != wrench::WorkflowTask::State::READY) {
            throw std::runtime_error(
                    "Unexpected task2 state: " + wrench::WorkflowTask::stateToString(task1->getState()));
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, TerminateStandardJobsTest)
#else
TEST_F(BatchServiceTest, TerminateStandardJobsTest)
#endif
{
    DO_TEST_WITH_FORK(do_TerminateStandardJobsTest_test);
}

void BatchServiceTest::do_TerminateStandardJobsTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"},
                                                            "", {{wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"}},
                                                            {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new TerminateOneStandardJobSubmissionTestWMS(
                                    this, hostname)));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  PILOT JOB TERMINATION TEST                                   **/
/**********************************************************************/

class TerminateOnePilotJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    TerminateOnePilotJobSubmissionTestWMS(BatchServiceTest *test,
                                          const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        WRENCH_INFO("Creating two pilot jobs");
        auto pjob1 = job_manager->createPilotJob();
        auto pjob2 = job_manager->createPilotJob();

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "300";//time in seconds
        batch_job_args["-c"] = "10"; //number of cores per node
        try {
            WRENCH_INFO("Submitting both pilot jobs, each of them asking for 4 compute nodes and 300 seconds");
            job_manager->submitJob(pjob1, this->test->compute_service, batch_job_args);
            job_manager->submitJob(pjob2, this->test->compute_service, batch_job_args);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Exception: " + std::string(e.what()));
        }

        WRENCH_INFO("Sleeping for 1 second");
        wrench::Simulation::sleep(1);
        WRENCH_INFO("At this point one of the two jobs should be running");
        {
            auto num_running_pilots = job_manager->getNumRunningPilotJobs();
            if (num_running_pilots != 1) {
                throw std::runtime_error("The number of running pilot jobs should be 1 but is " + std::to_string(num_running_pilots));
            }
        }

        // Terminate job 1 (which is running)
        WRENCH_INFO("Terminating pilot job #1");
        job_manager->terminateJob(pjob1);

        WRENCH_INFO("Sleeping another second");
        wrench::Simulation::sleep(1);

        {
            auto num_running_pilots = job_manager->getNumRunningPilotJobs();
            if (num_running_pilots != 1) {
                throw std::runtime_error("The number of running pilot jobs should be 1 but is " + std::to_string(num_running_pilots));
            }
        }

        // Letting pilot job 2 expire...
        // WRENCH_INFO("Terminating pilot job #2");
        // job_manager->terminateJob(pjob2);
        wrench::Simulation::sleep(10000);

        if (pjob1->getState() != wrench::PilotJob::State::TERMINATED) {
            throw std::runtime_error(
                    "Pilot job #1's state should be TERMINATED (instead: " + std::to_string(pjob1->getState()) + ")");
        }
        if (pjob2->getState() != wrench::PilotJob::State::EXPIRED) {
            throw std::runtime_error(
                    "Pilot job #2's state should be EXPIRED (instead: " + std::to_string(pjob2->getState()) + ")");
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, TerminatePilotJobsTest)
#else
TEST_F(BatchServiceTest, TerminatePilotJobsTest)
#endif
{
    DO_TEST_WITH_FORK(do_TerminatePilotJobsTest_test);
}

void BatchServiceTest::do_TerminatePilotJobsTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));


    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new TerminateOnePilotJobSubmissionTestWMS(
                                    this, hostname)));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ONE STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST    **/
/**********************************************************************/

class
        OneStandardJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    OneStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                    const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 60, 2, 2, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            // Coverage: just get the number of hosts
            auto num_hosts = this->test->compute_service->getNumHosts();

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "2";
            batch_job_args["-t"] = "300";//time in seconds
            batch_job_args["-c"] = "4";  //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
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

            // Try to terminate the already terminated job
            try {
                job_manager->terminateJob(job);
            } catch (wrench::ExecutionException &e) {
                if (not std::dynamic_pointer_cast<wrench::NotAllowed>(e.getCause())) {
                    throw std::runtime_error("Got an expected exception, but the failure cause is not NotAllowed");
                }
            }

            this->test->workflow->removeTask(task);

            // Shutdown the compute service, for testing purposes
            this->test->compute_service->stop();

            // Shutdown the compute service, for testing purposes, which should do nothing
            this->test->compute_service->stop();
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, OneStandardJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_OneStandardJobTaskTest_test);
}

void BatchServiceTest::do_OneStandardJobTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));


    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                                            {{wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new OneStandardJobSubmissionTestWMS(
                                    this, hostname)));

    // Create two files
    auto input_file = wrench::Simulation::addFile("input_file", 10000);
    auto output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  STANDARD JOB FAILURE TEST    **/
/**********************************************************************/

class StandardJobFailureTestWMS : public wrench::ExecutionController {
public:
    StandardJobFailureTestWMS(BatchServiceTest *test,
                              const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 60, 2, 2, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));


            // Create a StandardJob with a bogus pre file copy (source and destination are swapped!)
            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "2";
            batch_job_args["-t"] = "300";//time in seconds
            batch_job_args["-c"] = "4";  //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            this->test->workflow->removeTask(task);
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_StandardJobFailureTest) {
#else
TEST_F(BatchServiceTest, StandardJobFailureTest) {
#endif
    DO_TEST_WITH_FORK(do_StandardJobFailureTest_test);
}

void BatchServiceTest::do_StandardJobFailureTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));


    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                                            {{wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new StandardJobFailureTestWMS(
                                    this, hostname)));

    // Create two files
    auto input_file = wrench::Simulation::addFile("input_file", 10000);
    auto output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  TWO STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST    **/
/**********************************************************************/

class TwoStandardJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    TwoStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                    const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks;
        std::map<std::string, std::string> batch_job_args;

        // Create and submit two jobs that should be able to run concurrently
        for (size_t i = 0; i < 2; i++) {
            double task_flops = 10 * 1800;
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->test->workflow->addTask("test_job_1_task_" + std::to_string(i),
                                                          task_flops,
                                                          num_cores, num_cores, ram));
        }

        // Create a Standard Job with only the tasks
        auto standard_job_1 = job_manager->createStandardJob(tasks);

        // Create the BatchComputeService-specific argument
        batch_job_args["-N"] = std::to_string(2);    // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(18000);// Time in seconds (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10);   //number of cores per task1

        // Submit this job to the BatchComputeService service
        //        WRENCH_INFO("Submitting the first job");
        job_manager->submitJob(standard_job_1, this->test->compute_service,
                               batch_job_args);

        // Create and submit a job that needs 2 nodes and 30 minutes
        tasks.clear();
        for (size_t i = 0; i < 2; i++) {
            double task_flops = 10 * 1800;
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->test->workflow->addTask("test_job_2_task_" + std::to_string(i),
                                                          task_flops,
                                                          num_cores, num_cores, ram));
        }

        // Create a Standard Job with only the tasks
        auto standard_job_2 = job_manager->createStandardJob(tasks);

        // Create the BatchComputeService-specific argument
        batch_job_args.clear();
        batch_job_args["-N"] = std::to_string(2);    // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(18000);// Time in seconds (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10);   //number of cores per task1

        // Submit this job to the BatchComputeService service, after sleeping some epsilon
        // to ensure some sequence of the two jobs and not simultaneous events
        wrench::Simulation::sleep(0.000001);
        //        WRENCH_INFO("Submitting the second job");
        job_manager->submitJob(standard_job_2, this->test->compute_service,
                               batch_job_args);

        // Wait for the two execution events
        for (auto const &job: {standard_job_1, standard_job_2}) {
            // Wait for the workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
                auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);
                if (real_event) {
                    if (real_event->standard_job != job) {
                        throw std::runtime_error("Wrong job completion order: got " +
                                                 real_event->standard_job->getName() + " but expected " +
                                                 job->getName());
                    }
                } else {
                    throw std::runtime_error(
                            "Unexpected workflow execution event: " + event->toString());
                }
            } catch (wrench::ExecutionException &e) {
                //ignore (network error or something)
            }

            double completion_time = wrench::Simulation::getCurrentSimulatedDate();
            double expected_completion_time = 1800;
            RUNTIME_DBL_EQ(completion_time, expected_completion_time,
                           "Unexpected job completion time for job " + job->getName() + ": " +
                                   std::to_string(completion_time),
                           0.0001);
        }
        return 0;
    }
};

TEST_F(BatchServiceTest, TwoStandardJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_TwoStandardJobSubmissionTest_test);
}

void BatchServiceTest::do_TwoStandardJobSubmissionTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new TwoStandardJobSubmissionTestWMS(this, hostname)));

    // Create two files
    auto input_file = wrench::Simulation::addFile("input_file", 10000);
    auto output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  ONE PILOT JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class OnePilotJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    OnePilotJobSubmissionTestWMS(BatchServiceTest *test,
                                 const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a pilot job that needs 1 host, 1 code, 0 bytes of RAM and 30 seconds
            auto pilot_job = job_manager->createPilotJob();
            // Re-creating it
            pilot_job = job_manager->createPilotJob();

            // Submit a pilot job to a null compute service
            try {
                job_manager->submitJob(pilot_job, nullptr, {});
                throw std::runtime_error("Should not be able to submit a pilot job to a null compute service");
            } catch (std::invalid_argument &ignore) {
            }

            std::map<std::string, std::string> bogus_batch_job_args;
            bogus_batch_job_args["-N"] = "x";
            bogus_batch_job_args["-t"] = "60";//time in seconds
            bogus_batch_job_args["-c"] = "4"; //number of cores per node

            // Coverage
            pilot_job->getPriority();

            // Submit a pilot job with bogus BatchComputeService jobs
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service,
                                       bogus_batch_job_args);
                throw std::runtime_error("Should not be able to submit a pilot job with bogus arguments");
            } catch (std::invalid_argument &ignore) {
            }

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "60";//time in seconds
            batch_job_args["-c"] = "4"; //number of cores per node

            pilot_job->getState();   // coverage
            pilot_job->getPriority();// coverage

            // Submit a pilot job
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            // Coverage: submit the job again
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service, batch_job_args);
                throw std::runtime_error("Shouldn't be able to submit same job twice");
            } catch (std::invalid_argument &ignore) {}

            // Wait for a workflow execution event (pilot job started)
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Wait for another workflow execution event (pilot job terminated)

            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobExpiredEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }
        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, OnePilotJobSubmissionTest) {
#else
TEST_F(BatchServiceTest, OnePilotJobSubmissionTest) {
#endif
    DO_TEST_WITH_FORK(do_PilotJobTaskTest_test);
}

void BatchServiceTest::do_PilotJobTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"},
                                                            "", {{wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"}}, {})));

    // Create a File Registry Service
    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new OnePilotJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  STANDARD + PILOT JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class StandardPlusPilotJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    StandardPlusPilotJobSubmissionTestWMS(BatchServiceTest *test,
                                          const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 50, 2, 2, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "120";//time in seconds
            batch_job_args["-c"] = "4";  //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
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
            this->test->workflow->removeTask(task);
        }

        {
            // Create a pilot job that needs 1 host, 1 code, 0 bytes of RAM, and 30 seconds
            auto pilot_job = job_manager->createPilotJob();

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "60";//time in seconds
            batch_job_args["-c"] = "4"; //number of cores per node

            // Submit a pilot job
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobExpiredEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, StandardPlusPilotJobSubmissionTest) {
#else
TEST_F(BatchServiceTest, StandardPlusPilotJobSubmissionTest) {
#endif
    DO_TEST_WITH_FORK(do_StandardPlusPilotJobTaskTest_test);
}

void BatchServiceTest::do_StandardPlusPilotJobTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new StandardPlusPilotJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**************************************************************************/
/**INSUFFICIENT CORES JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST**/
/**************************************************************************/

class InsufficientCoresJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    InsufficientCoresJobSubmissionTestWMS(BatchServiceTest *test,
                                          const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Standard Job
        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 50, 2, 12, 0);

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createStandardJob(task);

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "120";//time in seconds
            batch_job_args["-c"] = "12"; //number of cores per node, which is too many!

            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
                throw std::runtime_error("Job Submission should have generated an exception");
            } catch (wrench::ExecutionException &e) {
                auto cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
                if (not cause) {
                    throw std::runtime_error("Got an expected exception but unexpected failure cause: " +
                                             e.getCause()->toString() + " (expected: NotEnoughResources)");
                }
            }

            this->test->workflow->removeTask(task);
        }

        // Pilot Job
        {
            // Create a PilotJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createPilotJob();

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "120";//time in seconds
            batch_job_args["-c"] = "12"; //number of cores per node, which is too many!

            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
                throw std::runtime_error("Job Submission should have generated an exception");
            } catch (wrench::ExecutionException &e) {
                auto cause = std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause());
                if (not cause) {
                    throw std::runtime_error("Got an expected exception but unexpected failure cause: " +
                                             e.getCause()->toString() + " (expected: NotEnoughResources)");
                }
            }
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, InsufficientCoresJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_InsufficientCoresTaskTest_test);
}

void BatchServiceTest::do_InsufficientCoresTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new InsufficientCoresJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  NO ARGUMENTS JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST **/
/**********************************************************************/

class NoArgumentsJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    NoArgumentsJobSubmissionTestWMS(BatchServiceTest *test,
                                    const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 50, 2, 2, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>,
                                std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
                throw std::runtime_error(
                        "Should not have been able to submit jobs without service-specific args");
            } catch (std::invalid_argument &e) {
            }
            this->test->workflow->removeTask(task);
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, NoArgumentsJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_noArgumentsJobSubmissionTest_test);
}

void BatchServiceTest::do_noArgumentsJobSubmissionTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new NoArgumentsJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  STANDARDJOB TIMEOUT TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class StandardJobTimeoutSubmissionTestWMS : public wrench::ExecutionController {
public:
    StandardJobTimeoutSubmissionTestWMS(BatchServiceTest *test,
                                        const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task1 that lasts one min and requires 1 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 65, 1, 1, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "60";//time in seconds
            batch_job_args["-c"] = "4"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
            if (real_event) {
                auto cause = std::dynamic_pointer_cast<wrench::JobTimeout>(real_event->failure_cause);
                if (not cause) {
                    throw std::runtime_error("Expected event, but unexpected failure cause: " +
                                             real_event->failure_cause->toString() + " (expected: JobTimeout)");
                }
                if (cause->getJob() != job) {
                    throw std::runtime_error("Expected JobTimeout failure cause does not point to expected job");
                }
                cause->toString();// for coverage
                // success, do nothing for now
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
            this->test->workflow->removeTask(task);
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, StandardJobTimeOutTask) {
#else
TEST_F(BatchServiceTest, StandardJobTimeOutTask) {
#endif
    DO_TEST_WITH_FORK(do_StandardJobTimeOutTaskTest_test);
}

void BatchServiceTest::do_StandardJobTimeOutTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new StandardJobTimeoutSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  PILOTJOB TIMEOUT TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class PilotJobTimeoutSubmissionTestWMS : public wrench::ExecutionController {
public:
    PilotJobTimeoutSubmissionTestWMS(BatchServiceTest *test,
                                     const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a pilot job that needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
            auto pilot_job = job_manager->createPilotJob();

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "60";//time in seconds
            batch_job_args["-c"] = "4"; //number of cores per node

            // Submit a pilot job
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }

            if (not std::dynamic_pointer_cast<wrench::PilotJobExpiredEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_PilotJobTimeOutTaskTest) {
#else
TEST_F(BatchServiceTest, PilotJobTimeOutTaskTest) {
#endif
    DO_TEST_WITH_FORK(do_PilotJobTimeOutTaskTest_test);
}

void BatchServiceTest::do_PilotJobTimeOutTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new PilotJobTimeoutSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  BEST FIT STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class BestFitStandardJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    BestFitStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                        const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task1 that lasts one min and requires 8 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task", 50, 8, 8, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            //Create another sequential task1 that lasts one min and requires 9 cores
            std::shared_ptr<wrench::WorkflowTask> task1 = this->test->workflow->addTask("task1", 50, 9, 9, 0);
            task1->addInputFile(wrench::Simulation::getFileByID("input_file_1"));
            task1->addOutputFile(wrench::Simulation::getFileByID("output_file_1"));

            //Create another sequential task1 that lasts one min and requires 1 cores
            std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->addTask("task2", 50, 1, 1, 0);
            task2->addInputFile(wrench::Simulation::getFileByID("input_file_2"));
            task2->addOutputFile(wrench::Simulation::getFileByID("output_file_2"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "120";//time in seconds
            batch_job_args["-c"] = "8";  //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            auto job1 = job_manager->createStandardJob(
                    {task1},
                    {{task1->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task1->getInputFiles().at(0))},
                     {task1->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                             this->test->storage_service1, task1->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file_1")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file_1")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file_1")))});

            std::map<std::string, std::string> task1_batch_job_args;
            task1_batch_job_args["-N"] = "1";
            task1_batch_job_args["-t"] = "120";//time in seconds
            task1_batch_job_args["-c"] = "9";  //number of cores per node
            try {
                job_manager->submitJob(job1, this->test->compute_service, task1_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            auto job2 = job_manager->createStandardJob(
                    {task2},
                    {{task2->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task2->getInputFiles().at(0))},
                     {task2->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                             this->test->storage_service1, task2->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file_2")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file_2")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file_2")))});

            std::map<std::string, std::string> task2_batch_job_args;
            task2_batch_job_args["-N"] = "1";
            task2_batch_job_args["-t"] = "120";//time in seconds
            task2_batch_job_args["-c"] = "1";  //number of cores per node
            try {
                job_manager->submitJob(job2, this->test->compute_service, task2_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            //wait for three standard job completion events
            int num_events = 0;
            while (num_events < 3) {
                std::shared_ptr<wrench::ExecutionEvent> event;
                try {
                    event = this->waitForNextEvent();
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }
                if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
                num_events++;
            }

            if (task1->getExecutionHost() != task2->getExecutionHost()) {
                throw std::runtime_error(
                        "BatchServiceTest::BestFitStandardJobSubmissionTest():: BestFit did not pick the right hosts");
            }

            this->test->workflow->removeTask(task);
            this->test->workflow->removeTask(task1);
            this->test->workflow->removeTask(task2);
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_BestFitStandardJobSubmissionTest)
#else
TEST_F(BatchServiceTest, BestFitStandardJobSubmissionTest)
#endif
{
    DO_TEST_WITH_FORK(do_BestFitTaskTest_test);
}

void BatchServiceTest::do_BestFitTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(
                                    hostname,
                                    {"Host1", "Host2", "Host3", "Host4"}, "",
                                    {{wrench::BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM, "BESTFIT"}})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new BestFitStandardJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);
    std::shared_ptr<wrench::DataFile> input_file_1 = wrench::Simulation::addFile("input_file_1", 10000);
    std::shared_ptr<wrench::DataFile> output_file_1 = wrench::Simulation::addFile("output_file_1", 20000);
    std::shared_ptr<wrench::DataFile> input_file_2 = wrench::Simulation::addFile("input_file_2", 10000);
    std::shared_ptr<wrench::DataFile> output_file_2 = wrench::Simulation::addFile("output_file_2", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));
    ASSERT_NO_THROW(storage_service1->createFile(input_file_1));
    ASSERT_NO_THROW(storage_service1->createFile(input_file_2));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  FIRST FIT STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST **/
/**********************************************************************/

class FirstFitStandardJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    FirstFitStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                         const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            //In this test, we create 40 tasks, each task1 uses one core. There are 4 hosts with 10 cores.
            //So the first 10 tasks should run on first host, next 10 tasks should run on second host,
            //next 10 tasks should run on third and last 10 tasks should run on fourth host
            //using first fit host selection scheduling

            int num_tasks = 40;
            int num_cores_in_each_task = 1;
            unsigned long num_hosts_in_platform = 4;
            unsigned long repetition = num_tasks / (num_cores_in_each_task * num_hosts_in_platform);
            std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks = {};
            std::vector<std::shared_ptr<wrench::StandardJob>> jobs = {};
            for (int i = 0; i < num_tasks; i++) {
                tasks.push_back(this->test->workflow->addTask("task1" + std::to_string(i), 59, num_cores_in_each_task,
                                                              num_cores_in_each_task, 0));
                jobs.push_back(job_manager->createStandardJob(tasks[i]));
                std::map<std::string, std::string> args;
                args["-N"] = "1";
                args["-t"] = "60";
                args["-c"] = std::to_string(num_cores_in_each_task);
                try {
                    job_manager->submitJob(jobs[i], this->test->compute_service, args);
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error(
                            "Got some exception");
                }
            }

            //wait for two standard job completion events
            int num_events = 0;
            while (num_events < num_tasks) {
                std::shared_ptr<wrench::ExecutionEvent> event;
                try {
                    event = this->waitForNextEvent();
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }
                if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
                num_events++;
            }

            for (unsigned long i = 0; i < num_hosts_in_platform; i++) {
                for (unsigned long j = i * repetition; j < (((i + 1) * repetition) - 1); j++) {
                    if (tasks[j]->getExecutionHost() != tasks[j + 1]->getExecutionHost()) {
                        throw std::runtime_error(
                                "BatchServiceTest::FirstFitStandardJobSubmissionTest():: The tasks did not execute on the right hosts");
                    }
                }
            }

            for (int i = 0; i < num_tasks; i++) {
                this->test->workflow->removeTask(tasks[i]);
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_FirstFitStandardJobSubmissionTest)
#else
TEST_F(BatchServiceTest, FirstFitStandardJobSubmissionTest)
#endif
{
    DO_TEST_WITH_FORK(do_FirstFitTaskTest_test);
}

void BatchServiceTest::do_FirstFitTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(
                                    hostname,
                                    {"Host1", "Host2", "Host3", "Host4"}, "",
                                    {{wrench::BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM, "BESTFIT"}})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new FirstFitStandardJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);
    std::shared_ptr<wrench::DataFile> input_file_1 = wrench::Simulation::addFile("input_file_1", 10000);
    std::shared_ptr<wrench::DataFile> output_file_1 = wrench::Simulation::addFile("output_file_1", 20000);
    std::shared_ptr<wrench::DataFile> input_file_2 = wrench::Simulation::addFile("input_file_2", 10000);
    std::shared_ptr<wrench::DataFile> output_file_2 = wrench::Simulation::addFile("output_file_2", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));
    ASSERT_NO_THROW(storage_service1->createFile(input_file_1));
    ASSERT_NO_THROW(storage_service1->createFile(input_file_2));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  ROUND ROBIN JOB SUBMISSION TASK SIMULATION TEST               **/
/**********************************************************************/

class RoundRobinStandardJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    RoundRobinStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                           const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();
        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task1_1 = this->test->workflow->addTask("task1_1", 60, 2, 2, 0);
            std::shared_ptr<wrench::WorkflowTask> task1_2 = this->test->workflow->addTask("task1_2", 60, 2, 2, 0);

            //Create another sequential task1 that lasts one min and requires 9 cores
            std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->addTask("task2", 360, 9, 9, 0);

            //Create another sequential task1 that lasts one min and requires 1 core
            std::shared_ptr<wrench::WorkflowTask> task3 = this->test->workflow->addTask("task3", 59, 1, 1, 0);

            //Create another sequential task1 that lasts one min and requires 10 cores
            std::shared_ptr<wrench::WorkflowTask> task4 = this->test->workflow->addTask("task4", 600, 10, 10, 0);

            auto job = job_manager->createStandardJob({task1_1, task1_2});

            auto job2 = job_manager->createStandardJob(task2);

            auto job3 = job_manager->createStandardJob(task3);

            auto job4 = job_manager->createStandardJob(task4);

            std::map<std::string, std::string> task1_batch_job_args;
            task1_batch_job_args["-N"] = "2";
            task1_batch_job_args["-t"] = "60";//time in seconds
            task1_batch_job_args["-c"] = "2"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, task1_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }
            wrench::Simulation::sleep(0.01);

            std::map<std::string, std::string> task2_batch_job_args;
            task2_batch_job_args["-N"] = "1";
            task2_batch_job_args["-t"] = "60";//time in seconds
            task2_batch_job_args["-c"] = "9"; //number of cores per node
            try {
                job_manager->submitJob(job2, this->test->compute_service, task2_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }
            wrench::Simulation::sleep(0.01);

            std::map<std::string, std::string> task3_batch_job_args;
            task3_batch_job_args["-N"] = "1";
            task3_batch_job_args["-t"] = "60";//time in seconds
            task3_batch_job_args["-c"] = "10";//number of cores per node
            try {
                job_manager->submitJob(job3, this->test->compute_service, task3_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }
            wrench::Simulation::sleep(0.01);

            std::map<std::string, std::string> task4_batch_job_args;
            task4_batch_job_args["-N"] = "1";
            task4_batch_job_args["-t"] = "120";//time in seconds
            task4_batch_job_args["-c"] = "10"; //number of cores per node
            try {
                job_manager->submitJob(job4, this->test->compute_service, task4_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            //wait for standard job completion events
            int num_events = 0;
            while (num_events < 4) {
                std::shared_ptr<wrench::ExecutionEvent> event;
                try {
                    event = this->waitForNextEvent();
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }
                if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
                num_events++;
            }

            WRENCH_INFO("Task1_1 completed on host %s", task1_1->getExecutionHost().c_str());
            WRENCH_INFO("Task1_2 completed on host %s", task1_2->getExecutionHost().c_str());
            WRENCH_INFO(
                    "Task2 completed on host %s", task2->getExecutionHost().c_str());
            WRENCH_INFO(
                    "Task3 completed on host %s", task3->getExecutionHost().c_str());
            WRENCH_INFO(
                    "Task4 completed on host %s", task4->getExecutionHost().c_str());

            double EPSILON = 1.0;
            double not_expected_date = 60;//FIRSTFIT and BESTFIT would complete in ~60 seconds but ROUNDROBIN would finish, in this case, in ~90 seconds
            if (std::abs(wrench::Simulation::getCurrentSimulatedDate() - not_expected_date) <= EPSILON) {
                throw std::runtime_error(
                        "BatchServiceTest::ROUNDROBINTEST():: The tasks did not finish on time: Simulation Date > Expected Date");
            } else {
                //congrats, round robin works
                //however let's check further if the task1 hostname is equal to the task4 hostname
                if ((task1_1->getExecutionHost() != task4->getExecutionHost()) and (task1_2->getExecutionHost() != task4->getExecutionHost())) {
                    throw std::runtime_error(
                            "BatchServiceTest::ROUNDROBINTEST():: The tasks did not execute on the right hosts: " +
                            task1_1->getExecutionHost() + "-" +
                            task1_2->getExecutionHost() + "-" +
                            task2->getExecutionHost() + "-" +
                            task3->getExecutionHost() + "-" +
                            task4->getExecutionHost());
                }
            }

            this->test->workflow->removeTask(task1_1);
            this->test->workflow->removeTask(task1_2);
            this->test->workflow->removeTask(task2);
            this->test->workflow->removeTask(task3);
            this->test->workflow->removeTask(task4);
        }

        {
            int num_tasks = 20;
            std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks = {};
            std::vector<std::shared_ptr<wrench::StandardJob>> jobs = {};
            for (int i = 0; i < num_tasks; i++) {
                tasks.push_back(this->test->workflow->addTask("task1" + std::to_string(i), 59, 1, 1, 0));
                jobs.push_back(job_manager->createStandardJob(tasks[i]));
                std::map<std::string, std::string> args;
                args["-N"] = "1";
                args["-t"] = "60";
                args["-c"] = "1";
                try {
                    job_manager->submitJob(jobs[i], this->test->compute_service, args);
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error(
                            "Exception: " + std::string(e.what()));
                }
            }

            //wait for two standard job completion events
            int num_events = 0;
            while (num_events < num_tasks) {
                std::shared_ptr<wrench::ExecutionEvent> event;
                try {
                    event = this->waitForNextEvent();
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }
                if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
                num_events++;
            }

            unsigned long num_hosts = 4;
            for (int i = 0; i < num_tasks; i++) {
                if (tasks[i]->getExecutionHost() != tasks[(i + num_hosts) % num_hosts]->getExecutionHost()) {
                    throw std::runtime_error(
                            "BatchServiceTest::ROUNDROBINTEST():: The tasks in the second test did not execute on the right hosts");
                }
            }

            for (int i = 0; i < num_tasks; i++) {
                this->test->workflow->removeTask(tasks[i]);
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_RoundRobinTaskTest)
#else
TEST_F(BatchServiceTest, RoundRobinTaskTest)
#endif
{
    DO_TEST_WITH_FORK(do_RoundRobinTask_test);
}

void BatchServiceTest::do_RoundRobinTask_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //            argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(
                                    hostname,
                                    {"Host1", "Host2", "Host3", "Host4"}, "",
                                    {{wrench::BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM, "ROUNDROBIN"}})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new RoundRobinStandardJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);
    std::shared_ptr<wrench::DataFile> input_file_1 = wrench::Simulation::addFile("input_file_1", 10000);
    std::shared_ptr<wrench::DataFile> output_file_1 = wrench::Simulation::addFile("output_file_1", 20000);
    std::shared_ptr<wrench::DataFile> input_file_2 = wrench::Simulation::addFile("input_file_2", 10000);
    std::shared_ptr<wrench::DataFile> output_file_2 = wrench::Simulation::addFile("output_file_2", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));
    ASSERT_NO_THROW(storage_service1->createFile(input_file_1));
    ASSERT_NO_THROW(storage_service1->createFile(input_file_2));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/***********************************************************************************************/
/**  STANDARDJOB INSIDE PILOT JOB FAILURE TASK SIMULATION TEST ON ONE-ONE HOST                **/
/***********************************************************************************************/

class StandardJobInsidePilotJobTimeoutSubmissionTestWMS : public wrench::ExecutionController {
public:
    StandardJobInsidePilotJobTimeoutSubmissionTestWMS(
            BatchServiceTest *test,
            const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a pilot job that needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
            auto pilot_job = job_manager->createPilotJob();

            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 60, 2, 2, 0);
            std::shared_ptr<wrench::DataFile> file1 = wrench::Simulation::getFileByID("input_file");
            std::shared_ptr<wrench::DataFile> file2 = wrench::Simulation::getFileByID("output_file");
            task->addInputFile(file1);
            task->addOutputFile(file2);

            std::map<std::string, std::string> pilot_batch_job_args;
            pilot_batch_job_args["-N"] = "1";
            pilot_batch_job_args["-t"] = "120";//time in seconds
            pilot_batch_job_args["-c"] = "4";  //number of cores per node

            // Submit a pilot job
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service,
                                       pilot_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception " + std::string(e.what()));
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Create a StandardJob with some pre-copies and post-deletions
            auto job = job_manager->createStandardJob(
                    {task}, {{file1, wrench::FileLocation::LOCATION(this->test->storage_service1, file1)}}, {}, {}, {});

            try {
                job_manager->submitJob(job, pilot_job->getComputeService(), {});
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            // Coverage
            try {
                job_manager->terminateJob((std::shared_ptr<wrench::PilotJob>) nullptr);
                throw std::runtime_error("Shouldn't be able to terminate a null job");
            } catch (std::invalid_argument &ignore) {}

            // Terminate the pilot job while it's running a standard job
            try {
                job_manager->terminateJob(pilot_job);
            } catch (std::exception &e) {
                throw std::runtime_error("Unexpected exception while terminating pilot job: " + std::string(e.what()));
            }

            // Wait for the standard job failure notification
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Error while getting and execution event: " + e.getCause()->toString());
            }
            auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
            if (real_event) {
                auto cause = std::dynamic_pointer_cast<wrench::JobTimeout>(real_event->failure_cause);
                if (not cause) {
                    throw std::runtime_error("Got a job failure event but unexpected failure cause: " +
                                             real_event->failure_cause->toString() + " (expected: JobTimeout)");
                }
                std::string error_msg = cause->toString();
                if (cause->getJob() != job) {
                    throw std::runtime_error(
                            "Got the correct failure even, a correct cause type, but the cause points to the wrong job");
                }
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            this->test->workflow->removeTask(task);
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, StandardJobInsidePilotJobTimeOutTaskTest) {
    DO_TEST_WITH_FORK(do_StandardJobInsidePilotJobTimeOutTaskTest_test);
}

void BatchServiceTest::do_StandardJobInsidePilotJobTimeOutTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"}, "/scratch")));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new StandardJobInsidePilotJobTimeoutSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  STANDARDJOB INSIDE PILOT JOB SUCESS TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class StandardJobInsidePilotJobSucessSubmissionTestWMS : public wrench::ExecutionController {
public:
    StandardJobInsidePilotJobSucessSubmissionTestWMS(BatchServiceTest *test,
                                                     const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a pilot job that needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
            auto pilot_job = job_manager->createPilotJob();

            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 60, 2, 2, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            std::map<std::string, std::string> pilot_batch_job_args;
            pilot_batch_job_args["-N"] = "1";
            pilot_batch_job_args["-t"] = "120";//time in seconds
            pilot_batch_job_args["-c"] = "4";  //number of cores per node

            // Submit a pilot job
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service,
                                       pilot_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception " + std::string(e.what()));
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Create a StandardJob with some pre-copies and post-deletions
            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>,
                                std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            try {
                job_manager->submitJob(job, pilot_job->getComputeService(), {});
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
            }

            // Wait for the standard job success notification
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            this->test->workflow->removeTask(task);
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, StandardJobInsidePilotJobSucessTaskTest) {
    DO_TEST_WITH_FORK(do_StandardJobInsidePilotJobSucessTaskTest_test);
}

void BatchServiceTest::do_StandardJobInsidePilotJobSucessTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new StandardJobInsidePilotJobSucessSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**************************************************************************/
/**  INSUFFICIENT CORES INSIDE PILOT JOB SIMULATION TEST ON ONE-ONE HOST **/
/**************************************************************************/

class InsufficientCoresInsidePilotJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    InsufficientCoresInsidePilotJobSubmissionTestWMS(
            BatchServiceTest *test,
            const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a pilot job that needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
            auto pilot_job = job_manager->createPilotJob();

            // Create a sequential task1 that lasts one min and requires 5 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 60, 5, 5, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            std::map<std::string, std::string> pilot_batch_job_args;
            pilot_batch_job_args["-N"] = "1";
            pilot_batch_job_args["-t"] = "120";//time in seconds
            pilot_batch_job_args["-c"] = "4";  //number of cores per node

            // Submit a pilot job
            try {
                job_manager->submitJob(pilot_job, this->test->compute_service,
                                       pilot_batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception " + std::string(e.what()));
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::PilotJobStartedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Create a StandardJob with some pre-copies and post-deletions
            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>,
                                std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            try {
                job_manager->submitJob(job, pilot_job->getComputeService(), {{task->getID(), "5"}});
                throw std::runtime_error(
                        "Expected a runtime error of insufficient cores in pilot job");
            } catch (wrench::ExecutionException &e) {
            }

            this->test->workflow->removeTask(task);
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, InsufficientCoresInsidePilotJobTaskTest) {
    DO_TEST_WITH_FORK(do_InsufficientCoresInsidePilotJobTaskTest_test);
}

void BatchServiceTest::do_InsufficientCoresInsidePilotJobTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new InsufficientCoresInsidePilotJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/***********************************************************************/
/** MULTIPLE STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST **/
/***********************************************************************/

class MultipleStandardJobSubmissionTestWMS : public wrench::ExecutionController {
public:
    MultipleStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                         const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            int num_standard_jobs = 10;
            int each_task_time = 60;//in seconds
            std::vector<std::shared_ptr<wrench::StandardJob>> jobs;
            std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks;
            for (int i = 0; i < num_standard_jobs; i++) {
                // Create a sequential task1 that lasts for random seconds and requires 2 cores
                std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1" + std::to_string(i), each_task_time, 2,
                                                                                           2, 0);
                auto job = job_manager->createStandardJob(task);
                tasks.push_back(task);
                jobs.push_back(job);
            }

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = std::to_string((each_task_time) *num_standard_jobs);//time in seconds
            batch_job_args["-c"] = "2";                                                //number of cores per node
            for (const auto &standard_jobs: jobs) {
                try {
                    job_manager->submitJob(standard_jobs, this->test->compute_service, batch_job_args);
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error(
                            "Exception: " + std::string(e.what()));
                }
            }

            for (int i = 0; i < num_standard_jobs; i++) {

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
            }

            for (const auto &each_task: tasks) {
                this->test->workflow->removeTask(each_task);
            }
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, MultipleStandardJobSubmissionTest) {
    DO_TEST_WITH_FORK(do_MultipleStandardTaskTest_test);
}

void BatchServiceTest::do_MultipleStandardTaskTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new MultipleStandardJobSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/***********************************************************************/
/** DIFFERENT BATCHSERVICE ALGORITHMS SUBMISSION TASK SIMULATION TEST **/
/***********************************************************************/

class DifferentBatchAlgorithmsSubmissionTestWMS : public wrench::ExecutionController {
public:
    DifferentBatchAlgorithmsSubmissionTestWMS(BatchServiceTest *test,
                                              const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task1 that lasts one min and requires 2 cores
            std::shared_ptr<wrench::WorkflowTask> task = this->test->workflow->addTask("task1", 60, 2, 2, 0);
            task->addInputFile(wrench::Simulation::getFileByID("input_file"));
            task->addOutputFile(wrench::Simulation::getFileByID("output_file"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createStandardJob(
                    {task},
                    {{task->getInputFiles().at(0), wrench::FileLocation::LOCATION(
                                                           this->test->storage_service1, task->getInputFiles().at(0))},
                     {task->getOutputFiles().at(0), wrench::FileLocation::LOCATION(
                                                            this->test->storage_service1, task->getOutputFiles().at(0))}},
                    {std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            wrench::FileLocation::LOCATION(this->test->storage_service1, wrench::Simulation::getFileByID("input_file")),
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))},
                    {},
                    {std::shared_ptr<wrench::FileLocation>(
                            wrench::FileLocation::LOCATION(this->test->storage_service2, wrench::Simulation::getFileByID("input_file")))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "300";//time in seconds
            batch_job_args["-c"] = "4";  //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error(
                        "Exception: " + std::string(e.what()));
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
            this->test->workflow->removeTask(task);
        }

        return 0;
    }
};


#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DifferentBatchAlgorithmsSubmissionTest)
#else
TEST_F(BatchServiceTest, DISABLED_DifferentBatchAlgorithmsSubmissionTest)
#endif
{
    DO_TEST_WITH_FORK(do_DifferentBatchAlgorithmsSubmissionTest_test);
}

void BatchServiceTest::do_DifferentBatchAlgorithmsSubmissionTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk1"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname, {"/disk2"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(
                                    hostname,
                                    {"Host1", "Host2", "Host3", "Host4"}, "", {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "filler"}, {wrench::BatchComputeServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM, "fcfs"}})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new DifferentBatchAlgorithmsSubmissionTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two files
    std::shared_ptr<wrench::DataFile> input_file = wrench::Simulation::addFile("input_file", 10000);
    std::shared_ptr<wrench::DataFile> output_file = wrench::Simulation::addFile("output_file", 20000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(input_file));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  SHUTDOWN WITH PENDING/RUNNING JOBS TEST **/
/**********************************************************************/

class ShutdownWithPendingRunningJobsTestWMS : public wrench::ExecutionController {
public:
    ShutdownWithPendingRunningJobsTestWMS(BatchServiceTest *test,
                                          const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
        this->test = test;
    }

private:
    BatchServiceTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create 3 tasks
        std::shared_ptr<wrench::WorkflowTask> tasks[3];
        for (int i = 0; i < 3; i++) {
            tasks[i] = this->test->workflow->addTask("task1" + std::to_string(i), 600, 10, 10, 0);
        }

        // Submit them individually
        std::shared_ptr<wrench::StandardJob> jobs[3];

        for (int i = 0; i < 3; i++) {
            jobs[i] = job_manager->createStandardJob(tasks[i]);

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "1";
            batch_job_args["-t"] = "120";//time in seconds
            batch_job_args["-c"] = "10"; //number of cores per node
            try {
                job_manager->submitJob(jobs[i], this->test->compute_service, batch_job_args);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Exception: " + std::string(e.what()));
            }
        }

        // Sleep 5 seconds
        wrench::Simulation::sleep(5);

        // Terminate the service
        this->test->compute_service->stop(true, wrench::ComputeService::TerminationCause::TERMINATION_JOB_KILLED);

        // Sleep 5 seconds
        wrench::Simulation::sleep(5);

        // Wait for workflow execution events
        for (int i = 0; i < 3; i++) {
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
                throw std::runtime_error(
                        "Should have received a STANDARD_JOB_FAILURE event (received " + event->toString());
            }

            auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event);
            auto cause = std::dynamic_pointer_cast<wrench::JobKilled>(real_event->failure_cause);
            if (not cause) {
                throw std::runtime_error("Expected event, but unexpected failure cause: " +
                                         real_event->failure_cause->toString() + " (expected: JobKilled)");
            }
            if ((cause->getJob() != jobs[0]) and
                (cause->getJob() != jobs[1]) and
                (cause->getJob() != jobs[2])) {
                throw std::runtime_error("Expected JobKilled failure cause does not point to expected job");
            }
            cause->toString();// for coverage
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, ShutdownWithPendingRunningJobsTest) {
    DO_TEST_WITH_FORK(do_ShutdownWithPendingRunningJobsTest_test);
}

void BatchServiceTest::do_ShutdownWithPendingRunningJobsTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService(hostname,
                                                            {"Host1"}, "", {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new ShutdownWithPendingRunningJobsTestWMS(
                                    this, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
