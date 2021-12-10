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
#include <wrench/action/CustomAction.h>
#include <wrench/action/FileReadAction.h>
#include <wrench/action/FileWriteAction.h>
#include <wrench/action/FileCopyAction.h>
#include <wrench/services/helper_services/action_execution_service//ActionExecutionService.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceMessage.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes/HostError.h>

#include <utility>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(action_scheduler_test, "Log category for ActionExecutionServiceTest");

#define EPSILON (std::numeric_limits<double>::epsilon())

class ActionExecutionServiceTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Simulation> simulation;
    std::shared_ptr<wrench::DataFile> file;
    std::shared_ptr<wrench::StorageService> ss;

    void do_ActionExecutionServiceOneActionSuccessTest_test();
    void do_ActionExecutionServiceOneActionTerminateTest_test();
    void do_ActionExecutionServiceOneActionCrashRestartTest_test();
    void do_ActionExecutionServiceOneActionCrashNoRestartTest_test();
    void do_ActionExecutionServiceOneActionFailureTest_test();
    void do_ActionExecutionServiceOneActionNotEnoughResourcesTest_test();
    void do_ActionExecutionServiceThreeActionsInSequenceTest_test();

protected:
    ActionExecutionServiceTest() {

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
                          "         <prop id=\"ram\" value=\"1024B\"/> "
                          "       </host>  "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
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
    std::shared_ptr<wrench::Workflow> workflow;

};


/**********************************************************************/
/**  ACTION SCHEDULER ONE ACTION SUCCESS TEST                        **/
/**********************************************************************/


