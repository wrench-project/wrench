/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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

WRENCH_LOG_CATEGORY(bare_metal_compute_service_multi_action_test, "Log category for BareMetalComputeServiceMultiAction test");

class BareMetalComputeServiceMultiActionTest : public ::testing::Test {
public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service3 = nullptr;
    std::shared_ptr<wrench::BareMetalComputeService> compute_service = nullptr;

    void do_DAGOfSleeps_test();
    void do_NonDAG_test();
    void do_RAMConstraintsAndPriorities_test();
    void do_PartialFailure_test();
    void do_PartialTermination_test();

protected:
    BareMetalComputeServiceMultiActionTest() {

        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create two files
        input_file = workflow->addFile("input_file", 10000.0);
        output_file = workflow->addFile("output_file", 20000.0);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2/\"/>"
                          "          </disk>"

                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100GB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "         <prop id=\"ram\" value=\"1000B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"100MBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
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
/**  DAG OF SLEEPS TEST                                              **/
/**********************************************************************/

class DAGOfSleepsTestWMS : public wrench::WMS {
public:
    DAGOfSleepsTestWMS(BareMetalComputeServiceMultiActionTest *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceMultiActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");

        std::uniform_real_distribution<double> dist_sleep(10.0, 100.0);
        //Mersenne Twister: Good quality random number generator
        std::mt19937 rng(42);

        unsigned long first_chain_length = 10;
        unsigned long fork_width = 15;
        unsigned long second_chain_length = 6;

        std::vector<std::shared_ptr<wrench::SleepAction>> first_chain_tasks;
        for (unsigned long i=0; i < first_chain_length; i++) {
            first_chain_tasks.push_back(job->addSleepAction("chain1_sleep_"+std::to_string(i), dist_sleep(rng)));
            if (i > 0) {
                job->addActionDependency(first_chain_tasks[i-1], first_chain_tasks[i]);
            }
        }
        std::vector<std::shared_ptr<wrench::SleepAction>> fork_tasks;
        for (unsigned long i=0; i < fork_width; i++) {
            fork_tasks.push_back(job->addSleepAction("fork_sleep_"+std::to_string(i), dist_sleep(rng)));
            job->addActionDependency(first_chain_tasks[first_chain_tasks.size()-1], fork_tasks[i]);
        }

        std::vector<std::shared_ptr<wrench::SleepAction>> second_chain_tasks;
        for (unsigned long i=0; i < second_chain_length; i++) {
            second_chain_tasks.push_back(job->addSleepAction("chain2_sleep_"+std::to_string(i), dist_sleep(rng)));
            if (i > 0) {
                job->addActionDependency(second_chain_tasks[i-1], second_chain_tasks[i]);
            } else if (i == 0) {
                for (unsigned long j=0; j < fork_width; j++) {
                    job->addActionDependency(fork_tasks[j], second_chain_tasks[0]);
                }
            }
        }

        // Compute the expected makespan
        double expected_makespan = 0.0;
        for (auto const &a : first_chain_tasks) {
            expected_makespan += a->getSleepTime();
        }
        double max_fork = 0.0;
        for (auto const &a : fork_tasks) {
            max_fork = (max_fork < a->getSleepTime() ? a->getSleepTime() : max_fork);
        }
        expected_makespan += max_fork;
        for (auto const &a : second_chain_tasks) {
            expected_makespan += a->getSleepTime();
        }

        // Submit the job
        job_manager->submitJob(job, this->test->compute_service, {});

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check event content
        auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event);
        if (real_event->job != job) {
            throw std::runtime_error("Event's job isn't the right job!");
        }
        if (real_event->compute_service != this->test->compute_service)  {
            throw std::runtime_error("Event's compute service isn't the right compute service!");
        }

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::COMPLETED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Check makespan
        if (std::abs<double>(wrench::Simulation::getCurrentSimulatedDate() - expected_makespan) > 0.0001) {
            throw std::runtime_error("Unexpected makespan");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceMultiActionTest, DAGOfSleeps) {
    DO_TEST_WITH_FORK(do_DAGOfSleeps_test);
}

void BareMetalComputeServiceMultiActionTest::do_DAGOfSleeps_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("multi_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new DAGOfSleepsTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  NON-DAG OF SLEEPS TEST                                          **/
/**********************************************************************/

class NonDAGsTestWMS : public wrench::WMS {
public:
    NonDAGsTestWMS(BareMetalComputeServiceMultiActionTest *test,
                       const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                       const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                       std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceMultiActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");

        auto action1 = job->addSleepAction("chain1_sleep_1", 10.0);
        auto action2 = job->addSleepAction("chain1_sleep_2", 10.0);
        auto action3 = job->addSleepAction("chain1_sleep_3", 10.0);
        auto action4 = job->addSleepAction("chain1_sleep_4", 10.0);

        job->addActionDependency(action1, action2);
        job->addActionDependency(action2, action3);
        job->addActionDependency(action3, action4);

        try {
            job->addActionDependency(action4, action2);
            throw std::runtime_error("Shouldn't be able to create a cycle!");
        } catch (std::invalid_argument &e) {
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceMultiActionTest, NonDAG) {
    DO_TEST_WITH_FORK(do_NonDAG_test);
}

void BareMetalComputeServiceMultiActionTest::do_NonDAG_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("multi_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new NonDAGsTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  RAM CONSTRAINTS AND PRIORITIES TEST                             **/
/**********************************************************************/

class RAMConstraintsAndPrioritiesTestWMS : public wrench::WMS {
public:
    RAMConstraintsAndPrioritiesTestWMS(BareMetalComputeServiceMultiActionTest *test,
                   const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                   const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                   std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceMultiActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");

        auto sleep1 = job->addSleepAction("sleep_1", 10.0);
        auto compute1 = job->addComputeAction("compute_1", 100.0, 200.0, 1, 2, wrench::ParallelModel::AMDAHL(1.0));
        auto compute2 = job->addComputeAction("compute_2", 100.0, 200.0, 1, 2, wrench::ParallelModel::AMDAHL(1.0));
        auto sleep2 = job->addSleepAction("sleep_2", 10.0);
        auto compute3 = job->addComputeAction("compute_3", 100.0, 600.0, 1, 1, wrench::ParallelModel::AMDAHL(1.0));
        compute3->setPriority(10.0);
        auto compute4 = job->addComputeAction("compute_4", 100.0, 600.0, 1, 2, wrench::ParallelModel::AMDAHL(1.0));
        compute4->setPriority(1.0);
        auto sleep3 = job->addSleepAction("sleep_3", 10.0);

        job->addActionDependency(sleep1, compute1);
        job->addActionDependency(sleep1, compute2);
        job->addActionDependency(compute1, sleep2);
        job->addActionDependency(compute2, sleep2);
        job->addActionDependency(sleep2, compute3);
        job->addActionDependency(sleep2, compute4);
        job->addActionDependency(compute3, sleep3);
        job->addActionDependency(compute4, sleep3);

        // Submit the job
        job_manager->submitJob(job, this->test->compute_service, {});

        // Wait for the workflow execution event
        std::shared_ptr<wrench::ExecutionEvent> event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::COMPLETED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        // Check timing
        if (std::abs<double>(compute1->getEndDate() - compute2->getEndDate()) > 0.0001) {
            throw std::runtime_error("Compute1 and Compute2 actions should have completed at the same time");
        }

        if (std::abs<double>(compute3->getEndDate() - compute4->getStartDate()) > 0.0001) {
            throw std::runtime_error("Compute4 action should start only after Compute3 action has completed");
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceMultiActionTest, RAMConstraintsAndPriorities) {
    DO_TEST_WITH_FORK(do_RAMConstraintsAndPriorities_test);
}

void BareMetalComputeServiceMultiActionTest::do_RAMConstraintsAndPriorities_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("multi_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new RAMConstraintsAndPrioritiesTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_THROW(simulation->stageFile(input_file, (std::shared_ptr<wrench::StorageService>) nullptr),
                 std::invalid_argument);
    ASSERT_THROW(simulation->stageFile(nullptr, storage_service1), std::invalid_argument);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  PARTIAL FAILURE TEST                                            **/
/**********************************************************************/

class PartialFailureTestWMS : public wrench::WMS {
public:
    PartialFailureTestWMS(BareMetalComputeServiceMultiActionTest *test,
                                       const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                       const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                       std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceMultiActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");

        auto sleep1 = job->addSleepAction("sleep1", 10.0);
        auto file_read = job->addFileReadAction("file_read", this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service1));
        auto sleep_after_file_read = job->addSleepAction("sleep_after_file_read", 10.0);
        auto compute = job->addComputeAction("compute", 10000.0, 100.0, 1,1, wrench::ParallelModel::AMDAHL(1.0));
        auto sleep_after_compute = job->addSleepAction("sleep_after_compute", 10.0);

        job->addActionDependency(sleep1, file_read);
        job->addActionDependency(sleep1, compute);
        job->addActionDependency(file_read, sleep_after_file_read);
        job->addActionDependency(compute, sleep_after_compute);

        // Submit the job
        job_manager->submitJob(job, this->test->compute_service, {});

        // Sleep 11 seconds
        wrench::Simulation::sleep(11.0);

        // Shut down the compute service
        this->test->compute_service->stop(true);

        // Wait for the workflow execution event
        auto event = this->waitForNextEvent();
        auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
        if (not real_event) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }
        if (not std::dynamic_pointer_cast<wrench::SomeActionsHaveFailed>(real_event->failure_cause)) {
            throw std::runtime_error("Unexpected job-level failure cause");
        }

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        if (sleep1->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected state of the sleep1 action: " + sleep1->getStateAsString());
        }
        if (file_read->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected state of the file_read action: " + file_read->getStateAsString());
        }
        if (sleep_after_file_read->getState() != wrench::Action::State::NOT_READY) {
            throw std::runtime_error("Unexpected state of the sleep_after_file_read action: " + sleep_after_file_read->getStateAsString());
        }
        if (compute->getState() != wrench::Action::State::KILLED) {
            throw std::runtime_error("Unexpected state of the compute action: " + compute->getStateAsString());
        }
        if (sleep_after_compute->getState() != wrench::Action::State::NOT_READY) {
            throw std::runtime_error("Unexpected state of the sleep_after_compute action: " + sleep_after_compute->getStateAsString());
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceMultiActionTest, PartialFailure) {
    DO_TEST_WITH_FORK(do_PartialFailure_test);
}

void BareMetalComputeServiceMultiActionTest::do_PartialFailure_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("multi_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new PartialFailureTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  PARTIAL TERMINATION TEST                                        **/
/**********************************************************************/

class PartialTerminationTestWMS : public wrench::WMS {
public:
    PartialTerminationTestWMS(BareMetalComputeServiceMultiActionTest *test,
                          const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                          const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                          std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    BareMetalComputeServiceMultiActionTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("my_job");

        auto sleep1 = job->addSleepAction("sleep1", 10.0);
        auto file_read = job->addFileReadAction("file_read", this->test->input_file, wrench::FileLocation::LOCATION(this->test->storage_service1));
        auto sleep_after_file_read = job->addSleepAction("sleep_after_file_read", 10.0);
        auto compute = job->addComputeAction("compute", 10000.0, 100.0, 1,1, wrench::ParallelModel::AMDAHL(1.0));
        auto sleep_after_compute = job->addSleepAction("sleep_after_compute", 10.0);

        job->addActionDependency(sleep1, file_read);
        job->addActionDependency(sleep1, compute);
        job->addActionDependency(file_read, sleep_after_file_read);
        job->addActionDependency(compute, sleep_after_compute);

        // Submit the job
        job_manager->submitJob(job, this->test->compute_service, {});

        // Sleep 11 seconds
        wrench::Simulation::sleep(11.0);

        // Terminate the job
        job_manager->terminateJob(job);

        // Check job state
        if (job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
            throw std::runtime_error("Unexpected job state: " + job->getStateAsString());
        }

        if (sleep1->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected state of the sleep1 action: " + sleep1->getStateAsString());
        }
        if (file_read->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected state of the file_read action: " + file_read->getStateAsString());
        }
        if (sleep_after_file_read->getState() != wrench::Action::State::NOT_READY) {
            throw std::runtime_error("Unexpected state of the sleep_after_file_read action: " + sleep_after_file_read->getStateAsString());
        }
        if (compute->getState() != wrench::Action::State::KILLED) {
            throw std::runtime_error("Unexpected state of the compute action: " + compute->getStateAsString());
        }
        if (sleep_after_compute->getState() != wrench::Action::State::NOT_READY) {
            throw std::runtime_error("Unexpected state of the sleep_after_compute action: " + sleep_after_compute->getStateAsString());
        }

        return 0;
    }
};

TEST_F(BareMetalComputeServiceMultiActionTest, PartialTermination) {
    DO_TEST_WITH_FORK(do_PartialTermination_test);
}

void BareMetalComputeServiceMultiActionTest::do_PartialTermination_test() {
    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();

    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("multi_action_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BareMetalComputeService("Host3",
                                                {std::make_pair("Host4",
                                                                std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                wrench::ComputeService::ALL_RAM))},
                                                {"/scratch"},
                                                {})));

    // Create a Storage Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host2", {"/"})));

    // Create a WMS
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::WMS> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
            new PartialTerminationTestWMS(
                    this,
                    {compute_service}, {
                            storage_service1
                    }, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


