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
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service_performance_test, "Log category for SimpleStorageServicePerformanceTest");


#define FILE_SIZE 10000000000.00
#define MBPS_BANDWIDTH 100
#define STORAGE_SIZE (100.0 * FILE_SIZE)

class SimpleStorageServicePerformanceTest : public ::testing::Test {

public:
    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_2;
    wrench::WorkflowFile *file_3;
    wrench::StorageService *storage_service_1 = nullptr;
    wrench::StorageService *storage_service_2 = nullptr;

    wrench::ComputeService *compute_service = nullptr;

    void do_FileRead_test();
    void do_ConcurrentFileCopies_test();


protected:
    SimpleStorageServicePerformanceTest() {

      // Create the simplest workflow
      workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
      workflow = workflow_unique_ptr.get();

      // Create the files
      file_1 = workflow->addFile("file_1", FILE_SIZE);
      file_2 = workflow->addFile("file_2", FILE_SIZE);
      file_3 = workflow->addFile("file_3", FILE_SIZE);

      // Create a 3-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"SrcHost\" speed=\"1f\"/> "
              "       <host id=\"DstHost\" speed=\"1f\"/> "
              "       <host id=\"WMSHost\" speed=\"1f\"/> "
              "       <link id=\"link\" bandwidth=\"" + std::to_string(MBPS_BANDWIDTH) + "MBps\" latency=\"1us\"/>"
              "       <route src=\"SrcHost\" dst=\"DstHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"SrcHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"DstHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  CONCURRENT FILE COPIES TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceConcurrentFileCopiesTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceConcurrentFileCopiesTestWMS(SimpleStorageServicePerformanceTest *test,
                                                     const std::set<wrench::ComputeService *> compute_services,
                                                     const std::set<wrench::StorageService *> &storage_services,
                                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServicePerformanceTest *test;

    int main() {

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();


      // Time the time it takes to transfer a file from Src to Dst
      double copy1_start = this->simulation->getCurrentSimulatedDate();
      data_movement_manager->initiateAsynchronousFileCopy(this->test->file_1,
                                                          this->test->storage_service_1,
                                                          this->test->storage_service_2);

      std::unique_ptr<wrench::WorkflowExecutionEvent> event1 = this->getWorkflow()->waitForNextExecutionEvent();
      double event1_arrival = this->simulation->getCurrentSimulatedDate();

      // Now do 2 of them in parallel
      double copy2_start = this->simulation->getCurrentSimulatedDate();
      data_movement_manager->initiateAsynchronousFileCopy(this->test->file_2,
                                                          this->test->storage_service_1,
                                                          this->test->storage_service_2);

      double copy3_start = this->simulation->getCurrentSimulatedDate();
      data_movement_manager->initiateAsynchronousFileCopy(this->test->file_3,
                                                          this->test->storage_service_1,
                                                          this->test->storage_service_2);


      std::unique_ptr<wrench::WorkflowExecutionEvent> event2 = this->getWorkflow()->waitForNextExecutionEvent();
      double event2_arrival = this->simulation->getCurrentSimulatedDate();

      std::unique_ptr<wrench::WorkflowExecutionEvent> event3 = this->getWorkflow()->waitForNextExecutionEvent();
      double event3_arrival = this->simulation->getCurrentSimulatedDate();

      double transfer_time_1 = event1_arrival - copy1_start;
      double transfer_time_2 = event2_arrival - copy2_start;
      double transfer_time_3 = event3_arrival - copy3_start;

      // Do relative checks
      if (std::abs(copy2_start - copy3_start) > 0.1) {
        throw std::runtime_error("Time between two asynchronous operations is too big");
      }

      if ((std::abs(transfer_time_2 / transfer_time_1 - 2.0) > 0.001) ||
          (std::abs(transfer_time_3 / transfer_time_1 - 2.0) > 0.001)) {
        throw std::runtime_error("Concurrent transfers should be roughly twice slower");
      }

      if (std::abs(event2_arrival - event3_arrival) > 0.1) {
        throw std::runtime_error("Time between two asynchronous operation completions is too big");
      }


      // Do absolute checks
      double expected_transfer_time_1 =  FILE_SIZE / (0.92 * MBPS_BANDWIDTH*1000*1000);

      if (std::abs(transfer_time_1 - expected_transfer_time_1) > 1.0) {
        throw std::runtime_error("Unexpected transfer time #1 " + std::to_string(transfer_time_1) +
                                         " (should be around " + std::to_string(expected_transfer_time_1) + ")");
      }

      double expected_transfer_time_2 =  FILE_SIZE / (0.5 * 0.92 * MBPS_BANDWIDTH*1000*1000);

      if (std::abs(transfer_time_2 - expected_transfer_time_2) > 1.0) {
        throw std::runtime_error("Unexpected transfer time #2 " + std::to_string(transfer_time_2) +
                                 " (should be around " + std::to_string(expected_transfer_time_2) + ")");
      }

      return 0;
    }
};

TEST_F(SimpleStorageServicePerformanceTest, ConcurrentFileCopies) {
  DO_TEST_WITH_FORK(do_ConcurrentFileCopies_test);
}

void SimpleStorageServicePerformanceTest::do_ConcurrentFileCopies_test() {

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
                  new wrench::BareMetalComputeService("WMSHost",
                                                               {std::make_pair("WMSHost", std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))}, {})));

  // Create Two Storage Services
  ASSERT_NO_THROW(storage_service_1 = simulation->add(
                  new wrench::SimpleStorageService("SrcHost", STORAGE_SIZE)));
  ASSERT_NO_THROW(storage_service_2 = simulation->add(
                  new wrench::SimpleStorageService("DstHost", STORAGE_SIZE)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceConcurrentFileCopiesTestWMS(
                  this, {compute_service}, {storage_service_1, storage_service_2},
                          "WMSHost")));

  wms->addWorkflow(this->workflow);

  // Create a file registry
  simulation->add(new wrench::FileRegistryService("WMSHost"));

  // Staging all files on the Src storage service
  ASSERT_NO_THROW(simulation->stageFiles({{file_1->getID(), file_1}, {file_2->getID(), file_2}, {file_3->getID(), file_3}}, storage_service_1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}



/**********************************************************************/
/**  FILE READ  TEST                                                 **/
/**********************************************************************/

class SimpleStorageServiceFileReadTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceFileReadTestWMS(SimpleStorageServicePerformanceTest *test,
                                                     const std::set<wrench::StorageService *> &storage_services,
                                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServicePerformanceTest *test;

    int main() {

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      double before_read = simulation->getCurrentSimulatedDate();
      try {
        this->test->storage_service_1->readFile(this->test->file_1);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      double elapsed = simulation->getCurrentSimulatedDate() - before_read;

      double effecive_bandwidth = (MBPS_BANDWIDTH * 1000 * 1000) * 0.92;

      double expected_elapsed = this->test->file_1->getSize() / effecive_bandwidth;

      if (std::abs(elapsed - expected_elapsed) > 1.0) {
        throw std::runtime_error("Incorrect file read time " + std::to_string(elapsed) + " (expected: " + std::to_string(expected_elapsed) + ")");
      }

      return 0;
    }
};

TEST_F(SimpleStorageServicePerformanceTest, FileRead) {
  DO_TEST_WITH_FORK(do_FileRead_test);
}

void SimpleStorageServicePerformanceTest::do_FileRead_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("performance_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create Two Storage Services
  ASSERT_NO_THROW(storage_service_1 = simulation->add(
          new wrench::SimpleStorageService("SrcHost", STORAGE_SIZE)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceFileReadTestWMS(
                  this, {storage_service_1},
                  "WMSHost")));

  wms->addWorkflow(this->workflow);

  // Create a file registry
  simulation->add(new wrench::FileRegistryService("WMSHost"));

  // Staging all files on the  storage service
  ASSERT_NO_THROW(simulation->stageFiles({{file_1->getID(), file_1}, {file_2->getID(), file_2}, {file_3->getID(), file_3}}, storage_service_1));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
