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
#include "../../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_storage_service_functional_test, "Log category for SimpleStorageServiceFunctionalTest");


class SimpleStorageServiceFunctionalTest : public ::testing::Test {

public:
    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_10;
    wrench::WorkflowFile *file_100;
    wrench::WorkflowFile *file_500;
    wrench::StorageService *storage_service_100 = nullptr;
    wrench::StorageService *storage_service_500 = nullptr;
    wrench::StorageService *storage_service_1000 = nullptr;

    wrench::ComputeService *compute_service = nullptr;

    void do_BasicFunctionality_test();

    void do_SynchronousFileCopy_test();

    void do_AsynchronousFileCopy_test();

    void do_SynchronousFileCopyFailures_test();

    void do_AsynchronousFileCopyFailures_test();
    
    void do_Partitions_test();


protected:
    SimpleStorageServiceFunctionalTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      file_1 = workflow->addFile("file_1", 1.0);
      file_10 = workflow->addFile("file_10", 10.0);
      file_100 = workflow->addFile("file_100", 100.0);
      file_500 = workflow->addFile("file_500", 500.0);

      // Create a one-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"SingleHost\" speed=\"1f\"/> "
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
/**  BASIC FUNCTIONALITY SIMULATION TEST                             **/
/**********************************************************************/

class SimpleStorageServiceBasicFunctionalityTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceBasicFunctionalityTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                  const std::set<wrench::ComputeService *> &compute_services,
                                                  const std::set<wrench::StorageService *> &storage_services,
                                                  wrench::FileRegistryService *file_registry_service,
                                                  std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, file_registry_service, hostname,
                        "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Bogus staging (can only be done in maestro)
      bool success = true;
      try {
        this->simulation->stageFile(this->test->file_1, this->test->storage_service_100);
      } catch (std::runtime_error &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be possible to call stageFile() from a non-maestro process");
      }

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      // Do a few lookups from the file registry service
      for (auto f : {this->test->file_1, this->test->file_10, this->test->file_100, this->test->file_500}) {
        std::set<wrench::StorageService *> result = file_registry_service->lookupEntry(f);
        if ((result.size() != 1) || (*(result.begin()) != this->test->storage_service_1000)) {
          throw std::runtime_error(
                  "File registry service should know that file " + f->getID() + " is on storage service " +
                  this->test->storage_service_1000->getName());
        }

      }

