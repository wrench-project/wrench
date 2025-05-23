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
#include <wrench/simulation/SimulationMessage.h>
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/util/TraceFileLoader.h>
#include <wrench/job/PilotJob.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(job_manager_test, "Log category for JobManagerTest");


class JobManagerTest : public ::testing::Test {


public:
    std::shared_ptr<wrench::Workflow> workflow;
    std::shared_ptr<wrench::ComputeService> cs1, cs2;
    std::shared_ptr<wrench::ComputeService> cs;

    std::shared_ptr<wrench::Simulation> simulation;

    void do_JobManagerConstructorTest_test();

    void do_JobManagerCreateJobTest_test();

    void do_JobManagerSubmitJobTest_test();

    void do_JobManagerResubmitJobTest_test();

    void do_JobManagerTerminateJobTest_test();


protected:
    JobManagerTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>  "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"101B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "          <prop id=\"ram\" value=\"100B\" />"
                          "       </host> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"1us\"/>"
                          "       <link id=\"2\" bandwidth=\"5000GBps\" latency=\"1us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  DO CONSTRUCTOR TEST                                             **/
/**********************************************************************/

class JobManagerConstructorTestWMS : public wrench::ExecutionController {

public:
    JobManagerConstructorTestWMS(JobManagerTest *test,
                                 std::shared_ptr<wrench::Workflow> workflow,
                                 std::string hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }


private:
    JobManagerTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Kill the Job Manager abruptly, just for kicks
        job_manager->kill();

        return 0;
    }
};

TEST_F(JobManagerTest, ConstructorTest) {
    DO_TEST_WITH_FORK(do_JobManagerConstructorTest_test);
}

void JobManagerTest::do_JobManagerConstructorTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new JobManagerConstructorTestWMS(
                                    this, workflow, hostname)));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO CREATE JOB TEST                                             **/
/**********************************************************************/

class JobManagerCreateJobTestWMS : public wrench::ExecutionController {

public:
    JobManagerCreateJobTestWMS(JobManagerTest *test,
                               std::string hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }


private:
    JobManagerTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        try {
            job_manager->createStandardJob(nullptr);
            throw std::runtime_error("Should not be able to create a standard job with a nullptr task1 in it");
        } catch (std::invalid_argument &e) {
        }


        // Create a job with a nullptr task1 in it
        try {
            job_manager->createStandardJob((std::vector<std::shared_ptr<wrench::WorkflowTask>>){nullptr});
            throw std::runtime_error("Should not be able to create a standard job with a nullptr task1 in it");
        } catch (std::invalid_argument &e) {
        }


        // Create a job  with nothing in it
        try {
            std::vector<std::shared_ptr<wrench::WorkflowTask>> tasks;// empty
            job_manager->createStandardJob(tasks);
            throw std::runtime_error("Should not be able to create a standard job with nothing in it");
        } catch (std::invalid_argument &e) {
        }


        std::shared_ptr<wrench::WorkflowTask> t1 = this->test->workflow->addTask("t1", 1.0, 1, 1, 0.0);
        std::shared_ptr<wrench::WorkflowTask> t2 = this->test->workflow->addTask("t2", 1.0, 1, 1, 0.0);
        std::shared_ptr<wrench::DataFile> f = wrench::Simulation::addFile("f", 100);
        t1->addOutputFile(f);
        t2->addInputFile(f);

