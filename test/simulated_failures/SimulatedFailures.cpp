/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <wrench/services/helpers/ServiceFailureDetectorMessage.h>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"
#include "HostSwitch.h"
#include "wrench/services/helpers/ServiceFailureDetector.h"
#include "SleeperVictim.h"
#include "ComputerVictim.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simulated_failures_test, "Log category for SimulatedFailuresTests");


class SimulatedFailuresTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    void do_StartServiceOnDownHostTest_test();
    void do_FailureDetectorForSleeperTest_test();
    void do_FailureDetectorForComputerTest_test();
    void do_ServiceRestartTest_test();

protected:

    SimulatedFailuresTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

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
/**          START SERVICE ON DOWN HOST TEST                         **/
/**********************************************************************/

class StartServiceOnDownHostTestWMS : public wrench::WMS {

public:
    StartServiceOnDownHostTestWMS(SimulatedFailuresTest *test,
                          std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    SimulatedFailuresTest *test;

    int main() override {

        // Turn off FailedHost
        wrench::Simulation::sleep(10);
        simgrid::s4u::Host::by_name("FailedHost")->turn_off();

        // Starting a sleeper (that will reply with a bogus TTL Expiration message)
        auto sleeper = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHost", 100, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        sleeper->simulation = this->simulation;
        try {
            sleeper->start(sleeper, true, true); // Daemonized, auto-restart!!
        } catch (std::shared_ptr<wrench::HostError> &e) {
            // Expected exception
            return 0;
        }
        throw std::runtime_error("Should have gotten a HostFailure exception");

        return 0;
    }
};

TEST_F(SimulatedFailuresTest, StartServiceOnDownHostTest) {
    DO_TEST_WITH_FORK(do_StartServiceOnDownHostTest_test);
}

void SimulatedFailuresTest::do_StartServiceOnDownHostTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "StableHost";

    // Create a WMS
    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new StartServiceOnDownHostTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}



/**********************************************************************/
/**              FAILURE DETECTOR FOR SLEEPER TEST                   **/
/**********************************************************************/

class FailureDetectorForSleeperTestWMS : public wrench::WMS {

public:
    FailureDetectorForSleeperTestWMS(SimulatedFailuresTest *test,
                                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    SimulatedFailuresTest *test;

    int main() override {


        // Starting a victim on the FailedHost, which should fail at time 50
        auto victim1 = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHost", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim1->simulation = this->simulation;
        victim1->start(victim1, true, false); // Daemonized, no auto-restart

        // Starting its nemesis!
        auto murderer = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 50, "FailedHost", wrench::HostSwitch::Action::TURN_OFF));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector1 = std::shared_ptr<wrench::ServiceFailureDetector>(new wrench::ServiceFailureDetector("StableHost", victim1, this->mailbox_name));
        failure_detector1->simulation = this->simulation;
        failure_detector1->start(failure_detector1, true, false); // Daemonized, no auto-restart

        // Starting a victim on the FailedHostTrace, which should fail at time 100
        auto victim2 = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHostTrace", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim2->simulation = this->simulation;
        victim2->start(victim2, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector2 = std::shared_ptr<wrench::ServiceFailureDetector>(new wrench::ServiceFailureDetector("StableHost", victim2, this->mailbox_name));
        failure_detector2->simulation = this->simulation;
        failure_detector2->start(failure_detector2, true, false); // Daemonized, no auto-restart


        // Waiting for a message
        std::unique_ptr<wrench::SimulationMessage> message;

        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        WRENCH_INFO("GOT A MESSAGE!");
        auto real_msg = dynamic_cast<wrench::ServiceHasCrashedMessage *>(message.get());
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim1.get()) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        // And again...
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }
        WRENCH_INFO("FOT ANOTHER!");

        real_msg = dynamic_cast<wrench::ServiceHasCrashedMessage *>(message.get());
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim2.get()) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        WRENCH_INFO("WOOHOO!");

        return 0;
    }
};

TEST_F(SimulatedFailuresTest, FailureDetectorForSleeperTest) {
    DO_TEST_WITH_FORK(do_FailureDetectorForSleeperTest_test);
}

void SimulatedFailuresTest::do_FailureDetectorForSleeperTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "StableHost";

    // Create a WMS
    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new FailureDetectorForSleeperTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**              FAILURE DETECTOR FOR COMPUTER TEST                  **/
/**********************************************************************/

