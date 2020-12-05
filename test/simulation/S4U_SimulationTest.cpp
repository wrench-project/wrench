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

class S4U_SimulationTest : public ::testing::Test {

public:

    void do_basicAPI_Test();

protected:
    S4U_SimulationTest() {

        // Create the simplest workflow
        workflow = new wrench::Workflow();

        // Create a one-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "         <prop id=\"ram\" value=\"1024B\"/> "
                          "         <prop id=\"foo\" value=\"bar\"/> "
                          "       </host> "
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
    wrench::Workflow *workflow;

};

/**********************************************************************/
/**  BASIC API TEST                                                  **/
/**********************************************************************/


class S4U_SimulationAPITestWMS : public wrench::WMS {

public:
    S4U_SimulationAPITestWMS(S4U_SimulationTest *test,
                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    S4U_SimulationTest *test;

    int main() {

        // Flop rate
        double f = wrench::S4U_Simulation::getFlopRate();
        if (std::abs(f - 1.0) > 0.001) {
            throw std::runtime_error("Got wrong flop rate for current host (" + std::to_string(f) + " instead of 1)");
        }
        f = wrench::S4U_Simulation::getHostFlopRate("Host1");
        if (std::abs(f - 1.0) > 0.001) {
            throw std::runtime_error("Got wrong flop rate for Host1 (" + std::to_string(f) + " instead of 1)");
        }

        try {
            wrench::S4U_Simulation::getHostFlopRate("Bogus");
            throw std::runtime_error("Shouldn't not be able to get flop rate for a bogus host");
        } catch (std::invalid_argument &e) {}

        // Num cores
        unsigned int c = wrench::S4U_Simulation::getNumCores();
        if (c != 10) {
            throw std::runtime_error("Got wrong num cores for currenthost (" + std::to_string(c) + " instead of 10)");
        }
        c = wrench::S4U_Simulation::getHostNumCores("Host1");
        if (c != 10) {
            throw std::runtime_error("Got wrong num cores for Host1 (" + std::to_string(c) + " instead of 10)");
        }

        try {
            wrench::S4U_Simulation::getHostNumCores("Bogus");
            throw std::runtime_error("Shouldn't not be able to get num core for a bogus host");
        } catch (std::invalid_argument &e) {}

        // Memory capacity
        double m = wrench::S4U_Simulation::getMemoryCapacity();
        if (std::abs(m - 1024) > 0.001) {
            throw std::runtime_error("Got wrong memory capacity for curent host (" + std::to_string(m) + " instead of 1024)");
        }
        m = wrench::S4U_Simulation::getHostMemoryCapacity("Host1");
        if (std::abs(m - 1024) > 0.001) {
            throw std::runtime_error("Got wrong memory capacity for Host1 (" + std::to_string(m) + " instead of 1024)");
        }

        try {
            wrench::S4U_Simulation::getHostMemoryCapacity("Bogus");
            throw std::runtime_error("Shouldn't not be able to get memory capacity for a bogus host");
        } catch (std::invalid_argument &e) {}

        // on/off
        bool is_on = wrench::S4U_Simulation::isHostOn("Host1");
        if (not is_on) {
            throw std::runtime_error("Host1 should be on!");
        }

        try {
            wrench::S4U_Simulation::isHostOn("Bogus");
            throw std::runtime_error("Shouldn't not be able to get on/off status for a bogus host");
        } catch (std::invalid_argument &e) {}

        // generic property
        std::string p = wrench::S4U_Simulation::getHostProperty("Host1", "foo");
        if (p != "bar") {
            throw std::runtime_error("Got wrong memory property value for current host ('" + p + "' instead of 'bar')");
        }
        try {
            wrench::S4U_Simulation::getHostProperty("Host1", "stuff");
            throw std::runtime_error("Shouldn't not be able to get bogus property from host");
        } catch (std::invalid_argument &e) {}
        try {
            wrench::S4U_Simulation::getHostProperty("Bogus", "stuff");
            throw std::runtime_error("Shouldn't not be able to get property from bogus host");
        } catch (std::invalid_argument &e) {}

        return 0;
    }
};

TEST_F(S4U_SimulationTest, BasicAPI) {
    DO_TEST_WITH_FORK(do_basicAPI_Test);
}

void S4U_SimulationTest::do_basicAPI_Test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("file_registry_test");

    simulation->init(&argc, argv);

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new S4U_SimulationAPITestWMS(
                    this, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(workflow));

    // Running a "run a single task" simulation
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