      // Do a bogus lookup
      success = true;
      try {
        this->test->storage_service_1000->lookupFile(nullptr, nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a nullptr file!");
      }

      // Do a few queries to storage services
      for (auto f : {this->test->file_1, this->test->file_10, this->test->file_100, this->test->file_500}) {
        if ((!this->test->storage_service_1000->lookupFile(f, nullptr)) ||
            (this->test->storage_service_100->lookupFile(f, nullptr)) ||
            (this->test->storage_service_500->lookupFile(f, nullptr))) {
          throw std::runtime_error("Some storage services do/don't have the files that they shouldn't/should have");
        }
      }

      // Do a couple of bogus copies
      success = true;
      try {
        this->test->storage_service_100->copyFile(nullptr, this->test->storage_service_1000);
      } catch (std::invalid_argument &) {
        success = false;
      }
      if (success) {
        throw std::runtime_error(
                "Should not be able to store a nullptr to a storage service!");
      }

      success = true;
      try {
        this->test->storage_service_100->copyFile(this->test->file_500, nullptr);
      } catch (std::invalid_argument &) {
        success = false;
      }
      if (success) {
        throw std::runtime_error(
                "Should not be able to store a file to a nullptr storage service!");
      }

      // Copy a file to a storage service that doesn't have enough space
      success = true;
      try {
        this->test->storage_service_100->copyFile(this->test->file_500, this->test->storage_service_1000);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error(
                "Should not be able to store a file to a storage service that doesn't have enough capacity");
      }


      // Make sure the copy didn't happen
      success = false;
      if (this->test->storage_service_100->lookupFile(this->test->file_500, nullptr)) {
        success = true;
      }
      if (success) {
        throw std::runtime_error("File copy to a storage service without enough space shouldn't have succeeded");
      }

      // Copy a file to a storage service that has enough space
      success = true;
      try {
        this->test->storage_service_100->copyFile(this->test->file_10, this->test->storage_service_1000);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
      }
      if (!success) {
        throw std::runtime_error("Should be able to store a file to a storage service that has enough capacity");
      }


      // TODO: Make sure that the file registry service has been updated?

      // Send a free space request
      double free_space;
      try {
        free_space = this->test->storage_service_100->getFreeSpace();
      } catch (wrench::WorkflowExecutionException &e) {
        free_space = -1.0;
      }
      if (free_space != 90.0) {
        throw std::runtime_error(
                "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 90.0");
      }

      // Read a file on a storage service
      try {
        this->test->storage_service_100->readFile(this->test->file_10, nullptr);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Should be able to read a file available on a storage service");
      }

      // Read a file on a storage service that doesn't have that file
      success = true;
      try {
        this->test->storage_service_100->readFile(this->test->file_100, nullptr);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to read a file unavailable a storage service");
      }

      // Delete a file on a storage service that doesnt' have it
      success = true;
      try {
        this->test->storage_service_100->deleteFile(this->test->file_100);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        if (e.getCause()->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
          throw std::runtime_error("Got an expected 'file not found' exception, but not the expected failure cause type");
        }
        auto real_cause = (wrench::FileNotFound *) e.getCause().get();
        if (real_cause->getStorageService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
        }
        if (real_cause->getFile() != this->test->file_100) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
        }

      }
      if (success) {
        throw std::runtime_error("Should not be able to delete a file unavailable a storage service");
      }

      // Delete a file in a bogus partition
      success = true;
      try {
        this->test->storage_service_100->deleteFile(this->test->file_100, "bogus");
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        if (e.getCause()->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
          throw std::runtime_error("Got an expected 'file not found' exception, but not the expected failure cause type");
        }
        auto real_cause = (wrench::FileNotFound *) e.getCause().get();
        if (real_cause->getStorageService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
        }
        if (real_cause->getFile() != this->test->file_100) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
        }

      }
      if (success) {
        throw std::runtime_error("Should not be able to delete a file unavailable a storage service");
      }



      // Delete a file on a storage service that has it
      try {
        this->test->storage_service_100->deleteFile(this->test->file_10);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Should  be able to delete a file available a storage service");
      }

      // Check that the storage capacity is back to what it should be
      try {
        free_space = this->test->storage_service_100->getFreeSpace();
      } catch (wrench::WorkflowExecutionException &e) {
        free_space = -1.0;
      }
      if (free_space != 100.0) {
        throw std::runtime_error(
                "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 100.0");
      }



      // Do a bogus asynchronous file copy (file = nullptr);
      success = true;
      try {
        data_movement_manager->initiateAsynchronousFileCopy(nullptr,
                                                            this->test->storage_service_1000,
                                                            this->test->storage_service_100);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to do an initiateAsynchronousFileCopy with a nullptr file");
      }
      // Do a bogus asynchronous file copy (src = nullptr);
      success = true;
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_1,
                                                            nullptr,
                                                            this->test->storage_service_100);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to do an initiateAsynchronousFileCopy with a nullptr src");
      }

      // Do a bogus asynchronous file copy (dst = nullptr);
      success = true;
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_1,
                                                            this->test->storage_service_1000,
                                                            nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to do an initiateAsynchronousFileCopy with a nullptr dst");
      }


      // Do a valid asynchronous file copy
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_1,
                                                            this->test->storage_service_1000,
                                                            this->test->storage_service_100);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while submitting a file copy operations");
      }


      // Wait for a workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          // success, do nothing
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Check that the copy has happened..
      if (!this->test->storage_service_100->lookupFile(this->test->file_1, nullptr)) {
        throw std::runtime_error("Asynchronous file copy operation didn't copy the file");
      }

      // Check that the free space has been updated at the destination
      try {
        free_space = this->test->storage_service_100->getFreeSpace();
      } catch (wrench::WorkflowExecutionException &e) {
        free_space = -1.0;
      }
      if (free_space != 99.0) {
        throw std::runtime_error(
                "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 99.0");
      }



      // Do an INVALID asynchronous file copy (file too big)
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500,
                                                            this->test->storage_service_1000,
                                                            this->test->storage_service_100);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while submitting a file copy operations");
      }

      // Wait for a workflow execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::STORAGE_NOT_ENOUGH_SPACE) {
            throw std::runtime_error("Should have gotten a 'out of space' failure cause");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Do an INVALID asynchronous file copy (file not there)
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500,
                                                            this->test->storage_service_100,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while submitting a file copy operations");
      }

      // Wait for a workflow execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
            throw std::runtime_error("Should have gotten a 'file not found' failure cause");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Do a really bogus file removal
      success = true;
      try {
        this->test->storage_service_100->deleteFile(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to delete a nullptr file from a storage service");
      }


      // Shutdown the service
      this->test->storage_service_100->stop();

      // Try to do stuff with a shutdown service
      success = true;
      try {
        this->test->storage_service_100->lookupFile(this->test->file_1);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file from a DOWN service");
      }

      success = true;
      try {
        this->test->storage_service_100->lookupFile(this->test->file_1, "/");
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file from a DOWN service");
      }

      success = true;
      try {
        this->test->storage_service_100->lookupFile(this->test->file_1, nullptr);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file from a DOWN service");
      }

      success = true;
      try {
        this->test->storage_service_100->readFile(this->test->file_1, nullptr);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file from a down service");
      }

      success = true;
      try {
        this->test->storage_service_100->writeFile(this->test->file_1, nullptr);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file from a DOWN service");
      }

      success = true;
      try {
        free_space = this->test->storage_service_100->getFreeSpace();
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to get free space info from a DOWN service");
      }

      return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, BasicFunctionality) {
  DO_TEST_WITH_FORK(do_BasicFunctionality_test);
}