class FailureDetectorForComputerTestWMS : public wrench::WMS {

public:
    FailureDetectorForComputerTestWMS(SimulatedFailuresTest *test,
                                     std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    SimulatedFailuresTest *test;

    int main() override {

        // Starting a victim on the FailedHost, which should fail at time 50
        auto victim1 = std::shared_ptr<wrench::ComputerVictim>(new wrench::ComputerVictim("FailedHost", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim1->simulation = this->simulation;
        victim1->start(victim1, true, false); // Daemonized, no auto-restart

        // Starting its nemesis!
        auto murderer = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 50, "FailedHost", wrench::HostSwitch::Action::TURN_OFF));
        murderer->simulation = this->simulation;
        murderer->start(murderer, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector1 = std::shared_ptr<wrench::ServiceFailureDetector>(new wrench::ServiceFailureDetector("StableHost", victim1, this->mailbox_name));
        failure_detector1->simulation = this->simulation;
        failure_detector1->start(failure_detector1, true, false); // Daemonized, no auto-restart

        // Starting a victim on the FailedHostTrace, which should fail at time 100
        auto victim2 = std::shared_ptr<wrench::ComputerVictim>(new wrench::ComputerVictim("FailedHostTrace", 200, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        victim2->simulation = this->simulation;
        victim2->start(victim2, true, false); // Daemonized, no auto-restart

        // Starting the failure detector!
        auto failure_detector2 = std::shared_ptr<wrench::ServiceFailureDetector>(new wrench::ServiceFailureDetector("StableHost", victim2, this->mailbox_name));
        failure_detector2->simulation = this->simulation;
        failure_detector2->start(failure_detector2, true, false); // Daemonized, no auto-restart


        // Waiting for a message
        std::unique_ptr<wrench::SimulationMessage> message;

        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        WRENCH_INFO("GOT A MESSAGE!");
        auto real_msg = dynamic_cast<wrench::ServiceHasCrashedMessage *>(message.get());
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim1.get()) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        // And again...
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }
        WRENCH_INFO("FOT ANOTHER!");

        real_msg = dynamic_cast<wrench::ServiceHasCrashedMessage *>(message.get());
        if (not real_msg) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        } else {
            if (real_msg->service != victim2.get()) {
                throw std::runtime_error("Got a failure notification, but not for the right service!");
            }
        }

        WRENCH_INFO("WOOHOO!");

        return 0;
    }
};

#if ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 22)) || ((SIMGRID_VERSION_MAJOR == 3) && (SIMGRID_VERSION_MINOR == 21) && (SIMGRID_VERSION_PATCH > 0))
TEST_F(SimulatedFailuresTest, FailureDetectorForComputerTest) {
#else
TEST_F(SimulatedFailuresTest, DISABLED_FailureDetectorForComputerTest) {
#endif
    DO_TEST_WITH_FORK(do_FailureDetectorForComputerTest_test);
}

void SimulatedFailuresTest::do_FailureDetectorForComputerTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "StableHost";

    // Create a WMS
    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new FailureDetectorForComputerTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**                    SERVICE RESTART TEST                          **/
/**********************************************************************/

class ServiceRestartTestWMS : public wrench::WMS {

public:
    ServiceRestartTestWMS(SimulatedFailuresTest *test,
                              std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    SimulatedFailuresTest *test;

    int main() override {

        // Starting a sleeper (that will reply with a bogus TTL Expiration message)
        auto sleeper = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHost", 100, new wrench::ServiceTTLExpiredMessage(1), this->mailbox_name));
        sleeper->simulation = this->simulation;
        sleeper->start(sleeper, true, true); // Daemonized, auto-restart!!

        // Starting a host-switcher-offer
        auto death = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 10, "FailedHost", wrench::HostSwitch::Action::TURN_OFF));
        death->simulation = this->simulation;
        death->start(death, true, false); // Daemonized, no auto-restart

        // Starting a host-switcher-oner
        auto life = std::shared_ptr<wrench::HostSwitch>(new wrench::HostSwitch("StableHost", 30, "FailedHost", wrench::HostSwitch::Action::TURN_ON));
        life->simulation = this->simulation;
        life->start(life, true, false); // Daemonized, no auto-restart

        // Waiting for a message
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
            message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        if (dynamic_cast<wrench::ServiceTTLExpiredMessage *>(message.get())) {
            // All good
        } else {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        }

	return 0;
    }
};

TEST_F(SimulatedFailuresTest, ServiceRestartTest) {
    DO_TEST_WITH_FORK(do_ServiceRestartTest_test);
}

void SimulatedFailuresTest::do_ServiceRestartTest_test() {

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("failure_test");


    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "StableHost";

    // Create a WMS
    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(
            new ServiceRestartTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    free(argv[0]);
    free(argv);
}


