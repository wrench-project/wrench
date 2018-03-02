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

#include "../include/TestWithFork.h"

class FileRegistryTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_FileRegistry_Test();

protected:
    FileRegistryTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create a one-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"1000GBps\" latency=\"1000us\"/>"
              "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"0us\"/>"
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

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;

};

/**********************************************************************/
/**  SIMPLE PROXIMITY TEST                                            **/
/**********************************************************************/

class FileRegistryTestWMS : public wrench::WMS {

public:
    FileRegistryTestWMS(FileRegistryTest *test,
                const std::set<wrench::ComputeService *> &compute_services,
                const std::set<wrench::StorageService *> &storage_services,
                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    FileRegistryTest *test;

    int main() {


      wrench::WorkflowFile *file1 = workflow->addFile("file1", 100.0);
      wrench::WorkflowFile *file2 = workflow->addFile("file2", 100.0);
      wrench::FileRegistryService *frs = simulation->getFileRegistryService();

      bool success;
      std::set<wrench::StorageService *> locations;

      success = true;
      try {
        frs->addEntry(nullptr, this->test->storage_service1);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to add an entry with a nullptr file");
      }

      success = true;
      try {
        frs->addEntry(file1, nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to add an entry with a nullptr storage service");
      }

      success = true;
      try {
        frs->lookupEntry(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup an entry with a nullptr file");
      }

      // Adding twice file1
      frs->addEntry(file1, this->test->storage_service1);
      frs->addEntry(file1, this->test->storage_service1);

      // Getting file1
      try {
        locations = frs->lookupEntry(file1);
      } catch (std::exception &e) {
        throw std::runtime_error("Should not have gotten an exception when looking up entry");
      }

      if (locations.size() != 1) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }
      if ((*locations.begin()) != this->test->storage_service1) {
        throw std::runtime_error("Got the wrong location for file 1");
      }

      // Adding another location for file1
      frs->addEntry(file1, this->test->storage_service2);

      // Getting file1
      locations = frs->lookupEntry(file1);

      if (locations.size() != 2) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }
      if ((locations.find(this->test->storage_service1) == locations.end()) ||
          (locations.find(this->test->storage_service2) == locations.end())) {
        throw std::runtime_error("Got the wrong locations for file 1");
      }

      success = true;
      try {
        frs->removeEntry(nullptr, this->test->storage_service1);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to remove an entry with a nullptr file");
      }

      success = true;
      try {
        frs->removeEntry(file1, nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to remove an entry with a nullptr storage service");
      }

      frs->removeEntry(file1, this->test->storage_service1);

      // Getting file1
      locations = frs->lookupEntry(file1);

      if (locations.size() != 1) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }
      if (locations.find(this->test->storage_service2) == locations.end()) {
        throw std::runtime_error("Got the wrong locations for file 1");
      }

      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};

TEST_F(FileRegistryTest, SimpleFunctionality) {
  DO_TEST_WITH_FORK(do_FileRegistry_Test);
}

void FileRegistryTest::do_FileRegistry_Test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("file_registry_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr,
                                                               {}))));
  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new FileRegistryTestWMS(
                  this,
                  {compute_service}, {storage_service1, storage_service2}, hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry service
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  EXPECT_THROW(simulation->setFileRegistryService(nullptr), std::invalid_argument);
  EXPECT_NO_THROW(simulation->setFileRegistryService(std::move(file_registry_service)));


  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

