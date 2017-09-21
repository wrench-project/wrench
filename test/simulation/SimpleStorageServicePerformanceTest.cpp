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

#include "NoopScheduler.h"
#include "TestWithFork.h"


#define FILE_SIZE 10000000000.00
#define STORAGE_SIZE (100.0 * FILE_SIZE)

class SimpleStorageServicePerformanceTest : public ::testing::Test {

public:
    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_2;
    wrench::WorkflowFile *file_3;
    wrench::StorageService *storage_service_1 = nullptr;
    wrench::StorageService *storage_service_2 = nullptr;

    wrench::ComputeService *compute_service = nullptr;

    void do_ConcurrencyFileCopies_test();


protected:
    SimpleStorageServicePerformanceTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      file_1 = workflow->addFile("file_1", FILE_SIZE);
      file_2 = workflow->addFile("file_2", FILE_SIZE);
      file_3 = workflow->addFile("file_3", FILE_SIZE);

      // Create a 3-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"SrcHost\" speed=\"1f\"/> "
              "       <host id=\"DstHost\" speed=\"1f\"/> "
              "       <host id=\"WMSHost\" speed=\"1f\"/> "
              "       <link id=\"link\" bandwidth=\"10MBps\" latency=\"100us\"/>"
              "       <route src=\"SrcHost\" dst=\"DstHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"SrcHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"DstHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
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
/**  CONCURRENT FILE COPIES TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceConcurrencyFileCopiesTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceConcurrencyFileCopiesTestWMS(SimpleStorageServicePerformanceTest *test,
                                                     wrench::Workflow *workflow,
                                                     std::unique_ptr<wrench::Scheduler> scheduler,
                                                     std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServicePerformanceTest *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();


      // Time the time it takes to transfer a file from Src to Dst
      double copy1_start = this->simulation->getCurrentSimulatedDate();
      data_movement_manager->initiateAsynchronousFileCopy(this->test->file_1,
                                                          this->test->storage_service_1,
                                                          this->test->storage_service_2);

      std::unique_ptr<wrench::WorkflowExecutionEvent> event1 = workflow->waitForNextExecutionEvent();
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


      std::unique_ptr<wrench::WorkflowExecutionEvent> event2 = workflow->waitForNextExecutionEvent();
      double event2_arrival = this->simulation->getCurrentSimulatedDate();

      std::unique_ptr<wrench::WorkflowExecutionEvent> event3 = workflow->waitForNextExecutionEvent();
      double event3_arrival = this->simulation->getCurrentSimulatedDate();

      double transfer_time_1 = event1_arrival - copy1_start;
      double transfer_time_2 = event2_arrival - copy2_start;
      double transfer_time_3 = event3_arrival - copy3_start;

      if (fabs(copy2_start - copy3_start) > 0.1) {
        throw std::runtime_error("Time between two asynchronous operations is too big");
      }

      if ((fabs(transfer_time_2 / transfer_time_1 - 2.0) > 0.001) ||
          (fabs(transfer_time_3 / transfer_time_1 - 2.0) > 0.001)) {
        throw std::runtime_error("Concurrent transfers should be roughly twice slower");
      }

      if (fabs(event2_arrival - event3_arrival) > 0.1) {
        throw std::runtime_error("Time between two asynchronous operation completions is too big");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(SimpleStorageServicePerformanceTest, ConcurrencyFileCopies) {
  DO_TEST_WITH_FORK(do_ConcurrencyFileCopies_test);
}

void SimpleStorageServicePerformanceTest::do_ConcurrencyFileCopies_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("performance_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new SimpleStorageServiceConcurrencyFileCopiesTestWMS(this, workflow,
                                                                                            std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), "WMSHost"))));

  // Create a (unused) Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService("WMSHost", true, true, nullptr, {}))));

  // Create Two Storage Services
  EXPECT_NO_THROW(storage_service_1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService("SrcHost", STORAGE_SIZE))));
  EXPECT_NO_THROW(storage_service_2 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService("DstHost", STORAGE_SIZE))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService("WMSHost"));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging all files on the Src storage service
  EXPECT_NO_THROW(simulation->stageFiles({file_1, file_2, file_3}, storage_service_1));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}


