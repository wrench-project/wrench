/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "wrench/workflow/job/PilotJob.h"
#include "NoopScheduler.h"
#include "TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Log category for test");


class MultihostMulticoreComputeServiceTestScheduling : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::StorageService *storage_service1 = nullptr;
//    wrench::WorkflowFile *output_file1;
//    wrench::WorkflowFile *output_file2;
//    wrench::WorkflowFile *output_file3;
//    wrench::WorkflowFile *output_file4;
//    wrench::WorkflowTask *task1;
//    wrench::WorkflowTask *task2;
//    wrench::WorkflowTask *task3;
//    wrench::WorkflowTask *task4;

    wrench::ComputeService *compute_service = nullptr;

    void do_Noop_test();



protected:
    MultihostMulticoreComputeServiceTestScheduling() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
//      input_file = workflow->addFile("input_file", 10.0);
//      output_file1 = workflow->addFile("output_file1", 10.0);

//      // Create one task
//      task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0);

      // Add file-task dependencies
//      task1->addInputFile(input_file);
//
//      task1->addOutputFile(output_file1);


      // Create a two-host quad-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"4\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"4\"/> "
              "   </AS> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  NOOP SIMULATION TEST                                            **/
/**********************************************************************/

class XNoopTestWMS : public wrench::WMS {

public:
    XNoopTestWMS(MultihostMulticoreComputeServiceTestScheduling *test,
                wrench::Workflow *workflow,
                std::unique_ptr<wrench::Scheduler> scheduler,
                std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }


private:

    MultihostMulticoreComputeServiceTestScheduling *test;

    int main() {


//      // Create a job manager
//      std::unique_ptr<wrench::JobManager> job_manager =
//              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
//
//      // Create a data movement manager
//      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
//              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));
//
//      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      return 0;
    }
};

TEST_F(MultihostMulticoreComputeServiceTestScheduling, Noop) {
  DO_TEST_WITH_FORK(do_Noop_test);
}

void MultihostMulticoreComputeServiceTestScheduling::do_Noop_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new XNoopTestWMS(this, workflow,
                                                       std::unique_ptr<wrench::Scheduler>(
                                                               new NoopScheduler()),
                          hostname))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::pair<std::string, unsigned long>(hostname, 0)},
                                                               nullptr,
                                                               {}))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

