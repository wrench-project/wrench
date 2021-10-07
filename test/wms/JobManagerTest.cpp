/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include "helper_services/standard_job_executor/StandardJobExecutorMessage.h"
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/util/TraceFileLoader.h>
#include "wrench/workflow/job/PilotJob.h"

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(job_manager_test, "Log category for JobManagerTest");


class JobManagerTest : public ::testing::Test {


public:
    wrench::Simulation *simulation;

    void do_JobManagerConstructorTest_test();

    void do_JobManagerCreateJobTest_test();

    void do_JobManagerSubmitJobTest_test();

    void do_JobManagerResubmitJobTest_test();

    void do_JobManagerTerminateJobTest_test();


protected:
    JobManagerTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
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

    std::unique_ptr<wrench::Workflow> workflow;
    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";

};


/**********************************************************************/
/**  DO CONSTRUCTOR TEST                                             **/
/**********************************************************************/

class BogusStandardJobScheduler : public wrench::StandardJobScheduler {
    void scheduleTasks(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                       const std::vector<wrench::WorkflowTask *> &tasks);

};

void BogusStandardJobScheduler::scheduleTasks(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                              const std::vector<wrench::WorkflowTask *> &tasks) {}

class BogusPilotJobScheduler : public wrench::PilotJobScheduler {
    void schedulePilotJobs(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services) override;
};

void BogusPilotJobScheduler::schedulePilotJobs(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services) {}

class JobManagerConstructorTestWMS : public wrench::WMS {

public:
    JobManagerConstructorTestWMS(JobManagerTest *test,
                                 std::string hostname) :
            wrench::WMS(std::unique_ptr<BogusStandardJobScheduler>(new BogusStandardJobScheduler()),
                        std::unique_ptr<BogusPilotJobScheduler>(new BogusPilotJobScheduler()),
                        {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

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
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobManagerConstructorTestWMS(
                    this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO CREATE JOB TEST                                             **/
/**********************************************************************/

class JobManagerCreateJobTestWMS : public wrench::WMS {

public:
    JobManagerCreateJobTestWMS(JobManagerTest *test,
                               std::string hostname) :
            wrench::WMS(nullptr, nullptr,
                        {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        try {
            job_manager->createStandardJob(nullptr);
            throw std::runtime_error("Should not be able to create a standard job with a nullptr task in it");
        } catch (std::invalid_argument &e) {
        }


        // Create a job with a nullptr task in it
        try {
            job_manager->createStandardJob((std::vector<wrench::WorkflowTask *>) {nullptr});
            throw std::runtime_error("Should not be able to create a standard job with a nullptr task in it");
        } catch (std::invalid_argument &e) {
        }


        // Create a job  with nothing in it
        try {
            std::vector<wrench::WorkflowTask *> tasks; // empty
            job_manager->createStandardJob(tasks);
            throw std::runtime_error("Should not be able to create a standard job with nothing in it");
        } catch (std::invalid_argument &e) {
        }


        wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("t1", 1.0, 1, 1, 0.0);
        wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("t2", 1.0, 1, 1, 0.0);
        wrench::WorkflowFile *f = this->getWorkflow()->addFile("f", 100);
        t1->addOutputFile(f);
        t2->addInputFile(f);

        // Create a job with a null stuff in pre file copy
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > pre_file_copies;
            pre_file_copies.push_back(std::make_tuple((wrench::WorkflowFile *)nullptr, wrench::FileLocation::SCRATCH, wrench::FileLocation::SCRATCH));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a (nullptr, *, *) pre file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > pre_file_copies;
            pre_file_copies.push_back(std::make_tuple(f, nullptr, wrench::FileLocation::SCRATCH));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, nullptr, *) pre file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > pre_file_copies;
            pre_file_copies.push_back(std::make_tuple(f, wrench::FileLocation::SCRATCH, nullptr));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, *, nullptr) pre file copy");
        } catch (std::invalid_argument &e) {
        }


        // Create a job with a null stuff in post file copy
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > post_file_copies;
            post_file_copies.push_back(std::make_tuple((wrench::WorkflowFile *)nullptr, wrench::FileLocation::SCRATCH, wrench::FileLocation::SCRATCH));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a (nullptr, *, *) post file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > post_file_copies;
            post_file_copies.push_back(std::make_tuple(f, nullptr, wrench::FileLocation::SCRATCH));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, nullptr, *) post file copy");
        } catch (std::invalid_argument &e) {
        }
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > post_file_copies;
            post_file_copies.push_back(std::make_tuple(f, wrench::FileLocation::SCRATCH, nullptr));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a (*, *, nullptr) post file copy");
        } catch (std::invalid_argument &e) {
        }

