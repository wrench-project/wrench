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

#include "../../../src/wrench/services/compute/work_unit_executor/ComputeThread.h"

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(compute_thread_link_failure_test, "Log category for link failure with ComputeThread");


class ComputeThreadLinkFailureTest : public ::testing::Test {

public:

    void do_LinkFailure_test();


protected:
    ComputeThreadLinkFailureTest() {

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
/**  DO LINK FAILURE TEST                                            **/
/**********************************************************************/

class ComputeThreadLinkFailureTestWMS : public wrench::WMS {

public:
    ComputeThreadLinkFailureTestWMS(ComputeThreadLinkFailureTest *test,
                                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    ComputeThreadLinkFailureTest *test;

    int main() {

        // Create a random service on Host 2
        auto service_on_host2 = std::shared_ptr<wrench::FileRegistryService>(new wrench::FileRegistryService("Host2"));
        service_on_host2->simulation = this->simulation;
        service_on_host2->start(service_on_host2, true, false);

        // Create a compute thread on Host 3 that should report to the service on Host 2
        auto thread = std::shared_ptr<wrench::ComputeThread>(new wrench::ComputeThread("Host3", 100, service_on_host2->mailbox_name));
        thread->simulation = this->simulation;
        thread->start(thread, true, false);

        // Sleep 10 seconds and turn off link 23
        wrench::Simulation::sleep(10);

        try {
            wrench::S4U_Simulation::turnOffLink("bogus");
            throw std::runtime_error("Should not be able to call turnOffLink on a boguys link");
        } catch (std::invalid_argument &e) {

        }
        wrench::Simulation::turnOffLink("23");

        // Coverage
        wrench::S4U_Simulation::isLinkOn("23");
        try {
            wrench::S4U_Simulation::isLinkOn("bogus");
            throw std::runtime_error("Should not be able to call isLinkOn on a boguys link");
        } catch (std::invalid_argument &e) {

        }

        // Sleep 100 seconds
        wrench::Simulation::sleep(1000);


        // Coverage
        try {
            wrench::S4U_Simulation::turnOnLink("bogus");
            throw std::runtime_error("Should not be able to call turnOnLink on a boguys link");
        } catch (std::invalid_argument &e) {

        }

        // Done
        return 0;
    }
};

TEST_F(ComputeThreadLinkFailureTest, SimpleLinkFailure) {
    DO_TEST_WITH_FORK(do_LinkFailure_test);
}

void ComputeThreadLinkFailureTest::do_LinkFailure_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new ComputeThreadLinkFailureTestWMS(this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


