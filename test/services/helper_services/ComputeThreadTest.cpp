/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <random>
#include <wrench-dev.h>

#include "helper_services/work_unit_executor/ComputeThread.h"

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(compute_thread_test, "Log category for ComputeThreadTest");


class ComputeThreadTest : public ::testing::Test {

public:

    void do_Working_test();
    void do_KillAfterDeath_test();

protected:
    ComputeThreadTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"12\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"13\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
                          "       <link id=\"23\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"12\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"13\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"23\"/> </route>"
                          "   </zone> "
                          "</platform>";
        // Create a four-host 10-core platform file
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};


/**********************************************************************/
/**  DO WORKING TEST                                                **/
/**********************************************************************/

class ComputeThreadWorkingTestWMS : public wrench::WMS {

public:
    ComputeThreadWorkingTestWMS(ComputeThreadTest *test,
                                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    ComputeThreadTest *test;

    int main() {

        // Create a compute thread on Host 3 that should report to me
        auto thread = std::shared_ptr<wrench::ComputeThread>(new wrench::ComputeThread("Host3", 100, this->mailbox_name));
        thread->simulation = this->simulation;
        thread->start(thread, true, false);

        // Get the message
        auto message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);

        if (not dynamic_cast<wrench::ComputeThreadDoneMessage*>(message.get())) {
            throw std::runtime_error("Didn't receive the expected ComputeThreadDoneMessage message");
        }

        // Done
        return 0;
    }
};

TEST_F(ComputeThreadTest, Working) {
    DO_TEST_WITH_FORK(do_Working_test);
}

void ComputeThreadTest::do_Working_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new ComputeThreadWorkingTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  DO KILL AFTER DEATH TEST                                        **/
/**********************************************************************/

class ComputeThreadKillAfterDeathTestWMS : public wrench::WMS {

public:
    ComputeThreadKillAfterDeathTestWMS(ComputeThreadTest *test,
                                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    ComputeThreadTest *test;

    int main() {

        // Create a compute thread on Host 3 that should report to me
        auto thread = std::shared_ptr<wrench::ComputeThread>(new wrench::ComputeThread("Host3", 100, this->mailbox_name));
        thread->simulation = this->simulation;
        thread->start(thread, true, false);

        // Get the message
        auto message = wrench::S4U_Mailbox::getMessage(this->mailbox_name);

        // Sleep and kill
        wrench::Simulation::sleep(1000);
        thread->kill();

        // Done
        return 0;
    }
};

TEST_F(ComputeThreadTest, KillAfterDeath) {
    DO_TEST_WITH_FORK(do_KillAfterDeath_test);
}

void ComputeThreadTest::do_KillAfterDeath_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new ComputeThreadKillAfterDeathTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}

