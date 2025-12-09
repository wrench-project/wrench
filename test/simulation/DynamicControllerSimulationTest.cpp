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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(dynamic_controller_test, "Log category for DynamicControllerSimulationTest");


class DynamicControllerSimulationTest : public ::testing::Test {

public:
    void do_dynamicControllerTest_test();

protected:
    ~DynamicControllerSimulationTest() override {
    }

    DynamicControllerSimulationTest() {
        // Create a platform file
        std::string xml = R"(<?xml version='1.0'?>
                          <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
                          <platform version="4.1"> 
                             <zone id="AS0" routing="Full">
                                 <host id="Host1" speed="1f" core="4">
                                 </host>
                             </zone>
                          </platform>)";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**            DYNAMIC CONTROLLER CREATION TEST                      **/
/**********************************************************************/


class TestDynamicChildController : public wrench::ExecutionController {
public:
    TestDynamicChildController(const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
    }

private:
    int main() override {
        wrench::Simulation::sleep(10);
        return 0;
    }
};


class TestDynamicMainController : public wrench::ExecutionController {
public:
    TestDynamicMainController(const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
    }

private:
    int main() override {
        wrench::Simulation::sleep(10);
        auto new_controller = this->simulation_->startNewExecutionController(new TestDynamicChildController(this->hostname));
        new_controller->join();
        return 0;
    }
};


TEST_F(DynamicControllerSimulationTest, DynamicControllerCreationJoining) {
    DO_TEST_WITH_FORK(do_dynamicControllerTest_test);
}

void DynamicControllerSimulationTest::do_dynamicControllerTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);
    simulation->instantiatePlatform(platform_file_path);

    // Create Execution Controllers
    auto c1 = simulation->add(new TestDynamicMainController("Host1"));

    // Running simulation
    simulation->launch();

    // Checking coherence
    ASSERT_DOUBLE_EQ(20, simulation->getCurrentSimulatedDate());

    // Freeing argv
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