class ActionExecutionServiceOneActionSuccessTestWMS : public wrench::WMS {

public:
    ActionExecutionServiceOneActionSuccessTestWMS(ActionExecutionServiceTest *test,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ActionExecutionServiceTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create an ActionExecutionService
        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
        compute_resources["Host3"] = std::make_tuple(3, 100.0);
        auto action_execution_service = std::shared_ptr<wrench::ActionExecutionService>(
                new wrench::ActionExecutionService("Host2", compute_resources,
                                             {}, {}));

        action_execution_service->setParentService(this->getSharedPtr<Service>());

        // Start it
        action_execution_service->setSimulation(this->simulation);
        action_execution_service->start(action_execution_service, true, false);

        // Create a Compound Job
        auto job = job_manager->createCompoundJob("my_job");

        // Add a sleep action to it
        auto action = job->addSleepAction("my_sleep", 10.0);

        // Submit the action to the action executor
        action_execution_service->submitAction(action);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutionServiceActionDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the start-date sensible?
        if (action->getStartDate() < 0.0 or action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getEndDate()));
        }

        // Is the end-date sensible?
        if (action->getEndDate() + EPSILON < 10.0 or action->getEndDate() > 10.0 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(action->getEndDate()));
        }

        // Is the state sensible?
        if (action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(ActionExecutionServiceTest, OneActionSuccess) {
    DO_TEST_WITH_FORK(do_ActionExecutionServiceOneActionSuccessTest_test);
}

void ActionExecutionServiceTest::do_ActionExecutionServiceOneActionSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host4", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ActionExecutionServiceOneActionSuccessTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}

/**********************************************************************/
/**  ACTION SCHEDULER ONE ACTION TERMINATE TEST                      **/
/**********************************************************************/


class ActionExecutionServiceOneActionTerminateTestWMS : public wrench::WMS {

public:
    ActionExecutionServiceOneActionTerminateTestWMS(ActionExecutionServiceTest *test,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ActionExecutionServiceTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create an ActionExecutionService
        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
        compute_resources["Host3"] = std::make_tuple(3, 100.0);
        auto action_execution_service = std::shared_ptr<wrench::ActionExecutionService>(
                new wrench::ActionExecutionService("Host2", compute_resources,
                                            {}, {}));
        action_execution_service->setParentService(this->getSharedPtr<Service>());

        // Start it
        action_execution_service->setSimulation(this->simulation);
        action_execution_service->start(action_execution_service, true, false);

        // Create a Compound Job
        auto job = job_manager->createCompoundJob("my_job");

        // Add a sleep action to it
        auto action = job->addSleepAction("my_sleep", 10.0);

        // Submit the action to the action executor
        action_execution_service->submitAction(action);

        // Sleep 5s
        wrench::Simulation::sleep(5.0);

        // Invalidly, submit the action to the action executor, for coverage
        try {
            action_execution_service->submitAction(action);
            throw std::runtime_error("Should not be able to submit a non-ready action to the action_execution_service");
        } catch (std::runtime_error &e) {
            // expected
        }

        // Terminate the action
        action_execution_service->terminateAction(action, wrench::ComputeService::TerminationCause::TERMINATION_JOB_KILLED);

        // Is the start-date sensible?
        if (action->getStartDate() < 0.0 or action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getEndDate()));
        }

        // Is the end-date sensible?
        if (action->getEndDate() + EPSILON < 5.0 or action->getEndDate() > 5.0 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(action->getEndDate()));
        }

        // Is the state sensible?
        if (action->getState() != wrench::Action::State::KILLED) {
            throw std::runtime_error("Unexpected action state: " + action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(ActionExecutionServiceTest, OneActionTerminate) {
    DO_TEST_WITH_FORK(do_ActionExecutionServiceOneActionTerminateTest_test);
}

void ActionExecutionServiceTest::do_ActionExecutionServiceOneActionTerminateTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host4", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ActionExecutionServiceOneActionTerminateTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}


/**********************************************************************/
/**  ACTION SCHEDULER ONE ACTION HOST CRASH RESTART TEST             **/
/**********************************************************************/


class ActionExecutionServiceOneActionCrashRestartTestWMS : public wrench::WMS {

public:
    ActionExecutionServiceOneActionCrashRestartTestWMS(ActionExecutionServiceTest *test,
                                             std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ActionExecutionServiceTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create an ActionExecutionService
        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
        compute_resources["Host3"] = std::make_tuple(4, 100.0);
        auto action_execution_service = std::shared_ptr<wrench::ActionExecutionService>(
                new wrench::ActionExecutionService("Host2", compute_resources, nullptr,
                                            {{wrench::ActionExecutionServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "false"}}, {}));

        action_execution_service->setParentService(this->getSharedPtr<wrench::Service>());
        
        // Start it
        action_execution_service->setSimulation(this->simulation);
        action_execution_service->start(action_execution_service, true, false);

        // Create a Compound Job
        auto job = job_manager->createCompoundJob("my_job");

        // Add a sleep action to it
        auto action = job->addComputeAction("my_compute", 100.0, 80.0, 2, 3, wrench::ParallelModel::AMDAHL(1.0));

        // Submit the action to the action executor
        action_execution_service->submitAction(action);

        // Sleep 1s
        wrench::Simulation::sleep(1.0);

        // Kill the Host
        wrench::Simulation::turnOffHost("Host3");

        // Sleep 5s
        wrench::Simulation::sleep(10.0);

        // Make it restart
        wrench::Simulation::turnOnHost("Host3");

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutionServiceActionDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }
        
        // Is the start-date sensible?
        if (action->getStartDate() < 12.0 or action->getStartDate() > 12.0) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getStartDate()));
        }

        // Is the end-date sensible?
        if (action->getEndDate() + EPSILON < 45.333333333333335701810 or action->getEndDate() > 45.33333333333333570181 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(action->getEndDate()));
        }

        // Is the state sensible?
        if (action->getState() != wrench::Action::State::COMPLETED) {
            throw std::runtime_error("Unexpected action state: " + action->getStateAsString());
        }

        // Check out the history
        auto history = action->getExecutionHistory();
        // Most recent
        if (history.top().ram_allocated != 80.0)
            throw std::runtime_error("Action history most recent: unexpected ram " + std::to_string(history.top().ram_allocated));
        if (history.top().num_cores_allocated != 3)
            throw std::runtime_error("Action history most recent: unexpected num cores " + std::to_string(history.top().num_cores_allocated));
        if (history.top().execution_host != "Host3")
            throw std::runtime_error("Action history most recent: unexpected execution host " + history.top().execution_host);
        if (history.top().physical_execution_host != "Host3")
            throw std::runtime_error("Action history most recent: unexpected physical execution host " + history.top().physical_execution_host);
        if (history.top().failure_cause != nullptr)
            throw std::runtime_error("Action history most recent: unexpected failure cause: " + history.top().failure_cause->toString());
        if (history.top().start_date < 12.0 - EPSILON or history.top().start_date > 12.0 + EPSILON)
            throw std::runtime_error("Action history most recent: unexpected start date: " + std::to_string(history.top().start_date));
        if (history.top().end_date + EPSILON < 45.33333333333333570181 or history.top().end_date > 45.33333333333333570181 + EPSILON)
            throw std::runtime_error("Action history most recent: unexpected end date: " + std::to_string(history.top().end_date));
        // Older
        history.pop();
        if (history.top().ram_allocated != 80.0)
            throw std::runtime_error("Action history older: unexpected ram " + std::to_string(history.top().ram_allocated));
        if (history.top().num_cores_allocated != 3)
            throw std::runtime_error("Action history older: unexpected num cores " + std::to_string(history.top().num_cores_allocated));
        if (history.top().execution_host != "Host3")
            throw std::runtime_error("Action history older: unexpected execution host " + history.top().execution_host);
        if (history.top().physical_execution_host != "Host3")
            throw std::runtime_error("Action history older: unexpected physical execution host " + history.top().physical_execution_host);
        if (history.top().failure_cause == nullptr)
            throw std::runtime_error("Action history older: There should be a failure cause: " + history.top().failure_cause->toString());
        if (history.top().start_date > EPSILON)
            throw std::runtime_error("Action history older: unexpected start date: " + std::to_string(history.top().start_date));
        if (history.top().end_date + EPSILON < 1.0 or history.top().end_date > 1.0 + EPSILON)
            throw std::runtime_error("Action history older: unexpected end date: " + std::to_string(history.top().end_date));

        return 0;
    }
};

