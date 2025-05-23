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

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"
#include "../failure_test_util/ResourceSwitcher.h"
#include "../failure_test_util/SleeperVictim.h"
#include <wrench/failure_causes/HostError.h>

WRENCH_LOG_CATEGORY(service_start_restart_test, "Log category for ServiceStartRestartTest");


class ServiceReStartHostFailuresTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    void do_StartServiceOnDownHostTest_test();
    void do_ServiceRestartTest_test();

protected:
    ~ServiceReStartHostFailuresTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    ServiceReStartHostFailuresTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();


        // up from 0 to 100, down from 100 to 200, up from 200 to 300, etc.
        std::string trace_file_content = "PERIODICITY 100\n"
                                         " 0 1\n"
                                         " 100 0";

        std::string trace_file_name = UNIQUE_PREFIX + "host.trace";
        std::string trace_file_path = "/tmp/" + trace_file_name;

        FILE *trace_file = fopen(trace_file_path.c_str(), "w");
        fprintf(trace_file, "%s", trace_file_content.c_str());
        fclose(trace_file);

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"FailedHost\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"FailedHostTrace\" speed=\"1f\" state_file=\"" +
                          trace_file_name + "\"  core=\"1\"/> "
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

class StartServiceOnDownHostTestWMS : public wrench::ExecutionController {

public:
    StartServiceOnDownHostTestWMS(ServiceReStartHostFailuresTest *test,
                                  std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    ServiceReStartHostFailuresTest *test;

    int main() override {

        // Turn off FailedHost
        wrench::Simulation::sleep(10);
        wrench::Simulation::turnOffHost("FailedHost");

        // Starting a sleeper (that will reply with a bogus TTL Expiration message)
        auto sleeper = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHost", 100, new wrench::ServiceDaemonStoppedMessage(1), this->commport));
        sleeper->setSimulation(this->getSimulation());
        try {
            sleeper->start(sleeper, true, true);// Daemonized, auto-restart!!
        } catch (wrench::ExecutionException &e) {
            // Expected exception
            e.getCause()->toString();// for coverage
            return 0;
        }
        throw std::runtime_error("Should have gotten a HostFailure exception");

        return 0;
    }
};

TEST_F(ServiceReStartHostFailuresTest, StartServiceOnDownHostTest) {
    DO_TEST_WITH_FORK(do_StartServiceOnDownHostTest_test);
}

void ServiceReStartHostFailuresTest::do_StartServiceOnDownHostTest_test() {

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
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new StartServiceOnDownHostTestWMS(this, hostname)));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**                    SERVICE RESTART TEST                          **/
/**********************************************************************/

class ServiceRestartTestWMS : public wrench::ExecutionController {

public:
    ServiceRestartTestWMS(ServiceReStartHostFailuresTest *test,
                          std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    ServiceReStartHostFailuresTest *test;

    int main() override {

        // Starting a sleeper (that will reply with some message)
        auto sleeper = std::shared_ptr<wrench::SleeperVictim>(new wrench::SleeperVictim("FailedHost", 100, new wrench::ServiceDaemonStoppedMessage(1), this->commport));
        sleeper->setSimulation(this->getSimulation());
        sleeper->start(sleeper, true, true);// Daemonized, auto-restart!!

        // Starting a host-switcher-offer
        auto death = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 10, "FailedHost",
                                                                                            wrench::ResourceSwitcher::Action::TURN_OFF, wrench::ResourceSwitcher::ResourceType::HOST));
        death->setSimulation(this->getSimulation());
        death->start(death, true, false);// Daemonized, no auto-restart

        // Starting a host-switcher-oner
        auto life = std::shared_ptr<wrench::ResourceSwitcher>(new wrench::ResourceSwitcher("StableHost", 30, "FailedHost",
                                                                                           wrench::ResourceSwitcher::Action::TURN_ON, wrench::ResourceSwitcher::ResourceType::HOST));
        life->setSimulation(this->getSimulation());
        life->start(life, true, false);// Daemonized, no auto-restart

        // Waiting for a message
        std::shared_ptr<wrench::SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (wrench::ExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::NetworkError>(e.getCause());
            throw std::runtime_error("Network error while getting a message!" + cause->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::ServiceDaemonStoppedMessage>(message)) {
            throw std::runtime_error("Unexpected " + message->getName() + " message");
        }

        return 0;
    }
};

TEST_F(ServiceReStartHostFailuresTest, ServiceRestartTest) {
    DO_TEST_WITH_FORK(do_ServiceRestartTest_test);
}

void ServiceReStartHostFailuresTest::do_ServiceRestartTest_test() {

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
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(
                            new ServiceRestartTestWMS(this, hostname)));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
