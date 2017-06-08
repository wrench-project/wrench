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
#include <stdio.h>
#include <services/storage_services/simple_storage_service/SimpleStorageService.h>
#include <wms/WMS.h>

class SimulationTest : public ::testing::Test {

protected:
    SimulationTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create two files
      input_file = workflow->addFile("file1", 10000.0);
      output_file = workflow->addFile("file2", 20000.0);

      // Create one task
      task = workflow->addTask("task", 3600);

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
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;

};

class TestWMS : public wrench::WMS {

public:
    TestWMS(wrench::Simulation *simulation, wrench::Workflow *workflow, std::unique_ptr<wrench::Scheduler> scheduler, std::string hostname, std::string suffix) :
            wrench::WMS(simulation, workflow, std::move(scheduler), hostname, suffix) {}


private:
    int main() {
      printf("NOTHING");

      return 0;
    }
};

TEST_F(SimulationTest, SimulationSetup) {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **)calloc(1,sizeof(char*));
  argv[0] = strdup("file_staging_test");


  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  // TODO: This should be about the platform!!!!
  ASSERT_THROW(simulation->launch(), std::runtime_error);

  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Create a simple storage service
  std::string hostname = simulation->getHostnameList()[0];

  wrench::StorageService *storage_service = nullptr;


  ASSERT_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService("not_a_hostname", 10000000000000.0))), std::invalid_argument);
  ASSERT_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, -1))), std::invalid_argument);

  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Without a file registry service this should fail
  ASSERT_THROW(simulation->stageFiles({input_file}, storage_service), std::runtime_error);

  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));


  simulation->setFileRegistryService(std::move(file_registry_service));

  ASSERT_THROW(simulation->stageFiles({input_file}, nullptr), std::invalid_argument);
  ASSERT_THROW(simulation->stageFiles({nullptr}, storage_service), std::invalid_argument);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));


    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return;
    }
    std::cerr << "Simulation done!" << std::endl;

}