TEST_F(ActionExecutionServiceTest, OneActionCrashRestart) {
    DO_TEST_WITH_FORK(do_ActionExecutionServiceOneActionCrashRestartTest_test);
}

void ActionExecutionServiceTest::do_ActionExecutionServiceOneActionCrashRestartTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host3", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ActionExecutionServiceOneActionCrashRestartTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}


/**********************************************************************/
/**  ACTION SCHEDULER ONE ACTION HOST CRASH NO RESTART TEST          **/
/**********************************************************************/


class ActionExecutionServiceOneActionCrashNoRestartTestWMS : public wrench::WMS {

public:
    ActionExecutionServiceOneActionCrashNoRestartTestWMS(ActionExecutionServiceTest *test,
                                                std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ActionExecutionServiceTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create an ActionExecutionService
        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
        compute_resources["Host3"] = std::make_tuple(3, 100.0);
        auto action_execution_service = std::shared_ptr<wrench::ActionExecutionService>(
                new wrench::ActionExecutionService("Host2", compute_resources, this->getSharedPtr<Service>(),
                                            {{wrench::ActionExecutionServiceProperty::FAIL_ACTION_AFTER_ACTION_EXECUTOR_CRASH, "true"}}, {}));

        action_execution_service->setParentService(this->getSharedPtr<wrench::Service>());
        
        // Start it
        action_execution_service->setSimulation(this->simulation);
        action_execution_service->start(action_execution_service, true, false);

        // Create a Compound Job
        auto job = job_manager->createCompoundJob("my_job");

        // Add a sleep action to it
        auto action = job->addFileReadAction("my_file_read", this->test->file, wrench::FileLocation::LOCATION(this->test->ss));

        // Submit the action to the action executor
        action_execution_service->submitAction(action);

        // Sleep 1s
        wrench::Simulation::sleep(1.0);

        // Kill the compute host
        wrench::Simulation::turnOffHost("Host3");

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutionServiceActionDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the start-date sensible?
        if (action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getStartDate()));
        }

        // Is the end-date sensible?
        if (action->getEndDate() + EPSILON < 1.0 or action->getEndDate() > 1.0 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(action->getEndDate()));
        }

        // Is the state sensible?
        if (action->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected action state: " + action->getStateAsString());
        }

        return 0;
    }
};

TEST_F(ActionExecutionServiceTest, OneActionCrashNoRestart) {
    DO_TEST_WITH_FORK(do_ActionExecutionServiceOneActionCrashNoRestartTest_test);
}

void ActionExecutionServiceTest::do_ActionExecutionServiceOneActionCrashNoRestartTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host3", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ActionExecutionServiceOneActionCrashNoRestartTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}


/**********************************************************************/
/**  ACTION SCHEDULER ONE ACTION FAILURE TEST                        **/
/**********************************************************************/