        // Create a job with a null stuff in pre file copy
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
            pre_file_copies.push_back(std::make_tuple(wrench::FileLocation::SCRATCH(nullptr), wrench::FileLocation::SCRATCH(nullptr)));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a (nullptr, *, *) pre file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
            pre_file_copies.push_back(std::make_tuple(nullptr, wrench::FileLocation::SCRATCH(f)));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, nullptr, *) pre file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
            pre_file_copies.push_back(std::make_tuple(wrench::FileLocation::SCRATCH(f), nullptr));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, *, nullptr) pre file copy");
        } catch (std::invalid_argument &e) {
        }


        // Create a job with a null stuff in post file copy
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> post_file_copies;
            post_file_copies.push_back(std::make_tuple(wrench::FileLocation::SCRATCH(nullptr), wrench::FileLocation::SCRATCH(nullptr)));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a (nullptr, *, *) post file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> post_file_copies;
            post_file_copies.push_back(std::make_tuple(nullptr, wrench::FileLocation::SCRATCH(f)));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, nullptr, *) post file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> post_file_copies;
            post_file_copies.push_back(std::make_tuple(wrench::FileLocation::SCRATCH(f), nullptr));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, *, nullptr) post file copy");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with SCRATCH as both src and dst
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> pre_file_copies;
            pre_file_copies.push_back(std::make_tuple(wrench::FileLocation::SCRATCH(f), wrench::FileLocation::SCRATCH(f)));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a pre file copy that has SCRATCH has both dst and src");
        } catch (std::invalid_argument &e) {
        }
        // Create a job with SCRATCH as both src and dst
        try {
            std::vector<std::tuple<std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>> post_file_copies;
            post_file_copies.push_back(std::make_tuple(wrench::FileLocation::SCRATCH(f), wrench::FileLocation::SCRATCH(f)));
            job_manager->createStandardJob({}, (std::map<std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a post file copy that has SCRATCH has both dst and src");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with not ok task1 dependencies
        try {
            job_manager->createStandardJob((std::vector<std::shared_ptr<wrench::WorkflowTask>>){t2});
            throw std::runtime_error("Should not be able to create a standard job with a not-self-contained task1");
        } catch (std::invalid_argument &e) {
        }


        // Create an "ok" job
        try {
            job_manager->createStandardJob({t1, t2});
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Should be able to create a standard job with two dependent tasks");
        }


        return 0;
    }
};

TEST_F(JobManagerTest, CreateJobTest) {
    DO_TEST_WITH_FORK(do_JobManagerCreateJobTest_test);
}

void JobManagerTest::do_JobManagerCreateJobTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new JobManagerCreateJobTestWMS(
                                    this, hostname)));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO SUBMIT JOB TEST                                              **/
/**********************************************************************/

class JobManagerSubmitJobTestWMS : public wrench::ExecutionController {

public:
    JobManagerSubmitJobTestWMS(JobManagerTest *test,
                               std::string hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }


private:
    JobManagerTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        std::shared_ptr<wrench::BareMetalComputeService> cs =
                this->getSimulation()->add(new wrench::BareMetalComputeService("Host1", {"Host1"}, "", {}, {}));

        try {
            std::shared_ptr<wrench::CompoundJob> compound_job = nullptr;
            job_manager->submitJob(compound_job, cs, {});
            throw std::runtime_error("Should not be able to submit a job with a nullptr job");
        } catch (std::invalid_argument &e) {
        }

        try {
            auto job = job_manager->createCompoundJob("");
            job_manager->submitJob(job, nullptr, {});
            throw std::runtime_error("Should not be able to submit a job with a nullptr compute service");
        } catch (std::invalid_argument &e) {
        }

        try {
            job_manager->terminateJob((std::shared_ptr<wrench::CompoundJob>) nullptr);
            throw std::runtime_error("Should not be able to terminate a nullptr job");
        } catch (std::invalid_argument &e) {
        }

        return 0;
    }
};

TEST_F(JobManagerTest, SubmitJobTest) {
    DO_TEST_WITH_FORK(do_JobManagerSubmitJobTest_test);
}

void JobManagerTest::do_JobManagerSubmitJobTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new JobManagerSubmitJobTestWMS(
                                    this, hostname)));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO SUBMIT JOB TEST                                              **/
/**********************************************************************/

class JobManagerResubmitJobTestWMS : public wrench::ExecutionController {

public:
    JobManagerResubmitJobTestWMS(JobManagerTest *test,
                                 std::string hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }


private:
    JobManagerTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get the compute_services
        std::shared_ptr<wrench::ComputeService> cs_does_support_standard_jobs;
        std::shared_ptr<wrench::ComputeService> cs_does_not_support_standard_jobs;
        for (auto cs: {this->test->cs1, this->test->cs2}) {
            if (cs->supportsStandardJobs()) {
                cs_does_support_standard_jobs = cs;
            } else {
                cs_does_not_support_standard_jobs = cs;
            }
        }

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->workflow->getTasks());

        // Try to submit a standard job to the wrong service
        try {
            job_manager->submitJob(job, cs_does_not_support_standard_jobs);
            throw std::runtime_error("Should not be able to submit a standard job to a service that does not support them");
        } catch (std::invalid_argument &ignore) {
        }

        // Check task1 states
        std::shared_ptr<wrench::WorkflowTask> task1 = this->test->workflow->getTaskByID("task1");
        std::shared_ptr<wrench::WorkflowTask> task2 = this->test->workflow->getTaskByID("task2");
        if (task1->getState() != wrench::WorkflowTask::State::READY) {
            throw std::runtime_error("Unexpected task1 state (should be READY but is " +
                                     wrench::WorkflowTask::stateToString(task1->getState()));
        }
        if (task2->getState() != wrench::WorkflowTask::State::NOT_READY) {
            throw std::runtime_error("Unexpected task2 state (should be READY but is " +
                                     wrench::WorkflowTask::stateToString(task2->getState()));
        }

        // Resubmit the SAME job to the right service
        try {
            job_manager->submitJob(job, cs_does_support_standard_jobs);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Should be able to subkmit a standard job to a service that supports them");
        }

        // Wait for the workflow execution event
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

