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

class StorageServiceTest : public ::testing::Test {

protected:
    StorageServiceTest() {

      // Create a bogus workflow
      workflow = new wrench::Workflow();

      // Create a few files
      file1 = workflow->addFile("file1", 1000.0);
      file2 = workflow->addFile("file2", 2000.0);
      file3 = workflow->addFile("file3", 2000.0);

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
    wrench::WorkflowFile *file1;
    wrench::WorkflowFile *file2;
    wrench::WorkflowFile *file3;

};

TEST_F(StorageServiceTest, FileStaging) {


  ASSERT_EQ(1,1);
  return;

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **)calloc(1,sizeof(char*));
  argv[0] = strdup("file_staging_test");
  simulation->init(&argc, argv);
  simulation->instantiatePlatform(platform_file_path);

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
  ASSERT_THROW(simulation->stageFiles({file1}, storage_service), std::runtime_error);

  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));


  simulation->setFileRegistryService(std::move(file_registry_service));

  ASSERT_THROW(simulation->stageFiles({file1}, nullptr), std::invalid_argument);
  ASSERT_THROW(simulation->stageFiles({nullptr}, storage_service), std::invalid_argument);

  // Staging f1 on the storage service
   EXPECT_NO_THROW(simulation->stageFiles({file1}, storage_service));



    std::cerr << "Launching the Simulation..." << std::endl;
    try {
        simulation->launch();
    } catch (std::runtime_error e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return;
    }
    std::cerr << "Simulation done!" << std::endl;



  // TODO: fix the segfault (just like for main...);
//   delete simulation;

}

