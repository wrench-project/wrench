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
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetectorMessage.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"
#include <wrench/services/helper_services/service_termination_detector/ServiceTerminationDetector.h>
#include "../failure_test_util/SleeperVictim.h"
#include "../failure_test_util/ComputerVictim.h"

WRENCH_LOG_CATEGORY(failure_detector_host_failure_test, "Log category for FailureDetectorHostFailuresTest");


class FailureDetectorHostFailuresTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    void do_FailureDetectorForSleeperTest_test();
    void do_FailureDetectorForComputerTest_test();

protected:

    FailureDetectorHostFailuresTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();


        // up from 0 to 100, down from 100 to 200, up from 200 to 300, etc.
        std::string trace_file_content = "PERIODICITY 100\n"
                                         " 0 1\n"
                                         " 100 0";

        std::string trace_file_name = "host.trace";
        std::string trace_file_path = "/tmp/"+trace_file_name;

        FILE *trace_file = fopen(trace_file_path.c_str(), "w");
        fprintf(trace_file, "%s", trace_file_content.c_str());
        fclose(trace_file);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"FailedHost\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"FailedHostTrace\" speed=\"1f\" state_file=\""+trace_file_name+"\"  core=\"1\"/> "
                          "       <host id=\"StableHost\" speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"link1\" bandwidth=\"100kBps\" latency=\"0\"/>"
                          "       <route src=\"FailedHost\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "       <route src=\"FailedHostTrace\" dst=\"StableHost\">"
                          "           <link_ctn id=\"link1\"/>"
                          "       </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};



/**********************************************************************/
/**              FAILURE DETECTOR FOR SLEEPER TEST                   **/
/**********************************************************************/

class FailureDetectorForSleeperTestWMS : public wrench::WMS {

public:
    FailureDetectorForSleeperTestWMS(FailureDetectorHostFailuresTest *test,
                                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    FailureDetectorHostFailuresTest *test;

    int main() override {


        // Starting a victim on the FailedHost, which should fail at time 50
        auto victim1 = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHost", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim1->setSimulation(this->simulation);
        victim1->start(victim1, true, false); // Daemonized, no auto-restart

        // Starting its nemesis!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 50, "FailedHost",
                wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->setSimulation(this->simulation);
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector1 = std::shared_ptr<wrench::ServiceTerminationDetector>(new wrench::ServiceTerminationDetector("StableHost", victim1, this->mailbox_name, true, false));
        failure_detector1->setSimulation(this->simulation);
        failure_detector1->start(failure_detector1, true, false); // Daemonized, no auto-restart

        // Starting a victim on the FailedHostTrace, which should fail at time 100
        auto victim2 = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHostTrace", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim2->setSimulation(this->simulation);
        victim2->start(victim2, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector2 = std::shared_ptr<wrench::ServiceTerminationDetector>(new wrench::ServiceTerminationDetector("StableHost", victim2, this->mailbox_name, true, false));
        failure_detector2->setSimulation(this->simulation);
        failure_detector2->start(failure_detector2, true, false); // Daemonized, no auto-restart


        // Waiting for a message
        std::shared_ptr<wrench::SimulationMessage> message;


        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        auto real_msg = std::dynamic_pointer_cast<wrench::ServiceHasCrashedMessage>(message);
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim1) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        // And again...

        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        real_msg = std::dynamic_pointer_cast<wrench::ServiceHasCrashedMessage>(message);
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim2) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        return 0;
    }
};

TEST_F(FailureDetectorHostFailuresTest, FailureDetectorForSleeperTest) {
    DO_TEST_WITH_FORK(do_FailureDetectorForSleeperTest_test);
}

void FailureDetectorHostFailuresTest::do_FailureDetectorForSleeperTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "StableHost";

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new FailureDetectorForSleeperTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**              FAILURE DETECTOR FOR COMPUTER TEST                  **/
/**********************************************************************/

class FailureDetectorForComputerTestWMS : public wrench::WMS {

public:
    FailureDetectorForComputerTestWMS(FailureDetectorHostFailuresTest *test,
                                     std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    FailureDetectorHostFailuresTest *test;

    int main() override {

        // Starting a victim on the FailedHost, which should fail at time 50
        auto victim1 = std::shared_ptr<wrench::ComputerVictim>(new wrench::ComputerVictim("FailedHost", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim1->setSimulation(this->simulation);
        victim1->start(victim1, true, false); // Daemonized, no auto-restart

        // Starting its nemesis!
        auto murderer = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 50, "FailedHost",
                wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        murderer->setSimulation(this->simulation);
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector1 = std::shared_ptr<wrench::ServiceTerminationDetector>(new wrench::ServiceTerminationDetector("StableHost", victim1, this->mailbox_name, true, false));
        failure_detector1->setSimulation(this->simulation);
        failure_detector1->start(failure_detector1, true, false); // Daemonized, no auto-restart

        // Starting a victim on the FailedHostTrace, which should fail at time 100
        auto victim2 = std::shared_ptr<wrench::ComputerVictim>(new wrench::ComputerVictim("FailedHostTrace", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim2->setSimulation(this->simulation);
        victim2->start(victim2, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector2 = std::shared_ptr<wrench::ServiceTerminationDetector>(new wrench::ServiceTerminationDetector("StableHost", victim2, this->mailbox_name, true, false));
        failure_detector2->setSimulation(this->simulation);
        failure_detector2->start(failure_detector2, true, false); // Daemonized, no auto-restart

        // Waiting for a message
        std::shared_ptr<wrench::SimulationMessage> message;

        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message! " + cause->toString());
        }

        auto real_msg = std::dynamic_pointer_cast<wrench::ServiceHasCrashedMessage>(message);
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim1) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        // And again...
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        real_msg = std::dynamic_pointer_cast<wrench::ServiceHasCrashedMessage>(message);
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim2) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        return 0;
    }
};

TEST_F(FailureDetectorHostFailuresTest, FailureDetectorForComputerTest) {
    DO_TEST_WITH_FORK(do_FailureDetectorForComputerTest_test);
}

void FailureDetectorHostFailuresTest::do_FailureDetectorForComputerTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "StableHost";

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new FailureDetectorForComputerTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