void SimpleStorageServiceFunctionalTest::do_BasicFunctionality_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES,
                                                                        wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a bad Storage Service
  ASSERT_THROW(storage_service_100 = simulation->add(
          new wrench::SimpleStorageService(hostname, -100.0)), std::invalid_argument);

  // Create Three Storage Services
  ASSERT_NO_THROW(storage_service_100 = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));
  ASSERT_NO_THROW(storage_service_500 = simulation->add(
          new wrench::SimpleStorageService(hostname, 500.0)));
  ASSERT_NO_THROW(storage_service_1000 = simulation->add(
          new wrench::SimpleStorageService(hostname, 1000.0)));


  // Create a file registry
  wrench::FileRegistryService *file_registry_service =
          simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceBasicFunctionalityTestWMS(this,
                                                            {compute_service},
                                                            {
                                                                    storage_service_100, storage_service_500,
                                                                    storage_service_1000
                                                            },
                                                            file_registry_service, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // A bogus staging
  ASSERT_THROW(simulation->stageFile(nullptr, storage_service_100), std::invalid_argument);

  // Another bogus staging
  ASSERT_THROW(simulation->stageFile(file_500, storage_service_100), std::runtime_error);

  // Staging all files on the 1000 storage service
  ASSERT_NO_THROW(simulation->stageFiles({{file_1->getID(),   file_1},
                                          {file_10->getID(),  file_10},
                                          {file_100->getID(), file_100},
                                          {file_500->getID(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  SYNCHRONOUS FILE COPY TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceSynchronousFileCopyTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceSynchronousFileCopyTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                   const std::set<wrench::ComputeService *> &compute_services,
                                                   const std::set<wrench::StorageService *> &storage_services,
                                                   std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      bool success;
      // Do a bogus file copy (file = nullptr)
      success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(nullptr, this->test->storage_service_1000,
                                                     this->test->storage_service_500);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to do a synchronous file copy with a nullptr file");
      }

      // Do a bogus file copy (src = nullptr)
      success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, nullptr,
                                                     this->test->storage_service_500);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to do a synchronous file copy with a nullptr src");
      }

      // Do a bogus file copy (dst = nullptr)
      success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Shouldn't be able to do a synchronous file copy with a nullptr src");
      }

      // Do the file copy
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an exception while doing a synchronous file copy: " + std::string(e.what()));
      }

      // Do the file copy again, which should fail
      success = false;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        success = true;
      }
      if (!success) {
        throw std::runtime_error("Should not be able to write a file beyond the storage capacity");
      }

      return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, SynchronousFileCopy) {
  DO_TEST_WITH_FORK(do_SynchronousFileCopy_test);
}

