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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(simulation_logging_test, "Log category for Simulation Logging Test");


class SimulationLoggingTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service = nullptr;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    void do_logging_test();

protected:

    SimulationLoggingTest() {
        // Create the simplest workflow
        workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
        workflow = workflow_unique_ptr.get();

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"/> "
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

class SimulationLoggingWMS : public wrench::WMS {

public:
    SimulationLoggingWMS(SimulationLoggingTest *test,
                         std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:

    SimulationLoggingTest *test;

    int main() {

        WRENCH_INFO("Testing WRENCH_INFO without color");
        wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Testing WRENCH_INFO");
        wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_YELLOW);
        WRENCH_WARN("Testing WRENCH_WARN");
        wrench::TerminalOutput::setThisProcessLoggingColor(wrench::TerminalOutput::COLOR_GREEN);
        WRENCH_DEBUG("Testing WRENCH_DEBUG");

        // Sleep for 1 second
        wrench::Simulation::sleep(1.0);

        return 0;
    }
};

/**********************************************************************/
/**          BASIC LOGGING TEST                                      **/
/**********************************************************************/


TEST_F(SimulationLoggingTest, Logging) {
    DO_TEST_WITH_FORK(do_logging_test);
}

void SimulationLoggingTest::do_logging_test() {

    // Re-enable logging just for this test
    xbt_log_control_set("simulation_logging_test.thresh:debug");

    // Create and initialize a simulation
    auto *simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("command_line_args_test");
    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);

    std::string hostname = "DualCoreHost";

    auto wms = simulation->add(new SimulationLoggingWMS(this, hostname));
    wms->addWorkflow(new wrench::Workflow(), 0);

    simulation->launch();

    delete simulation;
    free(argv[0]);
    free(argv);
}


