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

class SimulationCommandLineArgumentsTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service = nullptr;

    void do_versionArgument_test();

    void do_HelpWrenchArgument_test();

    void do_HelpSimGridArgument_test();

    void do_HelpArgument_test();

    void do_NoColorArgument_test();

    void do_FullLogArgument_test(std::string arg, int num_log_lines);

    void do_ActivateEnergyArgument_test();

protected:

    ~SimulationCommandLineArgumentsTest() {
        workflow->clear();
    }

    SimulationCommandLineArgumentsTest() {
        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();


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

class SimulationCommandLineArgumentsWMS : public wrench::WMS {

public:
    SimulationCommandLineArgumentsWMS(SimulationCommandLineArgumentsTest *test,
                                      std::shared_ptr<wrench::Workflow> workflow,
                                      std::string &hostname) :
            wrench::WMS(workflow, nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    SimulationCommandLineArgumentsTest *test;

    int main() {
        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Sleep for 1 second
        wrench::Simulation::sleep(1.0);

        return 0;
    }
};

/**********************************************************************/
/**           VERSION COMMAND-LINE ARGUMENT                          **/
/**********************************************************************/

TEST_F(SimulationCommandLineArgumentsTest, VersionArgument) {
    DO_TEST_WITH_FORK(do_versionArgument_test);
}

void SimulationCommandLineArgumentsTest::do_versionArgument_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-version");

    pid_t pid = fork();

    if (pid == 0) { // Child
        close(1);
        FILE *stdout_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stdout").c_str(), "w");
        ASSERT_NO_THROW(simulation->init(&argc, argv));
        // Will not get here
        fclose(stdout_file);
    }

    int exit_code;
    waitpid(pid, &exit_code, 0);

    // Check exit code
    ASSERT_EQ(exit_code, 0);

    // Check file content
    FILE *stdout_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stdout").c_str(), "r");
    int linecount = 0;
    char *line = nullptr;
    ssize_t read;
    size_t linecapp;
    while (getline(&line, &linecapp, stdout_file) != -1) {
        linecount++;
        if (linecount == 1) {
            ASSERT_EQ(strncmp(line, "This program was linked against WRENCH version ",
                              strlen("This program was linked against WRENCH version ")), 0);
        }
        if (linecount == 17) {
            ASSERT_EQ(strncmp(line, "This program was linked against SimGrid version ",
                              strlen("This program was linked against SimGrid version ")), 0);
        }
        free(line);
        line = nullptr;
    }

    fclose(stdout_file);

    ASSERT_GE(linecount, 2);


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**           HELP-WRENCH COMMAND-LINE ARGUMENT                      **/
/**********************************************************************/


TEST_F(SimulationCommandLineArgumentsTest, HelpWrenchArgument) {
    DO_TEST_WITH_FORK(do_HelpWrenchArgument_test);
}

void SimulationCommandLineArgumentsTest::do_HelpWrenchArgument_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--help-wrench");

    pid_t pid = fork();

    if (pid == 0) { // Child
        close(1);
        FILE *stdout_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stdout").c_str(), "w");
        ASSERT_NO_THROW(simulation->init(&argc, argv));
        // Doesn't get here
    }

    int exit_code;
    waitpid(pid, &exit_code, 0);

    // Check exit code
    ASSERT_EQ(exit_code, 0);

    // Check file content
    FILE *stdout_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stdout").c_str(), "r");
    int linecount = 0;
    char *line = nullptr;
    ssize_t read;
    size_t linecapp;
    while (getline(&line, &linecapp, stdout_file) != -1) {
        linecount++;
        free(line);
        line = nullptr;
    }
    fclose(stdout_file);

    ASSERT_GE(linecount, 5);
    ASSERT_LE(linecount, 15);


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**           HELP-SIMGRID COMMAND-LINE ARGUMENT                      **/
/**********************************************************************/


TEST_F(SimulationCommandLineArgumentsTest, HelpSimGridArgument) {
    DO_TEST_WITH_FORK(do_HelpSimGridArgument_test);
}

void SimulationCommandLineArgumentsTest::do_HelpSimGridArgument_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--help-simgrid");

    pid_t pid = fork();

    if (pid == 0) { // Child
        // Create and initialize a simulation

        close(1);
        FILE *stdout_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stdout").c_str(), "w");
        simulation->init(&argc, argv);
        // Will not get here
        fclose(stdout_file);
    }

    int exit_code;
    waitpid(pid, &exit_code, 0);

    // Check exit code
    ASSERT_EQ(exit_code, 0);

    // Check file content
    FILE *stdout_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stdout").c_str(), "r");
    int linecount = 0;
    char *line = nullptr;
    ssize_t read;
    size_t linecapp;
    while (getline(&line, &linecapp, stdout_file) != -1) {
        linecount++;
        free(line);
        line = nullptr;
    }
    fclose(stdout_file);

    ASSERT_GE(linecount, 100);
    ASSERT_LE(linecount, 300);


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**           HELP COMMAND-LINE ARGUMENT                             **/
/**********************************************************************/


TEST_F(SimulationCommandLineArgumentsTest, HelpArgument) {
    DO_TEST_WITH_FORK(do_HelpArgument_test);
}

void SimulationCommandLineArgumentsTest::do_HelpArgument_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--help");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Check that the arguments are unmodified
    ASSERT_EQ(argc, 2);
    ASSERT_EQ(!strcmp(argv[1], "--help"), 1);


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**           NO-COLOR     COMMAND-LINE ARGUMENT                     **/
/**********************************************************************/

TEST_F(SimulationCommandLineArgumentsTest, NoColorArgument) {
    DO_TEST_WITH_FORK(do_NoColorArgument_test);
}

void SimulationCommandLineArgumentsTest::do_NoColorArgument_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-no-color");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Redirecting to file, but this is complicated due to the simgrid logging...
    close(2);
    FILE *stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "w");

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    std::string hostname = "DualCoreHost";
    wms = simulation->add(new SimulationCommandLineArgumentsWMS(this, workflow, hostname));

    ASSERT_NO_THROW(simulation->launch());

    fclose(stderr_file);

    // Here we could programmatically check there are no colors.... yawn
    stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "r");
    ssize_t read;
    char *line = nullptr;
    size_t linecapp;
    bool found_color = false;
    while (getline(&line, &linecapp, stderr_file) != -1) {
        if (linecapp >= 3) {
            for (size_t i = 0; i < strlen(line) - 2; i++) {
                if ((line[i] == '\033') and (line[i + 1] == '[') and (line[i + 2] == '1')) {
                    found_color = true;
                    break;
                }
            }
        }
        free(line);
        line = nullptr;
        if (found_color) {
            break;
        }
    }
    fclose(stderr_file);

    ASSERT_EQ(found_color, false);



    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**           FULL_LOG COMMAND-LINE ARGUMENT                         **/
