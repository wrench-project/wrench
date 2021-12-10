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

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(simple_storage_service_limited_connection_test, "Log category for SimpleStorageServiceLimitedConnectionTest");


#define NUM_PARALLEL_TRANSFERS 7 // Changing this number  to a non n*3+ 1 number will ikely
// require changing the test code
#define FILE_SIZE 10000000000.00
#define STORAGE_SIZE (100.0 * FILE_SIZE)

class SimpleStorageServiceLimitedConnectionsTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::DataFile> files[NUM_PARALLEL_TRANSFERS];
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_wms_unlimited = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_wms_limited = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_unlimited = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_limited = nullptr;

    void do_ConcurrencyFileCopies_test();


protected:

    ~SimpleStorageServiceLimitedConnectionsTest() {
        workflow->clear();
    }

    SimpleStorageServiceLimitedConnectionsTest() {

      // Create the simplest workflow
      workflow = wrench::Workflow::createWorkflow();

      // Create the files
      for (size_t i=0; i < NUM_PARALLEL_TRANSFERS; i++) {
        files[i] = workflow->addFile("file_"+std::to_string(i), FILE_SIZE);
      }

      // Create a 3-host platform file
      std::string xml = "<?xml version='1.0'?>"
                                "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                                "<platform version=\"4.1\"> "
                                "   <zone id=\"AS0\" routing=\"Full\"> "
                                "       <host id=\"Host1\" speed=\"1Gf\" > "
                                "          <disk id=\"large_disk\" read_bw=\"50MBps\" write_bw=\"50MBps\">"
                                "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "B\"/>"
                                "             <prop id=\"mount\" value=\"/\"/>"
                                "          </disk>"
                                "       </host>"
                                "       <host id=\"Host2\" speed=\"1Gf\" > "
                                "          <disk id=\"large_disk\" read_bw=\"50MBps\" write_bw=\"50MBps\">"
                                "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "B\"/>"
                                "             <prop id=\"mount\" value=\"/\"/>"
                                "          </disk>"
                                "       </host>"
                                "       <host id=\"WMSHost\" speed=\"1Gf\" > "
                                "          <disk id=\"large_disk1\" read_bw=\"50MBps\" write_bw=\"50MBps\">"
                                "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "B\"/>"
                                "             <prop id=\"mount\" value=\"/disk1\"/>"
                                "          </disk>"
                                "          <disk id=\"large_disk2\" read_bw=\"50MBps\" write_bw=\"50MBps\">"
                                "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "B\"/>"
                                "             <prop id=\"mount\" value=\"/disk2\"/>"
                                "          </disk>"
                                "       </host>"
                                "       <link id=\"loop\" bandwidth=\"1000000GBps\" latency=\"0us\"/>"
                                "       <link id=\"link1\" bandwidth=\"54MBps\" latency=\"0us\"/>"
                                "       <link id=\"link2\" bandwidth=\"54MBps\" latency=\"0us\"/>"
                                "       <route src=\"WMSHost\" dst=\"Host1\">"
                                "         <link_ctn id=\"link1\"/>"
                                "       </route>"
                                "       <route src=\"WMSHost\" dst=\"Host2\">"
                                "         <link_ctn id=\"link2\"/>"
                                "       </route>"
                                "       <route src=\"WMSHost\" dst=\"WMSHost\">"
                                "         <link_ctn id=\"loop\"/>"
                                "       </route>"
                                "       <route src=\"Host1\" dst=\"Host1\">"
                                "         <link_ctn id=\"loop\"/>"
                                "       </route>"
                                "       <route src=\"Host2\" dst=\"Host2\">"
                                "         <link_ctn id=\"loop\"/>"
                                "       </route>"
                                "   </zone> "
                                "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;
};


