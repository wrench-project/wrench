/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <cmath>

#include <gtest/gtest.h>

#include <wrench-dev.h>

#include "../../include/TestWithFork.h"


#define FILE_SIZE 10000000000.00
#define STORAGE_SIZE (100.0 * FILE_SIZE)

class SimpleStorageServiceLimitedConnectionsTest : public ::testing::Test {

public:
    wrench::WorkflowFile *files[10];
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service_wms = nullptr;
    wrench::StorageService *storage_service_1 = nullptr;
    wrench::StorageService *storage_service_2 = nullptr;

    void do_ConcurrencyFileCopies_test();


protected:
    SimpleStorageServiceLimitedConnectionsTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      for (size_t i=0; i < 10; i++) {
        files[i] = workflow->addFile("file_"+std::to_string(i), FILE_SIZE);
      }

      // Create a 3-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\"/> "
              "       <host id=\"Host2\" speed=\"1f\"/> "
              "       <host id=\"WMSHost\" speed=\"1f\"/> "
              "       <link id=\"link1\" bandwidth=\"10MBps\" latency=\"100us\"/>"
              "       <link id=\"link2\" bandwidth=\"10MBps\" latency=\"100us\"/>"
              "       <route src=\"WMSHost\" dst=\"Host1\">"
              "         <link_ctn id=\"link1\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"Host2\">"
              "         <link_ctn id=\"link2\"/>"
              "       </route>"
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
/**  CONCURRENT FILE COPIES TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS(SimpleStorageServiceLimitedConnectionsTest *test,
                                                                       const std::set<wrench::ComputeService *> &compute_services,
                                                                       const std::set<wrench::StorageService *> &storage_services,
                                                                       const std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceLimitedConnectionsTest *test;

    int main() {


      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Get a file registry
      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      // Reading
      for (auto dst_storage_service : {this->test->storage_service_1, this->test->storage_service_2}) {

        for (auto reading : {true, false}) {

          // Initiate 10 asynchronous file copies to/from storage_service_1 (unlimited)
          double start = this->simulation->getCurrentSimulatedDate();
          for (int i = 0; i < 10; i++) {
            if (reading) {
              data_movement_manager->initiateAsynchronousFileCopy(this->test->files[i],
                                                                  this->test->storage_service_wms,
                                                                  dst_storage_service);
            } else {
              data_movement_manager->initiateAsynchronousFileCopy(this->test->files[i],
                                                                  dst_storage_service,
                                                                  this->test->storage_service_wms);
            }
          }

          double completion_dates[10];
          for (int i = 0; i < 10; i++) {
            std::unique_ptr<wrench::WorkflowExecutionEvent> event1 = this->getWorkflow()->waitForNextExecutionEvent();
            if (event1->type != wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION) {
              throw std::runtime_error("Unexpected Workflow Execution Event " + std::to_string(event1->type));
            }
            completion_dates[i] = this->simulation->getCurrentSimulatedDate();
          }

          // Check results for the unlimited storage service
          if (dst_storage_service == this->test->storage_service_1) {
            double baseline_elapsed = completion_dates[0] - start;
            for (int i=1; i < 10; i++) {
              if (fabs(baseline_elapsed - (completion_dates[i] - start)) > 1) {
                throw std::runtime_error("Incoherent transfer elapsed times for the unlimited storage service");
              }
            }
          }

          // Check results for the limited storage service
          if (dst_storage_service == this->test->storage_service_2) {
            bool success = true;

            if ((fabs(completion_dates[0] - completion_dates[1]) > 1) ||
                (fabs(completion_dates[2] - completion_dates[1]) > 1) ||
                (fabs(completion_dates[4] - completion_dates[3]) > 1) ||
                (fabs(completion_dates[5] - completion_dates[4]) > 1) ||
                (fabs(completion_dates[7] - completion_dates[6]) > 1) ||
                (fabs(completion_dates[8] - completion_dates[7]) > 1)) {
              success = false;
            }

            double elapsed_1 = completion_dates[0] - start;
            double elapsed_2 = completion_dates[3] - completion_dates[0];
            double elapsed_3 = completion_dates[6] - completion_dates[3];
            double elapsed_4 = completion_dates[9] - completion_dates[8];

            if ((fabs(elapsed_2 - elapsed_1) > 1) ||
                (fabs(elapsed_3 - elapsed_2) > 1)) {
              success = false;
            }

            if (fabs(3.0 * elapsed_4 - elapsed_3) > 1) {
              success = false;
            }

            if (not success) {
              throw std::runtime_error("Incoherent transfer elapsed times for the limited storage service");
            }
          }
        }
      }


      return 0;
    }
};

TEST_F(SimpleStorageServiceLimitedConnectionsTest, ConcurrencyFileCopies) {
  DO_TEST_WITH_FORK(do_ConcurrencyFileCopies_test);
}

void SimpleStorageServiceLimitedConnectionsTest::do_ConcurrencyFileCopies_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("performance_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a (unused) Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
                  new wrench::MultihostMulticoreComputeService("WMSHost",
                                                               {std::make_pair("WMSHost",
                                                                               std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                               0,
                                                               {{wrench::MultihostMulticoreComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "true"}, {wrench::MultihostMulticoreComputeServiceProperty::SUPPORTS_PILOT_JOBS, "true"}})));

  // Create a Local storage service with unlimited connections
  ASSERT_NO_THROW(storage_service_wms = simulation->add(
                  new wrench::SimpleStorageService("WMSHost", STORAGE_SIZE)));

  // Create a Storage service with unlimited connections
  ASSERT_NO_THROW(storage_service_1 = simulation->add(
                  new wrench::SimpleStorageService("Host1", STORAGE_SIZE)));

  // Create a Storage Service limited to 3 connections
  ASSERT_NO_THROW(storage_service_2 = simulation->add(
                  new wrench::SimpleStorageService("Host2", STORAGE_SIZE, {{"MAX_NUM_CONCURRENT_DATA_CONNECTIONS", "3"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS(
                  this, {compute_service}, {storage_service_wms, storage_service_1, storage_service_2},
                  "WMSHost")));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  simulation->add(new wrench::FileRegistryService("WMSHost"));

  // Staging all files on the WMS storage service
  for (int i=0; i < 10; i++) {
    ASSERT_NO_THROW(simulation->stageFile(files[i], storage_service_wms));
  }

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