/**********************************************************************/


TEST_F(SimulationCommandLineArgumentsTest, FullLogArgument) {
    DO_TEST_WITH_FORK_TWO_ARGS(do_FullLogArgument_test, "", 0);
    DO_TEST_WITH_FORK_TWO_ARGS(do_FullLogArgument_test, "--wrench-full-log", 3);
}

void SimulationCommandLineArgumentsTest::do_FullLogArgument_test(std::string arg, int num_log_lines) {
    // Undo the SimGrid Logging config for Google Tests
    xbt_log_control_set("root.thresh:info");

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup(arg.c_str());

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Redirecting to file, but this is complicated due to the simgrid logging...
    close(2);
    FILE *stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "w");

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    std::string hostname = "DualCoreHost";
    wms = simulation->add(new SimulationCommandLineArgumentsWMS(this, workflow, hostname));

    ASSERT_NO_THROW(simulation->launch());

    fclose(stderr_file);

    // Check file content
    stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "r");
    int linecount = 0;
    char *line = nullptr;
    size_t linecapp;
    while (getline(&line, &linecapp, stderr_file) != -1) {
        linecount++;
        free(line);
        line = nullptr;
    }
    fclose(stderr_file);

    ASSERT_EQ(linecount, num_log_lines);

    // Just in case
    xbt_log_control_set("root.thresh:critical");


    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**   ACTIVATE-ENERGY  COMMAND-LINE ARGUMENT                         **/
/**********************************************************************/


// THIS TEST IS BROKEN
TEST_F(SimulationCommandLineArgumentsTest, ActivateEnergyArgument) {
    DO_TEST_WITH_FORK(do_ActivateEnergyArgument_test);
}

void SimulationCommandLineArgumentsTest::do_ActivateEnergyArgument_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-energy-simulation");

    simulation->init(&argc, argv);

    simulation->instantiatePlatform(platform_file_path);

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    std::string hostname = "DualCoreHost";
    wms = simulation->add(new SimulationCommandLineArgumentsWMS(this, workflow, hostname));

    simulation->launch();

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
