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
#include <algorithm>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceRandomRepeatSwitcher.h"
#include "../failure_test_util/ResourceSwitcher.h"
#include <wms/WMSMessage.h>
#include <wrench/execution_controller/ExecutionControllerMessage.h>

WRENCH_LOG_CATEGORY(alarm_link_failures_test, "Log category for AlarmLinkFailuresTest");


class AlarmLinkFailuresTest : public ::testing::Test {

public:

    void do_AlarmLinkFailure_Test();

protected:
    AlarmLinkFailuresTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"link1\" bandwidth=\"1Bps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"link1\""
                          "       /> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**  LINK FAILURE  TEST                                              **/
/**********************************************************************/

class AlarmLinkFailuresTestWMS : public wrench::WMS {

public:
    AlarmLinkFailuresTestWMS(AlarmLinkFailuresTest *test,
                            std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    AlarmLinkFailuresTest *test;

    int main() {

        // Create an Alarm service that will go of in 10 seconds
        std::string mailbox = this->getWorkflow()->getCallbackMailbox();
        wrench::Alarm::createAndStartAlarm(this->simulation, 10,"Host2", mailbox,
                                           new wrench::ExecutionControllerAlarmTimerMessage("hello", 10000), "wms_timer");

        // Start the link killer that will turn off link1 in 20 seconds
        auto switcher = std::shared_ptr<wrench::ResourceSwitcher>(
                new wrench::ResourceSwitcher("Host1", 20, "link1",
                                             wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::LINK));
        switcher->setSimulation(this->simulation);
        switcher->start(switcher, true, false); // Daemonized, no auto-restart

        // Wait for the message
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(mailbox);
            throw std::runtime_error("Should never have gotten the alarm's message");
        } catch (std::shared_ptr<wrench::NetworkError> &error) {
            error->toString();
            error->getMailbox();
        }

        return 0;
    }
};

TEST_F(AlarmLinkFailuresTest, SimpleRandomTest) {
    DO_TEST_WITH_FORK(do_AlarmLinkFailure_Test);
}

void AlarmLinkFailuresTest::do_AlarmLinkFailure_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new AlarmLinkFailuresTestWMS(
                    this, "Host1")));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());



    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
