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

WRENCH_LOG_CATEGORY(multi_controller_test, "Log category for MultiControllerSimulationTest");


class MultiControllerSimulationTest : public ::testing::Test {

public:
    void do_multiControllerTest_test();

protected:
    ~MultiControllerSimulationTest() override {
    }

    MultiControllerSimulationTest() {
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
/**            GET READY TASKS SIMULATION TEST ON ONE HOST           **/
/**********************************************************************/

class TestController : public wrench::ExecutionController {

public:
    TestController(std::string controller_name,
                   double sleep_time,
                   double *finish_time,
                   const std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->controller_name = controller_name;
        this->sleep_time = sleep_time;
        this->finish_time = finish_time;
    }

private:
    int main() override {
        *(this->finish_time) = -1.0;
        wrench::Simulation::sleep(this->sleep_time);
        double now = wrench::Simulation::getCurrentSimulatedDate();
        *(this->finish_time) = now;
        return 0;
    }

    std::string controller_name;
    double sleep_time;
    double *finish_time;
};

TEST_F(MultiControllerSimulationTest, DaemonizedNonDaemonizedTest) {
    DO_TEST_WITH_FORK(do_multiControllerTest_test);
}

void MultiControllerSimulationTest::do_multiControllerTest_test() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    simulation->init(&argc, argv);
    simulation->instantiatePlatform(platform_file_path);

    // Create Execution Controllers
    double ft1, ft2, ft3;
    auto c1 = simulation->add(new TestController("Controller#1", 10, &ft1, "Host1"));
    auto c2 = simulation->add(new TestController("Controller#2", 100, &ft2, "Host1"));
    auto c3 = simulation->add(new TestController("Controller#3", 30, &ft3, "Host1"));
    c1->setDaemonized(false);
    c2->setDaemonized(true);
    c3->setDaemonized(false);

    // Running simulation
    simulation->launch();

    // Checking coherence
    ASSERT_DOUBLE_EQ(ft1, 10);
    ASSERT_DOUBLE_EQ(ft2, -1);
    ASSERT_DOUBLE_EQ(ft3, 30);

    // Freeing argv
    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
