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

class WMSTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::CloudComputeService> cs_cloud = nullptr;
    std::shared_ptr<wrench::BatchComputeService> cs_batch = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::DataFile> small_file = nullptr;
    std::shared_ptr<wrench::DataFile> big_file = nullptr;

    std::shared_ptr<wrench::Workflow> workflow;

    void do_DefaultHandlerWMS_test();
    void do_CustomHandlerWMS_test();

protected:
    WMSTest() {
        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"4\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"4\"> "
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
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }


    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  WMS REACTING TO EVENTS : DEFAULT HANDLERS                       **/
/**********************************************************************/

class TestDefaultHandlerWMS : public wrench::ExecutionController {

public:
    TestDefaultHandlerWMS(WMSTest *test,
                          double sleep_time,
                          std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test), sleep_time(sleep_time) {
    }

private:
    WMSTest *test;
    double sleep_time;

    int main() override {

        wrench::Simulation::sleep(this->sleep_time);

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create and start a VM on the cloud service
        auto cloud = this->test->cs_cloud;
        auto vm_name = cloud->createVM(4, 0.0);
        auto vm_cs = cloud->startVM(vm_name);
        vm_cs->getPhysicalHostname();// coverage

        // Get a "STANDARD JOB COMPLETION" event (default handler)
        auto task1 = this->test->workflow->addTask("task1", 10.0, 1, 1, 0);
        auto job1 = job_manager->createStandardJob(task1);
        job_manager->submitJob(job1, vm_cs);
        this->waitForAndProcessNextEvent();

        auto batch = this->test->cs_batch;

        // Get a "PILOT JOB STARTED" event (default handler)
        // This pilot job takes the whole machine!
        auto job2 = job_manager->createPilotJob();
        job_manager->submitJob(job2, batch, {{"-N", "1"}, {"-t", "50"}, {"-c", "4"}});
        this->waitForAndProcessNextEvent();

        // Submit another pilot job, which won't be running for a while
        auto job2_1 = job_manager->createPilotJob();
        job_manager->submitJob(job2_1, batch, {{"-N", "1"}, {"-t", "50"}, {"-c", "4"}});
        // Get the list of pending pilot jobs

        // Get a "STANDARD JOB FAILED" and "PILOT JOB EXPIRED" event (default handler)
        std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->addTask("task2", 100.0, 1, 1, 0);
        auto job3 = job_manager->createStandardJob(task2);
        job_manager->submitJob(job3, job2->getComputeService());
        this->waitForAndProcessNextEvent();
        this->waitForAndProcessNextEvent();

        // Get a "PILOT JOB STARTED" event (default handler)  (for job2_1)
        this->waitForAndProcessNextEvent();
        // Get a "PILOT JOB EXPIRED" event (default handler)   (for job2_1)
        this->waitForAndProcessNextEvent();


        // Get a "FILE COPY COMPLETION" event (default handler)
        data_movement_manager->initiateAsynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service1, this->test->small_file),
                wrench::FileLocation::LOCATION(this->test->storage_service2, this->test->small_file),
                nullptr);
        this->waitForAndProcessNextEvent();

        // Get a "FILE COPY FAILURE" event (default handler)
        data_movement_manager->initiateAsynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service1, this->test->big_file),
                wrench::FileLocation::LOCATION(this->test->storage_service2, this->test->big_file),
                nullptr);
        this->waitForAndProcessNextEvent();

        // Get a "COMPOUND JOB COMPLETED" event (default handler)
        auto sleep_job = job_manager->createCompoundJob("sleep_job");
        sleep_job->addSleepAction("sleep_action", 1.0);
        job_manager->submitJob(sleep_job, vm_cs);

        this->waitForAndProcessNextEvent();

        // Set a timer
        double timer_off_date = wrench::Simulation::getCurrentSimulatedDate() + 10;
        this->setTimer(timer_off_date, "timer went off");
        this->waitForAndProcessNextEvent();
        if (std::abs(wrench::Simulation::getCurrentSimulatedDate() - timer_off_date) > 0.1) {
            throw std::runtime_error("Did not get the timer event at the right date (" +
                                     std::to_string(wrench::Simulation::getCurrentSimulatedDate()) + " instead of " +
                                     std::to_string(timer_off_date) + ")");
        }
        return 0;
    }
};