class ActionExecutionServiceOneActionFailureTestWMS : public wrench::WMS {

public:
    ActionExecutionServiceOneActionFailureTestWMS(ActionExecutionServiceTest *test,
                                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ActionExecutionServiceTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create an ActionExecutionService
        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
        compute_resources["Host3"] = std::make_tuple(3, 100.0);
        auto action_execution_service = std::shared_ptr<wrench::ActionExecutionService>(
                new wrench::ActionExecutionService("Host2", compute_resources,
                                            {}, {}));
        action_execution_service->setParentService(this->getSharedPtr<wrench::Service>());

        // Start it
        action_execution_service->setSimulation(this->simulation);
        action_execution_service->start(action_execution_service, true, false);

        // Create a Compound Job
        auto job = job_manager->createCompoundJob("my_job");

        // Add a sleep action to it
        auto action = job->addFileReadAction("my_file_read", this->test->file, wrench::FileLocation::LOCATION(this->test->ss));

        // Submit the action to the action executor
        action_execution_service->submitAction(action);

        // Sleep 1s
        wrench::Simulation::sleep(1.0);

        // Kill the storage service
        wrench::Simulation::turnOffHost("Host4");

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutionServiceActionDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the start-date sensible?
        if (action->getStartDate() < 0.0 or action->getStartDate() > 0.0) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(action->getEndDate()));
        }

        // Is the end-date sensible?
        if (action->getEndDate() + EPSILON < action->getStartDate() + 1.0 or action->getEndDate() > action->getStartDate() + 1.0 + EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(action->getEndDate()));
        }

        // Is the state sensible?
        if (action->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected action state: " + action->getStateAsString());
        }

        // Is the failure cause sensible?
        if (not (std::dynamic_pointer_cast<wrench::NetworkError>(action->getFailureCause()))) {
            throw std::runtime_error("Unexpected failure cause");
        }

        return 0;
    }
};

TEST_F(ActionExecutionServiceTest, OneActionFailure) {
    DO_TEST_WITH_FORK(do_ActionExecutionServiceOneActionFailureTest_test);
}

void ActionExecutionServiceTest::do_ActionExecutionServiceOneActionFailureTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host4", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ActionExecutionServiceOneActionFailureTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}


/**********************************************************************/
/**  ACTION SCHEDULER ONE ACTION NOT ENOUGH RESOURCES  TEST          **/
/**********************************************************************/


class ActionExecutionServiceOneActionNotEnoughResourcesTestWMS : public wrench::WMS {

public:
    ActionExecutionServiceOneActionNotEnoughResourcesTestWMS(ActionExecutionServiceTest *test,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ActionExecutionServiceTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create an ActionExecutionService
        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
        compute_resources["Host3"] = std::make_tuple(3, 100.0);
        auto action_execution_service = std::shared_ptr<wrench::ActionExecutionService>(
                new wrench::ActionExecutionService("Host2", compute_resources,
                                            {}, {}));
        action_execution_service->setParentService(this->getSharedPtr<wrench::Service>());

        // Start it
        action_execution_service->setSimulation(this->simulation);
        action_execution_service->start(action_execution_service, true, false);

        // Create a Compound Job
        auto job = job_manager->createCompoundJob("my_job");

        // Add two compute actions with too high requirements to it
        auto action1 = job->addComputeAction("my_compute_1", 100.0, 200.0, 1,1, wrench::ParallelModel::AMDAHL(1.0));
        auto action2 = job->addComputeAction("my_compute_2", 100.0, 80.0, 5,5, wrench::ParallelModel::AMDAHL(1.0));

        // Submit the actions to the action executor
        try {
            action_execution_service->submitAction(action1);
            throw std::runtime_error("Should not have been able to submit action 1");
        } catch (wrench::ExecutionException &e) {
            if (not (std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause()))) {
               throw std::runtime_error("Unexpected failure cause");
            }
        }
        try {
            action_execution_service->submitAction(action2);
            throw std::runtime_error("Should not have been able to submit action 2");
        } catch (wrench::ExecutionException &e) {
            if (not (std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause()))) {
                throw std::runtime_error("Unexpected failure cause");
            }
        }

        return 0;
    }
};

TEST_F(ActionExecutionServiceTest, OneActionNotEnoughResources) {
    DO_TEST_WITH_FORK(do_ActionExecutionServiceOneActionNotEnoughResourcesTest_test);
}

void ActionExecutionServiceTest::do_ActionExecutionServiceOneActionNotEnoughResourcesTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host4", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ActionExecutionServiceOneActionNotEnoughResourcesTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}

/**********************************************************************/
/**  ACTION SCHEDULER THREE ACTIONS IN SEQUENCE TEST                 **/
/**********************************************************************/


