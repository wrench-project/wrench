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

class SimulationCommandLineArgumentsTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service = nullptr;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;

    void do_versionArgument_test();
    void do_HelpWrenchArgument_test();
    void do_HelpSimGridArgument_test();
    void do_HelpArgument_test();
    void do_NoColorArgument_test();
    void do_NoLogArgument_test();
    void do_ActivateEnergyArgument_test();

protected:

    SimulationCommandLineArgumentsTest() {
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

class SimulationCommandLineArgumentsWMS : public wrench::WMS {

public:
    SimulationCommandLineArgumentsWMS(SimulationCommandLineArgumentsTest *test,
                                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimulationCommandLineArgumentsTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Do nothing
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
  auto *simulation = new wrench::Simulation();
  int argc = 2;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("command_line_args_test");
  argv[1] = strdup("--version");

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
      ASSERT_EQ(strncmp(line, "WRENCH version ", strlen("WRENCH version ")), 0);
    }
    if (linecount == 2) {
      ASSERT_EQ(strncmp(line, "SimGrid version ", strlen("SimGrid version ")), 0);
    }
    free(line);
    line = nullptr;
  }

  fclose(stdout_file);

  ASSERT_GE(linecount, 2);

  delete simulation;
  free(argv[0]);
  free(argv[1]);
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
  auto *simulation = new wrench::Simulation();
  int argc = 2;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("command_line_args_test");
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

  delete simulation;
  free(argv[0]);
  free(argv[1]);
  free(argv);
}

/**********************************************************************/
/**           HELP-SIMGRID COMMAND-LINE ARGUMENT                      **/
/**********************************************************************/


TEST_F(SimulationCommandLineArgumentsTest, HelpSimGridArgument) {
  DO_TEST_WITH_FORK(do_HelpSimGridArgument_test);
}

void SimulationCommandLineArgumentsTest::do_HelpSimGridArgument_test() {

  auto *simulation = new wrench::Simulation();
  int argc = 2;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("command_line_args_test");
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

  ASSERT_GE(linecount, 150);
  ASSERT_LE(linecount, 250);

  delete simulation;
  free(argv[0]);
  free(argv[1]);
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
  auto *simulation = new wrench::Simulation();
  int argc = 2;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("command_line_args_test");
  argv[1] = strdup("--help");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Check that the arguments are unmodified
  ASSERT_EQ(argc, 2);
  ASSERT_EQ(!strcmp(argv[1], "--help"), 1);

  delete simulation;
  free(argv[0]);
  free(argv[1]);
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
  auto *simulation = new wrench::Simulation();
  int argc = 2;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("command_line_args_test");
  argv[1] = strdup("--wrench-no-color");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Redirecting to file, but this is complicated due to the simgrid logging...
  close(2);
  FILE *stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "w");

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  std::string hostname = "DualCoreHost";
  wms = simulation->add(new SimulationCommandLineArgumentsWMS(this, hostname));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

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


  delete simulation;
  free(argv[0]);
  free(argv[1]);
  free(argv);
}





/**********************************************************************/
/**           NO-LOG COMMAND-LINE ARGUMENT                           **/
/**********************************************************************/


TEST_F(SimulationCommandLineArgumentsTest, NoLogArgument) {
  DO_TEST_WITH_FORK(do_NoLogArgument_test);
}

void SimulationCommandLineArgumentsTest::do_NoLogArgument_test() {

  // Undo the SimGrid Logging config for Google Tests
  xbt_log_control_set("root.thresh:info");

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 2;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("command_line_args_test");
  argv[1] = strdup("--wrench-no-log");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Redirecting to file, but this is complicated due to the simgrid logging... 
  close(2);
  FILE *stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "w");

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  std::string hostname = "DualCoreHost";
  wms = simulation->add(new SimulationCommandLineArgumentsWMS(this, hostname));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  ASSERT_NO_THROW(simulation->launch());

  fclose(stderr_file);

  // Check file content
  stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "r");
  int linecount = 0;
  char *line = nullptr;
  ssize_t read;
  size_t linecapp;
  while (getline(&line, &linecapp, stderr_file) != -1) {
    linecount++;
    free(line);
    line = nullptr;
  }
  fclose(stderr_file);

  ASSERT_EQ(linecount, 0);

  // Just in case
  xbt_log_control_set("root.thresh:critical");

  delete simulation;
  free(argv[0]);
  free(argv[1]);
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
  auto *simulation = new wrench::Simulation();
  int argc = 2;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("command_line_args_test");
  argv[1] = strdup("--activate-energy");

  pid_t pid = fork();

  FILE *stderr_file;

  if (pid == 0) { // Child

    close(2); // Mute it
    stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "w");

    simulation->init(&argc, argv);

    // Setting up the platform
    // TODO: This should really throw something (due to no pstates in XML),
    try {
      simulation->instantiatePlatform(platform_file_path);
    } catch (simgrid::Exception &e) {
        // So sad we never get here
      exit(1);
    }

    // Create a WMS
    wrench::WMS *wms = nullptr;
    std::string hostname = "DualCoreHost";
    wms = simulation->add(new SimulationCommandLineArgumentsWMS(this, hostname));

    wms->addWorkflow(workflow);

    simulation->launch();

    exit(0); // Should not doesn't get here
  }


  int exit_code;
  waitpid(pid, &exit_code, 0);

  // Check exit code
  ASSERT_NE(exit_code, 0);

  std::cerr << "hERE\n";
  // Check that the error was what we thought it was (based on output!)
  stderr_file = fopen((UNIQUE_TMP_PATH_PREFIX + "unit_tests.stderr").c_str(), "r");
  char *line = nullptr;
  ssize_t read;
  size_t linecapp;
  bool found_error = false;
  char *to_find = strdup("No power range properties");
  while (getline(&line, &linecapp, stderr_file) != -1) {
    for (size_t i=0; i < strlen(line) - strlen(to_find); i++) {
      if (!strncmp(line+i, to_find, strlen(to_find))) {
        found_error = true;
        break;
      }
    }
    free(line);
    line = nullptr;
    if (found_error) {
      break;
    }
  }
  free(to_find);
  fclose(stderr_file);

  ASSERT_EQ(found_error, true);

  delete simulation;
  free(argv[0]);
  free(argv[1]);
  free(argv);
}


