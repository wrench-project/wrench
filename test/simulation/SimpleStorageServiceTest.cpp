/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <gtest/gtest.h>

#include <workflow/Workflow.h>
#include <simulation/Simulation.h>
#include <services/storage_services/simple_storage_service/SimpleStorageServiceTest.h>
#include <wms/scheduler/RandomScheduler.h>
#include <services/compute_services/multicore_compute_service/MulticoreComputeService.h>
#include <managers/data_movement_manager/DataMovementManager.h>
#include <wrench-dev.h>

// Convenient macro to lauch a test inside a separate process
// and check the exit code, which denotes an error
#define DO_TEST_WITH_FORK(function){ \
                                      pid_t pid = fork(); \
                                      if (pid) { \
                                        int exit_code; \
                                        waitpid(pid, &exit_code, 0); \
                                        ASSERT_EQ(exit_code, 0); \
                                      } else { \
                                        this->function(); \
                                        exit((::testing::Test::HasFailure() ? 666 : 0)); \
                                      } \
                                   }


class SimpleStorageServiceTest : public ::testing::Test {

public:
    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_10;
    wrench::WorkflowFile *file_100;
    wrench::WorkflowFile *file_500;
//    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service_100 = nullptr;
    wrench::StorageService *storage_service_500 = nullptr;
    wrench::StorageService *storage_service_1000 = nullptr;

    wrench::ComputeService *compute_service = nullptr;

    void do_CapacitySimulation_test();


protected:
    SimpleStorageServiceTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      file_1 = workflow->addFile("file_1", 1.0);
      file_10 = workflow->addFile("file_10", 10.0);
      file_100 = workflow->addFile("file_100", 100.0);
      file_500 = workflow->addFile("file_500", 500.0);

      // Create one task
//      task = workflow->addTask("task", 3600);
//      task->addInputFile(input_file);
//      task->addOutputFile(output_file);

      // Create a one-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"SingleHost\" speed=\"1f\"/> "
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
/**  CAPACITY SIMULATION TEST                                        **/
/**********************************************************************/

class CapacitySimulationTestWMS : public wrench::WMS {

public:
    CapacitySimulationTestWMS(SimpleStorageServiceTest *test,
                          wrench::Workflow *workflow,
                          std::unique_ptr<wrench::Scheduler> scheduler,
                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceTest *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(SimpleStorageServiceTest, CapacitySimulationTest) {
  DO_TEST_WITH_FORK(do_CapacitySimulation_test);
}

void SimpleStorageServiceTest::do_CapacitySimulation_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new CapacitySimulationTestWMS(this, workflow,
                                                                 std::unique_ptr<wrench::Scheduler>(
                                                                         new wrench::RandomScheduler()),
                          hostname))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      nullptr,
                                                      {}))));

  // Create Three Storage Services
  EXPECT_NO_THROW(storage_service_100 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));
  EXPECT_NO_THROW(storage_service_500 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 500.0))));
  EXPECT_NO_THROW(storage_service_1000 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 1000.0))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));


  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging all files on the 1000 storage service
  EXPECT_NO_THROW(simulation->stageFiles({file_1, file_10, file_100, file_500}, storage_service_1000));


  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}