void SimpleStorageServiceFunctionalTest::do_SynchronousFileCopy_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a  Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, 1, 0)},
                                                       {})));

  // Create 2 Storage Services
  ASSERT_NO_THROW(storage_service_1000 = simulation->add(
          new wrench::SimpleStorageService(hostname, 1000.0)));

  ASSERT_NO_THROW(storage_service_500 = simulation->add(
          new wrench::SimpleStorageService(hostname, 500.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceSynchronousFileCopyTestWMS(
                  this,
                  {compute_service},
                  {storage_service_1000, storage_service_500}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  simulation->add(new wrench::FileRegistryService(hostname));

  // Staging file_500 on the 1000-byte storage service
  ASSERT_NO_THROW(simulation->stageFiles({{file_500->getID(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  ASYNCHRONOUS FILE COPY TEST                                     **/
/**********************************************************************/

class SimpleStorageServiceAsynchronousFileCopyTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceAsynchronousFileCopyTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                    const std::set<wrench::ComputeService *> &compute_services,
                                                    const std::set<wrench::StorageService *> &storage_services,
                                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      // Initiate a file copy
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an exception while trying to initiate a file copy: " + std::string(e.what()));
      }

      // Initiate it again which should fail
      bool success = true;
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        if (e.getCause()->getCauseType() != wrench::FailureCause::FILE_ALREADY_BEING_COPIED) {
          throw std::runtime_error("Got expected exception, but unexpected failure cause " +
                                           e.getCause()->toString() +
                                           "(was expecting a 'file already being copied' failure cause");
        }
        auto real_cause = dynamic_cast<wrench::FileAlreadyBeingCopied*>(e.getCause().get());
        if (real_cause->getFile() != this->test->file_500) {
          throw std::runtime_error("Got expected failure cause, but failure cause does not point to the right file");
        }
        if (real_cause->getStorageService() != this->test->storage_service_500) {
          throw std::runtime_error("Got expected failure cause, but failure cause does not point to the right storage service");
        }
        if (real_cause->getPartition() != "/") {
          throw std::runtime_error("Got expected failure cause, but failure cause does not point to the right partition");
        }
        real_cause->toString(); // for coverage

      }
      if (success) {
        throw std::runtime_error("A duplicate asynchronous file copy should fail!");
      }

      // Wait for the next execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;

      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }



      return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, AsynchronousFileCopy) {
  DO_TEST_WITH_FORK(do_AsynchronousFileCopy_test);
}

void SimpleStorageServiceFunctionalTest::do_AsynchronousFileCopy_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES,
                                                                        wrench::ComputeService::ALL_RAM)}, {})));

  // Create 2 Storage Services
  ASSERT_NO_THROW(storage_service_1000 = simulation->add(
          new wrench::SimpleStorageService(hostname, 1000.0)));

  ASSERT_NO_THROW(storage_service_500 = simulation->add(
          new wrench::SimpleStorageService(hostname, 500.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceAsynchronousFileCopyTestWMS(
                  this, {compute_service}, {storage_service_1000, storage_service_500},
                  hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  simulation->add(new wrench::FileRegistryService(hostname));

  // Staging file_500 on the 1000-byte storage service
  ASSERT_NO_THROW(simulation->stageFiles({{file_500->getID(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  SYNCHRONOUS FILE COPY TEST WITH FAILURES                        **/
/**********************************************************************/

class SimpleStorageServiceSynchronousFileCopyFailuresTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceSynchronousFileCopyFailuresTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                           const std::set<wrench::ComputeService *> compute_services,
                                                           const std::set<wrench::StorageService *> &storage_services,
                                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      // Do the file copy while space doesn't fit
      bool success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_100);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::STORAGE_NOT_ENOUGH_SPACE) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()));
        }
        // Check Exception details
        wrench::StorageServiceNotEnoughSpace *real_cause = (wrench::StorageServiceNotEnoughSpace *) e.getCause().get();
        std::string error_msg = real_cause->toString();
        if (real_cause->getFile() != this->test->file_500) {
          throw std::runtime_error(
                  "Got the expected 'not enough space' exception, but the failure cause does not point to the correct file");
        }
        if (real_cause->getStorageService() != this->test->storage_service_100) {
          throw std::runtime_error(
                  "Got the expected 'not enough space' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should have gotten a 'not enough space' exception");
      }

      // Fo a file copy from myself
      success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_500,
                                                     this->test->storage_service_500);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should have gotten a 'can't copy from myself' exception");
      }



      // Do the file copy for a file that's not there

      // First delete the file (we used to have an "already there" error)
      this->test->storage_service_1000->deleteFile(this->test->file_500);

      success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_500,
                                                     this->test->storage_service_1000);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
          throw std::runtime_error("XXX Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()));
        }
        // Check Exception details
        wrench::FileNotFound *real_cause = (wrench::FileNotFound *) e.getCause().get();
        if (real_cause->getFile() != this->test->file_500) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
        }
        if (real_cause->getStorageService() != this->test->storage_service_500) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should have gotten a 'file not found' exception");
      }


      // Do the file copy from a dst storage service that's down
      this->test->storage_service_1000->stop();

      success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()));
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_1000) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should have gotten a 'service is down' exception");
      }


      // Do the file copy from a src storage service that's down
      this->test->storage_service_500->stop();

      success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()));
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_500) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should have gotten a 'service is down' exception");
      }

      return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, SynchronousFileCopyFailures) {
  DO_TEST_WITH_FORK(do_SynchronousFileCopyFailures_test);
}