        // Create a job with SCRATCH as both src and dst
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > pre_file_copies;
            pre_file_copies.push_back(std::make_tuple(f, wrench::FileLocation::SCRATCH, wrench::FileLocation::SCRATCH));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, pre_file_copies, {}, {});
            throw std::runtime_error("Should not be able to create a standard job with a pre file copy that has SCRATCH has both dst and src");
        } catch (std::invalid_argument &e) {
        }
        // Create a job with SCRATCH as both src and dst
        try {
            std::vector<std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation> > > post_file_copies;
            post_file_copies.push_back(std::make_tuple(f, wrench::FileLocation::SCRATCH, wrench::FileLocation::SCRATCH));
            job_manager->createStandardJob({}, (std::map<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>){}, {}, post_file_copies, {});
            throw std::runtime_error("Should not be able to create a standard job with a post file copy that has SCRATCH has both dst and src");
        } catch (std::invalid_argument &e) {
        }
        
        // Create a job with not ok task dependencies
        try {
            job_manager->createStandardJob((std::vector<wrench::WorkflowTask *>) {t2});
            throw std::runtime_error("Should not be able to create a standard job with a not-self-contained task");
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
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobManagerCreateJobTestWMS(
                    this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO SUBMIT JOB TEST                                              **/
/**********************************************************************/

class JobManagerSubmitJobTestWMS : public wrench::WMS {

public:
    JobManagerSubmitJobTestWMS(JobManagerTest *test,
                               std::string hostname) :
            wrench::WMS(nullptr, nullptr,
                        {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        try {
            job_manager->submitJob(nullptr, std::shared_ptr<wrench::ComputeService>((wrench::ComputeService *) (1234), [](void *ptr){}), {});
            throw std::runtime_error("Should not be able to submit a job with a nullptr job");
        } catch (std::invalid_argument &e) {
        }

        try {
            job_manager->submitJob(std::shared_ptr<wrench::WorkflowJob>((wrench::WorkflowJob *) (1234), [](void *ptr){}), nullptr, {});
            throw std::runtime_error("Should not be able to submit a job with a nullptr compute service");
        } catch (std::invalid_argument &e) {
        }

        try {
            job_manager->terminateJob(nullptr);
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
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = wrench::Simulation::getHostnameList()[0];

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobManagerSubmitJobTestWMS(
                    this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));


    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO SUBMIT JOB TEST                                              **/
/**********************************************************************/

class JobManagerResubmitJobTestWMS : public wrench::WMS {

public:
    JobManagerResubmitJobTestWMS(JobManagerTest *test,
                                 std::string hostname,
                                 std::set<std::shared_ptr<wrench::ComputeService>> compute_services) :
            wrench::WMS(nullptr, nullptr,
                        compute_services, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get the compute_services
        std::shared_ptr<wrench::ComputeService> cs_does_support_standard_jobs;
        std::shared_ptr<wrench::ComputeService> cs_does_not_support_standard_jobs;
        for (auto cs : this->getAvailableComputeServices<wrench::ComputeService>()) {
            if (cs->supportsStandardJobs()) {
                cs_does_support_standard_jobs = cs;
            } else {
                cs_does_not_support_standard_jobs = cs;
            }
        }

        // Create a standard job
        auto job = job_manager->createStandardJob(this->getWorkflow()->getTasks());

        // Try to submit a standard job to the wrong service
        try {
            job_manager->submitJob(job, cs_does_not_support_standard_jobs);
            throw std::runtime_error("Should not be able to submit a standard job to a service that does not support them");
        } catch (wrench::WorkflowExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::JobTypeNotSupported>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got expected exception but unexpected failure cause: " +
                                         e.getCause()->toString() + " (expected: JobTypeNotSupported)");
            }
            if (cause->getJob() != job) {
                throw std::runtime_error(
                        "Got expected exxeption and failure cause, but the failure cause does not point to the correct job");
            }
            if (cause->getComputeService() != cs_does_not_support_standard_jobs) {
                throw std::runtime_error(
                        "Got expected exxeption and failure cause, but the failure cause does not point to the correct compute service");
            }
        }

        // Check task states
        wrench::WorkflowTask *task1 = this->getWorkflow()->getTaskByID("task1");
        wrench::WorkflowTask *task2 = this->getWorkflow()->getTaskByID("task2");
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
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Should be able to subkmit a standard job to a service that supports them");
        }

        // Wait for the workflow execution event
        std::shared_ptr<wrench::WorkflowExecutionEvent> event;
        try {
            event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
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
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a ComputeService that does not support standard jobs
    std::shared_ptr<wrench::ComputeService> cs1, cs2;

    ASSERT_NO_THROW(cs1 = simulation->add(
            new wrench::BareMetalComputeService("Host2",
                                                {std::make_pair("Host2", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch",
                                                {{wrench::ComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "false"}})));

    // Create a ComputeService that does support standard jobs
    ASSERT_NO_THROW(cs2 = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host3", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch",
                                                {{wrench::ComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobManagerResubmitJobTestWMS(
                    this, "Host1", {cs1, cs2})));

    // Add tasks to the workflow
    wrench::WorkflowTask *task1 = workflow->addTask("task1", 10.0, 10, 10, 0.0);
    wrench::WorkflowTask *task2 = workflow->addTask("task2", 10.0, 10, 10, 0.0);
    workflow->addControlDependency(task1, task2);

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));


    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



/**********************************************************************/
/**  DO TERMINATE JOB TEST                                           **/
/**********************************************************************/

class JobManagerTerminateJobTestWMS : public wrench::WMS {

public:
    JobManagerTerminateJobTestWMS(JobManagerTest *test,
                                  std::string hostname,
                                  std::set<std::shared_ptr<wrench::ComputeService>> compute_services) :
            wrench::WMS(nullptr, nullptr,
                        compute_services, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    JobManagerTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Get the compute_services
        auto cs = *(this->getAvailableComputeServices<wrench::ComputeService>().begin());

        // Add tasks to the workflow
        wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("task1", 600, 10, 10, 80);
        wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("task2", 600, 9, 10, 80);
        wrench::WorkflowTask *t3 = this->getWorkflow()->addTask("task3", 600, 8, 10, 0);
        wrench::WorkflowTask *t4 = this->getWorkflow()->addTask("task4", 600, 10, 10, 0);

        /* t1 and t2 can't run at the same time in this example, due to RAM */

        this->getWorkflow()->addControlDependency(t1, t3);
        this->getWorkflow()->addControlDependency(t2, t3);
        this->getWorkflow()->addControlDependency(t3, t4);

        // Setting priorities for coverage
        t1->setPriority(12);
        t2->setPriority(3);
        t3->setPriority(42);

        // Create a standard job
        auto job = job_manager->createStandardJob(this->getWorkflow()->getTasks());
        job->getMinimumRequiredNumCores(); // coverage
        job->getPriority(); // coverage

        // Submit the standard job
        try {
            job_manager->submitJob(job, cs);
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Should be able to submit the job");
        }

        // Sleep for 90 seconds
        this->simulation->sleep(90);

        // Terminate the job
        try {
            job_manager->terminateJob(job);
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Should be able to terminate job");
        }

        // Check task states
        bool t1_and_t2_valid_states;

        t1_and_t2_valid_states = ((t1->getState() == wrench::WorkflowTask::State::COMPLETED) and
                                  (t2->getState() == wrench::WorkflowTask::State::READY)) or
                                 ((t1->getState() == wrench::WorkflowTask::State::READY) and
                                  (t2->getState() == wrench::WorkflowTask::State::COMPLETED));

        if (not t1_and_t2_valid_states) {
            throw std::runtime_error("Unexpected task1 and task 2 states (" +
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
    simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a ComputeService that supports standard jobs
    std::shared_ptr<wrench::ComputeService> cs = nullptr;
    ASSERT_NO_THROW(cs = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host3", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                "/scratch",
                                                {{wrench::ComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new JobManagerTerminateJobTestWMS(
                    this, "Host1", {cs})));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}