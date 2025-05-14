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

#include <wrench/action/SleepAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutorMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/failure_causes/HostError.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"
#include "wrench/failure_causes/OperationTimeout.h"

WRENCH_LOG_CATEGORY(action_timeout_executor_test, "Log category for ActionTimeoutExecutorTest");

//#define EPSILON (std::numeric_limits<double>::epsilon())
#define EPSILON (0.000001)


class ActionTimeoutExecutorTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Simulation> simulation;

    void do_ActionTimeoutExecutorSuccessTest_test();


protected:
    ActionTimeoutExecutorTest() {

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
/**  DO SLEEP ACTION EXECUTOR SUCCESS TEST                           **/
/**********************************************************************/


class ActionTimeoutExecutorSuccessTestWMS : public wrench::ExecutionController {

public:
    ActionTimeoutExecutorSuccessTestWMS(ActionTimeoutExecutorTest *test,
                                      const std::string& hostname) : wrench::ExecutionController(hostname, "test") {
    }


private:
    ActionTimeoutExecutorTest *test;

    int main() override {

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a compound job
        auto job = job_manager->createCompoundJob("");

        // Add a sleep_action
        auto sleep_action = job->addSleepAction("", 10.0);

        // coverage
        wrench::Action::getActionTypeAsString(sleep_action);

        // Create a sleep action executor with a timeout
        auto sleep_action_executor = std::shared_ptr<wrench::ActionExecutor>(
                new wrench::ActionExecutor("Host2", 0, 0.0, 0, false, this->commport, nullptr, sleep_action, nullptr));
        sleep_action_executor->setActionTimeout(5);
        // Start it
        sleep_action_executor->setSimulation(this->getSimulation());
        sleep_action_executor->start(sleep_action_executor, true, false);

        // Wait for a message from it
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting reply from Executor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = std::dynamic_pointer_cast<wrench::ActionExecutorDoneMessage>(message);
        if (!msg) {
            throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        // Is the start-date sensible?
        if (sleep_action->getStartDate() > EPSILON) {
            throw std::runtime_error("Unexpected action start date: " + std::to_string(sleep_action->getStartDate()));
        }

        // Is the end-date sensible?
        if (fabs(sleep_action->getEndDate() - 5.0) > EPSILON) {
            throw std::runtime_error("Unexpected action end date: " + std::to_string(sleep_action->getEndDate()));
        }

        // Is the state sensible?
        if (sleep_action->getState() != wrench::Action::State::FAILED) {
            throw std::runtime_error("Unexpected action state: " + sleep_action->getStateAsString());
        }

        // If the failure cause sensible?
        if (not std::dynamic_pointer_cast<wrench::OperationTimeout>(sleep_action->getFailureCause())) {
            throw std::runtime_error("Unexpected failure cause: " + sleep_action->getFailureCause()->toString());
        }

        return 0;
    }
};

TEST_F(ActionTimeoutExecutorTest, TimeoutTest) {
    DO_TEST_WITH_FORK(do_ActionTimeoutExecutorSuccessTest_test);
}


void ActionTimeoutExecutorTest::do_ActionTimeoutExecutorSuccessTest_test() {

    // Create and initialize a simulation
    simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
                            new ActionTimeoutExecutorSuccessTestWMS(this, "Host1")));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