void SimpleStorageServiceFunctionalTest::do_SynchronousFileCopyFailures_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES,
                                                                        wrench::ComputeService::ALL_RAM)}, {})));

  // Create 3 Storage Services
  ASSERT_NO_THROW(storage_service_1000 = simulation->add(
          new wrench::SimpleStorageService(hostname, 1000.0)));

  ASSERT_NO_THROW(storage_service_500 = simulation->add(
          new wrench::SimpleStorageService(hostname, 500.0)));

  ASSERT_NO_THROW(storage_service_100 = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0, {{"MAX_NUM_CONCURRENT_DATA_CONNECTIONS", "infinity"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceSynchronousFileCopyFailuresTestWMS(
                  this, {
                          compute_service
                  }, {
                          storage_service_100, storage_service_500,
                          storage_service_1000
                  }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  simulation->add(new wrench::FileRegistryService(hostname));

  // Staging file_500 on the 1000-byte storage service
  ASSERT_NO_THROW(simulation->stageFile(file_500, storage_service_1000));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}



/**********************************************************************/
/**  ASYNCHRONOUS FILE COPY TEST WITH FAILURES                        **/
/**********************************************************************/

class SimpleStorageServiceAsynchronousFileCopyFailuresTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceAsynchronousFileCopyFailuresTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                            const std::set<wrench::ComputeService *> compute_services,
                                                            const std::set<wrench::StorageService *> &storage_services,
                                                            std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      // Do the file copy while space doesn't fit
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                            this->test->storage_service_100);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;

      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::STORAGE_NOT_ENOUGH_SPACE) {
            throw std::runtime_error("Got an expected exception, but an incorrect failure cause type " +
                                     std::to_string(dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType()));
          }
          wrench::StorageServiceNotEnoughSpace *real_cause =
                  (wrench::StorageServiceNotEnoughSpace *) dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause.get();
          if (real_cause->getFile() != this->test->file_500) {
            throw std::runtime_error(
                    "Got the expected exception and failure type, but the failure cause doesn't point to the right file");
          }
          if (real_cause->getStorageService() != this->test->storage_service_100) {
            throw std::runtime_error(
                    "Got the expected exception and failure type, but the failure cause doesn't point to the right storage service");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Do the file copy for a file that's not there
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_100, this->test->storage_service_1000,
                                                            this->test->storage_service_100);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
            throw std::runtime_error("Got an expected exception, but an incorrect failure cause type " +
                                     std::to_string(dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType()));
          }
          wrench::FileNotFound *real_cause = (wrench::FileNotFound *) dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause.get();
          if (real_cause->getFile() != this->test->file_100) {
            throw std::runtime_error(
                    "Got the expected exception and failure type, but the failure cause doesn't point to the right file");
          }
          if (real_cause->getStorageService() != this->test->storage_service_1000) {
            throw std::runtime_error(
                    "Got the expected exception and failure type, but the failure cause doesn't point to the right storage service");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Do the file copy for a src storage service that's down
      this->test->storage_service_1000->stop();

      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_100, this->test->storage_service_1000,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
            throw std::runtime_error("Got an expected exception, but an incorrect failure cause type " +
                                     std::to_string(dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause->getCauseType()));
          }
          wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) dynamic_cast<wrench::FileCopyFailedEvent*>(event.get())->failure_cause.get();
          if (real_cause->getService() != this->test->storage_service_1000) {
            throw std::runtime_error(
                    "Got the expected exception and failure type, but the failure cause doesn't point to the right storage service");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Do the file copy from a dst storage service that's down
      this->test->storage_service_500->stop();

      bool success = true;
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()));
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != this->test->storage_service_500) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct storage service");
        }
      }
      if (success) {
        throw std::runtime_error("Should have gotten a 'service is down' exception");
      }

      return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, AsynchronousFileCopyFailures) {
  DO_TEST_WITH_FORK(do_AsynchronousFileCopyFailures_test);
}