TEST_F(WMSTest, DefaultEventHandling) {
    DO_TEST_WITH_FORK(do_DefaultHandlerWMS_test);
}

void WMSTest::do_DefaultHandlerWMS_test() {
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
    std::string hostname1 = "Host1";
    std::string hostname2 = "Host2";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname1, {"/"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname2, {"/"})));

    // Create a Cloud Service
    std::vector<std::string> cloud_hosts;
    cloud_hosts.push_back(hostname1);
    ASSERT_NO_THROW(cs_cloud = simulation->add(
                            new wrench::CloudComputeService(hostname1, cloud_hosts, "/scratch", {}, {})));

    // Create a Batch Service
    std::vector<std::string> batch_hosts;
    batch_hosts.push_back(hostname2);
    ASSERT_NO_THROW(cs_batch = simulation->add(
                            new wrench::BatchComputeService(hostname2, batch_hosts, "/scratch",
                                                            {},
                                                            //                    {{wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "false"}},
                                                            {})));


    // Create a WMS
    workflow = wrench::Workflow::createWorkflow();
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new TestDefaultHandlerWMS(this, 100, hostname1)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname1)));

    // Create two files
    this->small_file = wrench::Simulation::addFile("small", 10);
    this->big_file = wrench::Simulation::addFile("big", 1000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(this->small_file));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  WMS REACTING TO EVENTS : CUSTOM HANDLERS                       **/
/**********************************************************************/

class TestCustomHandlerWMS : public wrench::ExecutionController {

public:
    TestCustomHandlerWMS(WMSTest *test,
                         double sleep_time,
                         std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test), sleep_time(sleep_time) {
    }

private:
    WMSTest *test;
    double sleep_time;

    int counter;

