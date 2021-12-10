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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class S4U_DaemonTest : public ::testing::Test {

public:

    void do_basic_Test();
    void do_noCleanup_Test();

protected:

    ~S4U_DaemonTest() {
        workflow->clear();
    }

    S4U_DaemonTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"1000GBps\" latency=\"1000us\"/>"
                          "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"1500us\"/>"
                          "       <link id=\"4\" bandwidth=\"3000GBps\" latency=\"0us\"/>"
                          "       <link id=\"5\" bandwidth=\"8000GBps\" latency=\"0us\"/>"
                          "       <link id=\"6\" bandwidth=\"2900GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"3\""
                          "/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"4\""
                          "/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"5\""
                          "/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"6\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";

        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;

};

class Sleep100Daemon : public wrench::S4U_Daemon {

public:
    Sleep100Daemon(std::string hostname) :
            S4U_Daemon(hostname, "sleep100daemon", "sleep100daemon") {}

    int main() override {
        simgrid::s4u::this_actor::execute(100);
        return 0;
    }

};

/**********************************************************************/
/**  BASIC TEST                                                      **/
/**********************************************************************/


class S4U_DaemonTestWMS : public wrench::WMS {

public:
    S4U_DaemonTestWMS(S4U_DaemonTest *test,
                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    S4U_DaemonTest *test;

    int main() {

        std::shared_ptr<Sleep100Daemon> daemon =
                std::shared_ptr<Sleep100Daemon>(new Sleep100Daemon("Host1"));

        // Start the daemon without a life saver
        try {
            daemon->startDaemon(false, false);
            throw std::runtime_error("Should not be able to start a lifesaver-less daemon");
        } catch (std::runtime_error &e) {
        }

        // Create the life saver
        daemon->createLifeSaver(std::shared_ptr<Sleep100Daemon>(daemon));

        // Create the life saver again (which is bogus)
        try {
            daemon->createLifeSaver(std::shared_ptr<Sleep100Daemon>(daemon));
            throw std::runtime_error("Should not be able to create a second life saver");
        } catch (std::runtime_error &e) {
        }

        // Start a daemon without a simulation pointer
        try {
            daemon->startDaemon(false, false);
            throw std::runtime_error("Should not be able to start a simulation-less daemon");
        } catch (std::runtime_error &e) {
        }


        // Start the daemon for real
        daemon->setSimulation(this->simulation);
        daemon->startDaemon(false, false);

        daemon->isDaemonized(); // coverage

        // sleep 10 seconds
        wrench::Simulation::sleep(10);

        // suspend the daemon
        daemon->suspendActor();

        // sleep another 10 seconds
        wrench::Simulation::sleep(20);

        // resume the daemon
        daemon->resumeActor();

        // Join and check that we get to the right time
        daemon->join();

        double now = wrench::Simulation::getCurrentSimulatedDate();
        if (std::abs(now - 120) > 1) {
            throw std::runtime_error("Joining at time " + std::to_string(now) + " instead of expected 120");
        }

        return 0;
    }
};

TEST_F(S4U_DaemonTest, Basic) {
    DO_TEST_WITH_FORK(do_basic_Test);
}

void S4U_DaemonTest::do_basic_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new S4U_DaemonTestWMS(
                    this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());



    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  NO CLEANUP TEST                                                 **/
/**********************************************************************/



class S4U_DaemonNoCleanupTestWMS : public wrench::WMS {

public:
    S4U_DaemonNoCleanupTestWMS(S4U_DaemonTest *test,
                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    S4U_DaemonTest *test;

    int main() {

        std::shared_ptr<Sleep100Daemon> daemon =
                std::shared_ptr<Sleep100Daemon>(new Sleep100Daemon("Host2"));

        daemon->createLifeSaver(std::shared_ptr<Sleep100Daemon>(daemon));
        daemon->setSimulation(this->simulation);
        daemon->startDaemon(false, false);

        // sleep 10 seconds
        wrench::Simulation::sleep(10);

        // Turn off Host2
        wrench::Simulation::turnOffHost("Host2");

        return 0;
    }
};

TEST_F(S4U_DaemonTest, NoCleanup) {
    DO_TEST_WITH_FORK_EXPECT_FATAL_FAILURE(do_noCleanup_Test, true);
}

void S4U_DaemonTest::do_noCleanup_Test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new S4U_DaemonNoCleanupTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task1" simulation
    ASSERT_NO_THROW(simulation->launch());



    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