void SimpleStorageServiceFunctionalTest::do_AsynchronousFileCopyFailures_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES,
                                                                        wrench::ComputeService::ALL_RAM)}, {})));

  // Create 3 Storage Services
  ASSERT_NO_THROW(storage_service_1000 = simulation->add(
          new wrench::SimpleStorageService(hostname, 1000.0)));

  ASSERT_NO_THROW(storage_service_500 = simulation->add(
          new wrench::SimpleStorageService(hostname, 500.0)));

  ASSERT_NO_THROW(storage_service_100 = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SimpleStorageServiceAsynchronousFileCopyFailuresTestWMS(
                  this, {
                          compute_service
                  }, {
                          storage_service_100, storage_service_500,
                          storage_service_1000
                  }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  simulation->add(new wrench::FileRegistryService(hostname));

  // Staging file_500 on the 1000-byte storage service
  ASSERT_NO_THROW(simulation->stageFiles({{file_500->getID(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}



/**********************************************************************/
/**  DIRECTORIES TEST                                                **/
/**********************************************************************/

class PartitionsTestWMS : public wrench::WMS {

public:
    PartitionsTestWMS(SimpleStorageServiceFunctionalTest *test,
                                                            const std::set<wrench::StorageService *> &storage_services,
                                                            std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      // Copy storage_service_1000:/:file_10 to storage_service_500:foo:file_10
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_10, this->test->storage_service_1000, "/",
                                                            this->test->storage_service_500, "foo");
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;

      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          // do nothing
          break;
        }
        default:
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
      }

      // Do a very similar copy, but with "empty" partitions that default to "/"
      // Copy storage_service_1000:/:file_10 to storage_service_500:/:file_10
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_10, this->test->storage_service_1000, "",
                                                            this->test->storage_service_500, "");
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          // do nothing
          break;
        }
        default:
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
      }

      // Remove the file at storage_service_500:/:file_10
      try {
        this->test->storage_service_500->deleteFile(this->test->file_10, "");
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Should be able to delete file storage_service_500:/:file_10");
      }



      // Copy storage_service_500:/:file_10 to storage_service_1000:foo:file_10: SHOULD NOT WORK
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_10, this->test->storage_service_500, "/",
                                                            this->test->storage_service_1000, "foo");
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          // do nothing
          break;
        }
        default:
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
      }

      // Copy storage_service_500:foo:file_10 to storage_service_1000:foo
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_10, this->test->storage_service_500, "foo",
                                                            this->test->storage_service_1000, "foo");
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          // do nothing
          break;
        }
        default:
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
      }


      // Copy storage_service_500:foo:file_10 to storage_service_500:bar
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_10, this->test->storage_service_500, "foo",
                                                            this->test->storage_service_500, "bar");
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          // do nothing
          break;
        }
        default:
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
      }



      // Copy storage_service_500:foo:file_10 to storage_service_500:foo    SHOULD NOT WORK
      bool success = false;
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_10, this->test->storage_service_500, "foo",
                                                            this->test->storage_service_500, "foo");
      } catch (std::invalid_argument &e) {
        success = false;
      }

      if (success) {
        throw std::runtime_error("Should not be able to copy a file onto itself");
      }

      // Check all lookups
      if (not this->test->storage_service_1000->lookupFile(this->test->file_10)) {
        throw std::runtime_error("File should be in storage_service_1000, partition '/'");
      }
      if (not this->test->storage_service_1000->lookupFile(this->test->file_10, "/")) {
        throw std::runtime_error("File should be in storage_service_1000, partition '/'");
      }
      if (not this->test->storage_service_1000->lookupFile(this->test->file_10, "")) {
        throw std::runtime_error("File should be in storage_service_1000, partition '/'");
      }
      if (not this->test->storage_service_500->lookupFile(this->test->file_10, "foo")) {
        throw std::runtime_error("File should be in storage_service_500, partition '/'");
      }
      if (not this->test->storage_service_1000->lookupFile(this->test->file_10, "foo")) {
        throw std::runtime_error("File should be in storage_service_1000, partition '/'");
      }


      // Bogus lookup
      success = true;
      try {
        this->test->storage_service_1000->lookupFile(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a nullptr file");
      }

      success = true;
      try {
        this->test->storage_service_1000->lookupFile(nullptr, "/");
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a nullptr file");
      }

      // File copy from oneself to oneself!
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_10, this->test->storage_service_500, "foo",
                                                            this->test->storage_service_500, "faa");
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          // do nothing
          break;
        }
        default:
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
      }

      return 0;
    }
};

TEST_F(SimpleStorageServiceFunctionalTest, Partitions) {
  DO_TEST_WITH_FORK(do_Partitions_test);
}

void SimpleStorageServiceFunctionalTest::do_Partitions_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("capacity_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create 2 Storage Services
  ASSERT_NO_THROW(storage_service_1000 = simulation->add(
          new wrench::SimpleStorageService(hostname, 1000.0)));

  ASSERT_NO_THROW(storage_service_500 = simulation->add(
          new wrench::SimpleStorageService(hostname, 500.0)));


  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new PartitionsTestWMS(
                  this, {
                          storage_service_1000, storage_service_500
                  }, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  simulation->add(new wrench::FileRegistryService(hostname));

  // Staging file_500 on the 1000-byte storage service
  ASSERT_NO_THROW(simulation->stageFiles({{file_10->getID(), file_10}}, storage_service_1000));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