/**********************************************************************/
/**  CONCURRENT FILE COPIES TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS(
            SimpleStorageServiceLimitedConnectionsTest *test,
            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
            const std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceLimitedConnectionsTest *test;

    int main() {


      // Create a data movement manager
      auto data_movement_manager = this->createDataMovementManager();

      // Get a file registry
      auto file_registry_service = this->getAvailableFileRegistryService();

      // Reading
      for (auto dst_storage_service : {this->test->storage_service_unlimited, this->test->storage_service_limited}) {

        std::shared_ptr<wrench::StorageService> src_storage_service;
        if (dst_storage_service == this->test->storage_service_unlimited) {
          src_storage_service = this->test->storage_service_wms_unlimited;
        }
        if (dst_storage_service == this->test->storage_service_limited) {
          src_storage_service = this->test->storage_service_wms_limited;
        }

        double start = wrench::Simulation::getCurrentSimulatedDate();

        // Initiate asynchronous file copies to/from storage_service
        for (int i = 0; i < NUM_PARALLEL_TRANSFERS; i++) {
          data_movement_manager->initiateAsynchronousFileCopy(this->test->files[i],
                                                              wrench::FileLocation::LOCATION(src_storage_service),
                                                              wrench::FileLocation::LOCATION(dst_storage_service));
        }

        double elapsed[NUM_PARALLEL_TRANSFERS];
        for (int i = 0; i < NUM_PARALLEL_TRANSFERS; i++) {
          std::shared_ptr<wrench::ExecutionEvent> event1 = this->waitForNextEvent();
          if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(event1)) {
            throw std::runtime_error("Unexpected Workflow Execution Event: " + event1->toString());
          }
          elapsed[i] = wrench::Simulation::getCurrentSimulatedDate() - start;
        }

//          WRENCH_INFO("START = %lf", start);
//          for (int i = 0; i < NUM_PARALLEL_TRANSFERS; i++) {
//            WRENCH_INFO("--> elapsed : %lf", elapsed[i]);
//          }

        // Check results for the unlimited storage service
        if (dst_storage_service == this->test->storage_service_unlimited) {
          double baseline_elapsed = elapsed[0];
          for (int i=1; i < NUM_PARALLEL_TRANSFERS; i++) {
            if (std::abs(baseline_elapsed - elapsed[i]) > 1) {
              throw std::runtime_error("Incoherent transfer elapsed times for the unlimited storage service");
            }
          }
        }

        // Check results for the limited storage service
        if (dst_storage_service == this->test->storage_service_limited) {
          bool success = true;

          for (int i=0; i < NUM_PARALLEL_TRANSFERS; i+=3) {
            if (std::abs<double>(elapsed[i] - elapsed[i+1] > 1) > 1) {
              success = false;
            }
            if (std::abs<double>(elapsed[i+1] - elapsed[i+2] > 1) > 1) {
              success = false;
            }
            if ((i > 0) and (i < NUM_PARALLEL_TRANSFERS-1)) {
              if (std::abs<double>(elapsed[i] - 2.0 * elapsed[i/3-1]) > 1) {
                success = false;
              }
            }
          }
          if (std::abs<double>(elapsed[NUM_PARALLEL_TRANSFERS-1] - elapsed[NUM_PARALLEL_TRANSFERS-2] - (elapsed[0] / 3.0)) > 1) {
            success = false;
          }

          if (not success) {
            throw std::runtime_error("Incoherent transfer elapsed times for the limited storage service");
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
  auto simulation = wrench::Simulation::createSimulation();
  int argc = 1;
  char **argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("unit_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a (unused) Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BareMetalComputeService("WMSHost",
                                              {std::make_pair("WMSHost",
                                                              std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                              "",
                                              {})));

  // Create a Local storage service with unlimited connections
  ASSERT_NO_THROW(storage_service_wms_unlimited = simulation->add(
          new wrench::SimpleStorageService("WMSHost", {"/disk1"})));

  // Create a Local storage service with limited connections
  ASSERT_NO_THROW(storage_service_wms_limited = simulation->add(
          new wrench::SimpleStorageService("WMSHost", {"/disk2"},
                                           {{"MAX_NUM_CONCURRENT_DATA_CONNECTIONS", "3"}})));

  // Create a Storage service with unlimited connections
  ASSERT_NO_THROW(storage_service_unlimited = simulation->add(
          new wrench::SimpleStorageService("Host1", {"/"})));

  // Create a Storage Service limited to 3 connections
  ASSERT_NO_THROW(storage_service_limited = simulation->add(
          new wrench::SimpleStorageService("Host2", {"/"},
                                           {{"MAX_NUM_CONCURRENT_DATA_CONNECTIONS", "3"}})));

  // Create a WMS
  std::shared_ptr<wrench::WMS> wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceConcurrencyFileCopiesLimitedConnectionsTestWMS(
                  this, {compute_service}, {storage_service_wms_unlimited, storage_service_wms_limited, storage_service_unlimited, storage_service_limited},
                  "WMSHost")));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  simulation->add(new wrench::FileRegistryService("WMSHost"));

  // Staging all files on the WMS storage service
  for (int i=0; i < NUM_PARALLEL_TRANSFERS; i++) {
    ASSERT_NO_THROW(simulation->stageFile(files[i], storage_service_wms_unlimited));
    ASSERT_NO_THROW(simulation->stageFile(files[i], storage_service_wms_limited));
  }

  // Running a "run a single task1" simulation
  ASSERT_NO_THROW(simulation->launch());


  for (int i=0; i < argc; i++)
     free(argv[i]);
  free(argv);
}