TEST_F(JobManagerTest, ResubmitJobTest) {
    DO_TEST_WITH_FORK(do_JobManagerResubmitJobTest_test);
}

void JobManagerTest::do_JobManagerResubmitJobTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a ComputeService that does not support standard jobs

    ASSERT_NO_THROW(cs1 = simulation->add(
                            new wrench::BareMetalComputeService("Host2",
                                                                {std::make_pair("Host2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a ComputeService that does support standard jobs
    ASSERT_NO_THROW(cs2 = simulation->add(
                            new wrench::BareMetalComputeService("Host3",
                                                                {std::make_pair("Host3", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new JobManagerResubmitJobTestWMS(
                                    this, "Host1")));

    // Add tasks to the workflow
    std::shared_ptr<wrench::WorkflowTask> task1 = workflow->addTask("task1", 10.0, 10, 10, 0.0);
    std::shared_ptr<wrench::WorkflowTask> task2 = workflow->addTask("task2", 10.0, 10, 10, 0.0);
    workflow->addControlDependency(task1, task2);

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO TERMINATE JOB TEST                                           **/
/**********************************************************************/

class JobManagerTerminateJobTestWMS : public wrench::ExecutionController {

public:
    JobManagerTerminateJobTestWMS(JobManagerTest *test,
                                  std::string hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }


private:
    JobManagerTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get the compute_services
        auto cs = this->test->cs;

        // Add tasks to the workflow
        std::shared_ptr<wrench::WorkflowTask> t1 = this->test->workflow->addTask("task1", 600, 10, 10, 80);
        std::shared_ptr<wrench::WorkflowTask> t2 = this->test->workflow->addTask("task2", 600, 9, 10, 80);
        std::shared_ptr<wrench::WorkflowTask> t3 = this->test->workflow->addTask("task3", 600, 8, 10, 0);
        std::shared_ptr<wrench::WorkflowTask> t4 = this->test->workflow->addTask("task4", 600, 10, 10, 0);

        /* t1 and t2 can't run at the same time in this example, due to RAM */

        this->test->workflow->addControlDependency(t1, t3);
        this->test->workflow->addControlDependency(t2, t3);
        this->test->workflow->addControlDependency(t3, t4);

        // Setting priorities for coverage
        t1->setPriority(12);
        t2->setPriority(3);
        t3->setPriority(42);

        // Create a standard job
        auto job = job_manager->createStandardJob(this->test->workflow->getTasks());
        job->getMinimumRequiredNumCores();// coverage
        job->getPriority();               // coverage

        // Submit the standard job
        try {
            job_manager->submitJob(job, cs);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "Should be able to submit the job");
        }

        // Sleep for 90 seconds
        wrench::Simulation::sleep(90);

        // Terminate the job
        try {
            job_manager->terminateJob(job);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "Should be able to terminate job");
        }

        // Check task1 states
        bool t1_and_t2_valid_states;

        t1_and_t2_valid_states = ((t1->getState() == wrench::WorkflowTask::State::COMPLETED) and
                                  (t2->getState() == wrench::WorkflowTask::State::READY)) or
                                 ((t1->getState() == wrench::WorkflowTask::State::READY) and
                                  (t2->getState() == wrench::WorkflowTask::State::COMPLETED));

        if (not t1_and_t2_valid_states) {
            throw std::runtime_error("Unexpected task1 and task1 2 states (" +
                                     wrench::WorkflowTask::stateToString(t1->getState()) + " and " +
                                     wrench::WorkflowTask::stateToString(t2->getState()) + ")");
        }
        if (t3->getState() != wrench::WorkflowTask::State::NOT_READY) {
            throw std::runtime_error("Unexpected task1 state (should be NOT_READY but is " +
                                     wrench::WorkflowTask::stateToString(t3->getState()));
        }
        if (t4->getState() != wrench::WorkflowTask::State::NOT_READY) {
            throw std::runtime_error("Unexpected task1 state (should be NOT_READY but is " +
                                     wrench::WorkflowTask::stateToString(t4->getState()));
        }

        return 0;
    }
};

TEST_F(JobManagerTest, TerminateJobTest) {
    DO_TEST_WITH_FORK(do_JobManagerTerminateJobTest_test);
}

void JobManagerTest::do_JobManagerTerminateJobTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a ComputeService that supports standard jobs
    ASSERT_NO_THROW(cs = simulation->add(
                            new wrench::BareMetalComputeService("Host3",
                                                                {std::make_pair("Host3", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                                "/scratch",
                                                                {})));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new JobManagerTerminateJobTestWMS(
                                    this, "Host1")));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