    int main() override {

        wrench::Simulation::sleep(this->sleep_time);

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        this->counter = 0;

        // Get a "STANDARD JOB COMPLETION" event (default handler)
        auto cloud = this->test->cs_cloud;
        auto vm_name = cloud->createVM(4, 0.0);
        auto vm_cs = cloud->startVM(vm_name);

        std::shared_ptr<wrench::WorkflowTask> task1 = this->test->workflow->addTask("task1", 10.0, 1, 1, 0);
        auto job1 = job_manager->createStandardJob(task1);
        job_manager->submitJob(job1, vm_cs);
        this->waitForAndProcessNextEvent();
        if (this->counter != 1) {
            throw std::runtime_error("Did not get expected StandardJobCompletionEvent");
        }

        // Get a "PILOT JOB STARTED" event (default handler)
        auto job2 = job_manager->createPilotJob();
        job_manager->submitJob(job2, this->test->cs_batch, {{"-N", "1"}, {"-c", "1"}, {"-t", "2"}});
        this->waitForAndProcessNextEvent();
        if (this->counter != 2) {
            throw std::runtime_error("Did not get expected PilotJobStartEvent");
        }

        // Get a "STANDARD JOB FAILED" and "PILOT JOB EXPIRED" event (default handler)
        std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->addTask("task2", 200.0, 1, 1, 0);
        auto job3 = job_manager->createStandardJob(task2);
        job_manager->submitJob(job3, job2->getComputeService());
        this->waitForAndProcessNextEvent();
        if (this->counter != 3) {
            throw std::runtime_error("Did not get expected StandardJobFailedEvent");
        }
        this->waitForAndProcessNextEvent();
        if (this->counter != 4) {
            throw std::runtime_error("Did not get expected PilotJobExpiredEvent");
        }

        // Get a "FILE COPY COMPLETION" event (default handler)
        data_movement_manager->initiateAsynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service1, this->test->small_file),
                wrench::FileLocation::LOCATION(this->test->storage_service2, this->test->small_file), nullptr);
        this->waitForAndProcessNextEvent();
        if (this->counter != 5) {
            throw std::runtime_error("Did not get expected FileCompletedEvent");
        }

        // Get a "FILE COPY FAILURE" event (default handler)
        data_movement_manager->initiateAsynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service1, this->test->big_file),
                wrench::FileLocation::LOCATION(this->test->storage_service2, this->test->big_file), nullptr);
        this->waitForAndProcessNextEvent();
        if (this->counter != 6) {
            throw std::runtime_error("Did not get expected FileCopyFailureEvent");
        }

        // Set a timer
        double timer_off_date = wrench::Simulation::getCurrentSimulatedDate() + 10;
        this->setTimer(timer_off_date, "timer went off");
        this->waitForAndProcessNextEvent();
        if (std::abs(wrench::Simulation::getCurrentSimulatedDate() - timer_off_date) > 0.1) {
            throw std::runtime_error("Did not get the timer event at the right date (" +
                                     std::to_string(wrench::Simulation::getCurrentSimulatedDate()) +
                                     " instead of " +
                                     std::to_string(timer_off_date));
        }
        if (this->counter != 7) {
            throw std::runtime_error("Did not get expected TimerEvent");
        }

        // Get a "COMPOUND JOB COMPLETED" event (default handler)
        auto sleep_job = job_manager->createCompoundJob("sleep_job");
        sleep_job->addSleepAction("sleep_action", 1.0);
        job_manager->submitJob(sleep_job, vm_cs);

        this->waitForAndProcessNextEvent();
        if (this->counter != 8) {
            throw std::runtime_error("Did not get expected CompoundJobCompleted event");
        }

        // Get a "COMPOUND JOB FAILED" event (default handler)
        auto fail_job = job_manager->createCompoundJob("fail_job");
        auto file = wrench::Simulation::addFile("somefile", 1);
        fail_job->addFileReadAction("file_read_action",
                                    wrench::FileLocation::LOCATION(this->test->storage_service1, file));
        job_manager->submitJob(fail_job, vm_cs);

        this->waitForAndProcessNextEvent();
        if (this->counter != 9) {
            throw std::runtime_error("Did not get expected CompoundJobCompleted event");
        }


        return 0;
    }

    void processEventStandardJobCompletion(const std::shared_ptr<wrench::StandardJobCompletedEvent> &event) override {
        this->counter = 1;
    }

    void processEventPilotJobStart(const std::shared_ptr<wrench::PilotJobStartedEvent> &event) override {
        this->counter = 2;
    }

    void processEventStandardJobFailure(const std::shared_ptr<wrench::StandardJobFailedEvent> &event) override {
        this->counter = 3;
    }

    void processEventPilotJobExpiration(const std::shared_ptr<wrench::PilotJobExpiredEvent> &event) override {
        this->counter = 4;
    }

    void processEventFileCopyCompletion(const std::shared_ptr<wrench::FileCopyCompletedEvent> &event) override {
        this->counter = 5;
    }

    void processEventFileCopyFailure(const std::shared_ptr<wrench::FileCopyFailedEvent> &event) override {
        this->counter = 6;
    }

    void processEventTimer(const std::shared_ptr<wrench::TimerEvent> &event) override {
        this->counter = 7;
    }

    void processEventCompoundJobCompletion(const std::shared_ptr<wrench::CompoundJobCompletedEvent> &event) override {
        this->counter = 8;
    }

    void processEventCompoundJobFailure(const std::shared_ptr<wrench::CompoundJobFailedEvent> &event) override {
        this->counter = 9;
    }
};

TEST_F(WMSTest, CustomEventHandling) {
    DO_TEST_WITH_FORK(do_CustomHandlerWMS_test);
}

void WMSTest::do_CustomHandlerWMS_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname1 = "Host1";
    std::string hostname2 = "Host2";

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname1, {"/"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService(hostname2, {"/"})));

    // Create a Cloud Service
    std::vector<std::string> cloud_hosts;
    cloud_hosts.push_back(hostname1);
    ASSERT_NO_THROW(cs_cloud = simulation->add(
                            new wrench::CloudComputeService(hostname1, cloud_hosts, "/scratch", {}, {})));

    // Create a Batch Service
    std::vector<std::string> batch_hosts;
    batch_hosts.push_back(hostname1);
    ASSERT_NO_THROW(cs_batch = simulation->add(
                            new wrench::BatchComputeService(hostname2, batch_hosts, "/scratch", {}, {})));


    // Create a WMS
    workflow = wrench::Workflow::createWorkflow();
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new TestCustomHandlerWMS(this, 100, hostname1)));

    // Create a file registry
    ASSERT_NO_THROW(simulation->add(
            new wrench::FileRegistryService(hostname1)));

    // Create two files
    this->small_file = wrench::Simulation::addFile("small", 10);
    this->big_file = wrench::Simulation::addFile("big", 1000);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(storage_service1->createFile(this->small_file));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
