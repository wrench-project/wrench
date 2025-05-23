/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include <wrench/action/Action.h>
#include <wrench/action/SleepAction.h>
#include <wrench/action/ComputeAction.h>
#include <wrench/action/FileReadAction.h>
#include <wrench/action/FileWriteAction.h>
#include <wrench/action/FileCopyAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes/HostError.h>

#include <utility>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(compute_action_executor_test, "Log category for ComputeActionExecutorTest");

//#define EPSILON (std::numeric_limits<double>::epsilon())
#define EPSILON (0.000001)


class ComputeActionExecutorTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::Simulation> simulation;
    std::shared_ptr<wrench::DataFile> file;
    std::shared_ptr<wrench::StorageService> ss;

    void do_ComputeActionExecutorSuccessTest_test(bool simulation_computation_as_sleep);
    void do_ComputeActionExecutorFailureTest_test();

protected:
    ComputeActionExecutorTest() {
        std::string xml = "<?xml version='1.0'?>"
            "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
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
            "         <prop id=\"ram\" value=\"1024B\"/> "
            "       </host>  "
            "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
            "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
            "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
            "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
            "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
            "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
            "   </zone> "
            "</platform>";
        // Create a four-host 10-core platform file
        FILE* platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;
};


/**********************************************************************/
/**  COMPUTE ACTION EXECUTOR SUCCESS TEST                            **/
/**********************************************************************/


class ComputeActionExecutorTestWMS : public wrench::ExecutionController {
public:
    ComputeActionExecutorTestWMS(ComputeActionExecutorTest* test,
                                 const std::string& hostname,
                                 bool simulation_computation_as_sleep) :
        wrench::ExecutionController(hostname, "test"),
        test(test),
        simulation_computation_as_sleep(
            simulation_computation_as_sleep) {
    }

private:
    ComputeActionExecutorTest* test;
    bool simulation_computation_as_sleep;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");

        // Create an action executor
        std::shared_ptr<wrench::Action> action;
        unsigned long num_cores = 2;
        double ram = 200;

        action = std::dynamic_pointer_cast<wrench::Action>(
            job->addComputeAction("", 20.0, 100.0, 1, 4, wrench::ParallelModel::AMDAHL(1.0)));

        // coverage
        wrench::Action::getActionTypeAsString(action);

        double thread_creation_overhead = 0.1;
        auto action_executor = std::make_shared<wrench::ActionExecutor>(
            "Host2",
            num_cores,
            ram,
            thread_creation_overhead,
            simulation_computation_as_sleep,
            this->commport,
            nullptr,
            action,
            nullptr);

        // Start it
        action_executor->setSimulation(this->getSimulation());
        action_executor->start(action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        }
        catch (wrench::ExecutionException& e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the start-date sensible?
        if (action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getStartDate()));
        }

        // Is the end-date sensible?
        double expected_end_date = 10.2 + thread_creation_overhead;
        if (fabs(action->getEndDate() - expected_end_date) > EPSILON) {
            throw std::runtime_error(
                "Unexpected action end date: " + std::to_string(action->getEndDate()) + " (expected: ~" + std::to_string(expected_end_date) + ")");
        }

        // Is the state sensible?
        if (action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(ComputeActionExecutorTest, Success) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorSuccessTest_test, true);
    DO_TEST_WITH_FORK_ONE_ARG(do_ComputeActionExecutorSuccessTest_test, false);
}

void ComputeActionExecutorTest::do_ComputeActionExecutorSuccessTest_test(bool simulation_computation_as_sleep) {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char** argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    // argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(
        wrench::SimpleStorageService::createSimpleStorageService("Host3", {"/"}, {
                                                                     {
                                                                         wrench::SimpleStorageServiceProperty::BUFFER_SIZE,
                                                                         "10MB"
                                                                     }
                                                                 }));

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    ss->createFile(wrench::FileLocation::LOCATION(ss, file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
        new ComputeActionExecutorTestWMS(this, "Host1", simulation_computation_as_sleep)));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  COMPUTE ACTION EXECUTOR FAILURE TEST                            **/
/**********************************************************************/


class ComputeActionExecutorFailureTestWMS : public wrench::ExecutionController {
public:
    ComputeActionExecutorFailureTestWMS(ComputeActionExecutorTest* test,
                                        std::string hostname) : wrench::ExecutionController(hostname, "test"),
                                                                test(test) {
    }

private:
    ComputeActionExecutorTest* test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");

        // Create an action executor
        std::shared_ptr<wrench::Action> action;
        unsigned long num_cores = 2;
        double ram = 200;

        // Coverage
        try {
            job->addComputeAction("", -1.0, 0, 0, 1, wrench::ParallelModel::AMDAHL(1.0));
            throw std::runtime_error("Shouldn't be able to create a compute action with bogus arguments");
        }
        catch (std::invalid_argument& ignore) {
        }

        // Too much RAM
        action = std::dynamic_pointer_cast<wrench::Action>(
            job->addComputeAction("", 20.0, 300.0, 1, 4, wrench::ParallelModel::AMDAHL(1.0)));

        auto action_executor = std::shared_ptr<wrench::ActionExecutor>(
            new wrench::ActionExecutor("Host2",
                                       num_cores,
                                       ram,
                                       0.1,
                                       false,
                                       this->commport,
                                       nullptr,
                                       action,
                                       nullptr));

        // Start it
        action_executor->setSimulation(this->getSimulation());
        action_executor->start(action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        }
        catch (wrench::ExecutionException& e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        if (action->getState() != wrench::Action::FAILED) {
            throw std::runtime_error("Action should have failed");
        }

        if (not std::dynamic_pointer_cast<wrench::FatalFailure>(action->getFailureCause())) {
            throw std::runtime_error("Action's failure cause should be NotEnoughResources");
        }

        return 0;
    }
};

TEST_F(ComputeActionExecutorTest, Failure) {
    DO_TEST_WITH_FORK(do_ComputeActionExecutorFailureTest_test);
}

void ComputeActionExecutorTest::do_ComputeActionExecutorFailureTest_test() {
    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char** argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
    //    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(
        wrench::SimpleStorageService::createSimpleStorageService("Host3", {"/"}, {
                                                                     {
                                                                         wrench::SimpleStorageServiceProperty::BUFFER_SIZE,
                                                                         "10MB"
                                                                     }
                                                                 }));

    // Create a file
    this->file = wrench::Simulation::addFile("some_file", 1000000);

    ss->createFile(wrench::FileLocation::LOCATION(ss, file));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
        new ComputeActionExecutorFailureTestWMS(this, "Host1")));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