class ActionExecutionServiceThreeActionsInSequenceTestWMS : public wrench::WMS {

public:
    ActionExecutionServiceThreeActionsInSequenceTestWMS(ActionExecutionServiceTest *test,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    ActionExecutionServiceTest *test;

    int main() {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create an ActionExecutionService
        std::map<std::string, std::tuple<unsigned long, double>> compute_resources;
        compute_resources["Host3"] = std::make_tuple(3, 100.0);
        auto action_execution_service = std::shared_ptr<wrench::ActionExecutionService>(
                new wrench::ActionExecutionService("Host2", compute_resources,
                                            {}, {}));
        action_execution_service->setParentService(this->getSharedPtr<wrench::Service>());

        // Start it
        action_execution_service->setSimulation(this->simulation);
        action_execution_service->start(action_execution_service, true, false);

        // Create a Compound Job
        auto job = job_manager->createCompoundJob("my_job");

        // Add 2-core compute action action that will start 1st
        auto action1 = job->addComputeAction("compute_action_1", 20.0, 80.0, 2, 2, wrench::ParallelModel::AMDAHL(1.0));

        // Add 2-core compute action action that will start 2nd
        auto action2 = job->addComputeAction("compute_action_2", 90.0, 90.0, 2, 2, wrench::ParallelModel::AMDAHL(1.0));

        // Add 1-core compute action action that will start 3rd
        auto action3 = job->addComputeAction("compute_action_3", 10.0, 20.0, 2, 2, wrench::ParallelModel::AMDAHL(1.0));

        // Submit the actions to the action executor
        action_execution_service->submitAction(action1);
        action_execution_service->submitAction(action2);
        action_execution_service->submitAction(action3);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg1 = std::dynamic_pointer_cast<wrench::ActionExecutionServiceActionDoneMessage>(message);
        if (!msg1) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        if (msg1->action != action1) {
            throw std::runtime_error("Unexpected " + msg1->action->getName() + " action completion");
        }

        // Wait for a message from it
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg2 = std::dynamic_pointer_cast<wrench::ActionExecutionServiceActionDoneMessage>(message);
        if (!msg2) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        if (msg2->action != action2) {
            throw std::runtime_error("Unexpected " + msg2->action->getName() + " action completion");
        }

        // Wait for a message from it
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg3 = std::dynamic_pointer_cast<wrench::ActionExecutionServiceActionDoneMessage>(message);
        if (!msg3) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        if (msg3->action != action3) {
            throw std::runtime_error("Unexpected " + msg3->action->getName() + " action completion");
        }

        // Are the dates sensible?
        if (action1->getStartDate() > EPSILON )
            throw std::runtime_error("Unexpected action1 start date " + std::to_string(action1->getStartDate()));
        if ((action1->getEndDate() < 10.0 - EPSILON) or (action1->getEndDate() > 10.0 + EPSILON))
            throw std::runtime_error("Unexpected action1 end date " + std::to_string(action1->getEndDate()));
        if ((action2->getStartDate() < 10.0 - EPSILON) or (action2->getStartDate() > 10.0 + EPSILON))
            throw std::runtime_error("Unexpected action2 start date " + std::to_string(action2->getStartDate()));
        if ((action2->getEndDate() < 55.0 - EPSILON) or (action2->getEndDate() > 55.0 + EPSILON))
            throw std::runtime_error("Unexpected action2 end date " + std::to_string(action2->getEndDate()));
        if ((action3->getStartDate() < 55.0 - EPSILON) or (action3->getStartDate() > 55.0 + EPSILON))
            throw std::runtime_error("Unexpected action3 start date " + std::to_string(action3->getStartDate()));
        if ((action3->getEndDate() < 60.0 - EPSILON) or (action3->getEndDate() > 60.0 + EPSILON))
            throw std::runtime_error("Unexpected action3 end date " + std::to_string(action3->getEndDate()));


        return 0;
    }
};

TEST_F(ActionExecutionServiceTest, ThreeActionsInSequence) {
    DO_TEST_WITH_FORK(do_ActionExecutionServiceThreeActionsInSequenceTest_test);
}

void ActionExecutionServiceTest::do_ActionExecutionServiceThreeActionsInSequenceTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");
//    argv[2] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    this->workflow = wrench::Workflow::createWorkflow();

    // Create a Storage Service
    this->ss = simulation->add(new wrench::SimpleStorageService("Host4", {"/"}));

    // Create a file
    this->file = this->workflow->addFile("some_file", 1000000.0);

    ss->createFile(file, wrench::FileLocation::LOCATION(ss));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ActionExecutionServiceThreeActionsInSequenceTestWMS(this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(this->workflow));

    ASSERT_NO_THROW(simulation->launch());

    this->workflow->clear();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);

}