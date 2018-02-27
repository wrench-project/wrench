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
#include "../TestWithFork.h"


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
                                                  std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
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
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Do a few lookups from the file registry service
      for (auto f : {this->test->file_1, this->test->file_10, this->test->file_100, this->test->file_500}) {
        std::set<wrench::StorageService *> result = file_registry_service->lookupEntry(f);
        if ((result.size() != 1) || (*(result.begin()) != this->test->storage_service_1000)) {
          throw std::runtime_error(
                  "File registry service should know that file " + f->getId() + " is on storage service " +
                  this->test->storage_service_1000->getName());
        }

      }

      // Do a few queries to storage services
      for (auto f : {this->test->file_1, this->test->file_10, this->test->file_100, this->test->file_500}) {
        if ((!this->test->storage_service_1000->lookupFile(f)) ||
            (this->test->storage_service_100->lookupFile(f)) ||
            (this->test->storage_service_500->lookupFile(f))) {
          throw std::runtime_error("Some storage services do/don't have the files that they shouldn't/should have");
        }
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
      if (this->test->storage_service_100->lookupFile(this->test->file_500)) {
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
        free_space = this->test->storage_service_100->howMuchFreeSpace();
      } catch (wrench::WorkflowExecutionException &e) {
        free_space = -1.0;
      }
      if (free_space != 90.0) {
        throw std::runtime_error(
                "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 90.0");
      }

      // Read a file on a storage service
      try {
        this->test->storage_service_100->readFile(this->test->file_10);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Should be able to read a file available on a storage service");
      }

      // Read a file on a storage service that doesn't have that file
      success = true;
      try {
        this->test->storage_service_100->readFile(this->test->file_100);
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
        free_space = this->test->storage_service_100->howMuchFreeSpace();
      } catch (wrench::WorkflowExecutionException &e) {
        free_space = -1.0;
      }
      if (free_space != 100.0) {
        throw std::runtime_error(
                "Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 100.0");
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
        event = workflow->waitForNextExecutionEvent();
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
      if (!this->test->storage_service_100->lookupFile(this->test->file_1)) {
        throw std::runtime_error("Asynchronous file copy operation didn't copy the file");
      }

      // Check that the free space has been updated at the destination
      try {
        free_space = this->test->storage_service_100->howMuchFreeSpace();
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
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::STORAGE_NO_ENOUGH_SPACE) {
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
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
            throw std::runtime_error("Should have gotten a 'file not found' failure cause");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      // Do a bogus file removal
      success = true;
      try {
        this->test->storage_service_100->deleteFile(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to delete a nullptr file from a storage service");
      }

      // Terminate
      this->shutdownAllServices();
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

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr, {}))));
  // Create a bad Storage Service
  EXPECT_THROW(storage_service_100 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, -100.0))), std::invalid_argument);

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

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(
                  new SimpleStorageServiceBasicFunctionalityTestWMS(this,
                                                                    {compute_service},
                          {
                            storage_service_100, storage_service_500,
                                    storage_service_1000
                          }, hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // A bogus staging
  EXPECT_THROW(simulation->stageFile(nullptr, storage_service_100), std::invalid_argument);

  // Another bogus staging
  EXPECT_THROW(simulation->stageFile(file_500, storage_service_100), std::runtime_error);

  // Staging all files on the 1000 storage service
  EXPECT_NO_THROW(simulation->stageFiles({{file_1->getId(), file_1},
                                          {file_10->getId(), file_10},
                                          {file_100->getId(), file_100},
                                          {file_500->getId(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

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
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Do the file copy
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an exception while doing a synchronous file copy: " + std::string(e.what()));
      }

      // Do the file copy again, which should fail
      bool success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
      }
      if (!success) {
        throw std::runtime_error("Should be able fo write a file that's already there");
      }

      this->shutdownAllServices();
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

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a  Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, 1, 0)},
                                                               nullptr, {}))));

  // Create 2 Storage Services
  EXPECT_NO_THROW(storage_service_1000 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 1000.0))));

  EXPECT_NO_THROW(storage_service_500 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 500.0))));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new SimpleStorageServiceSynchronousFileCopyTestWMS(
                  this,
                  {compute_service},
                          { storage_service_1000, storage_service_500 }, hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging file_500 on the 1000-byte storage service
  EXPECT_NO_THROW(simulation->stageFiles({{file_500->getId(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

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
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Initiate a file copy
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an exception while trying to initiate a file copy: " + std::string(e.what()));
      }

      // Wait for the next execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;

      try {
        event = workflow->waitForNextExecutionEvent();
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

      // Do it all again, which should fail
      // Initiate a file copy
      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an exception while trying to initiate a file copy: " + std::string(e.what()));
      }

      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }


      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
//          if (event->failure_cause->getCauseType() != wrench::FailureCause::FILE_ALREADY_THERE) {
//            throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
//                                     std::to_string(event->failure_cause->getCauseType()));
//          }
//          wrench::StorageServiceFileAlreadyThere *real_cause = (wrench::StorageServiceFileAlreadyThere *) event->failure_cause.get();
//          if (real_cause->getFile() != this->test->file_500) {
//            throw std::runtime_error(
//                    "Got the expected 'file already there' exception, but the failure cause does not point to the correct file");
//          }
//          if (real_cause->getStorageService() != this->test->storage_service_500) {
//            throw std::runtime_error(
//                    "Got the expected 'file already there' exception, but the failure cause does not point to the correct storage service");
//          }

          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
        }
      }

      this->shutdownAllServices();
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

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr, {}))));

  // Create 2 Storage Services
  EXPECT_NO_THROW(storage_service_1000 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 1000.0))));

  EXPECT_NO_THROW(storage_service_500 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 500.0))));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new SimpleStorageServiceAsynchronousFileCopyTestWMS(
                  this,  {compute_service}, {storage_service_1000, storage_service_500},
                          hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging file_500 on the 1000-byte storage service
  EXPECT_NO_THROW(simulation->stageFiles({{file_500->getId(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

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
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Do the file copy while space doesn't fit
      bool success = true;
      try {
        data_movement_manager->doSynchronousFileCopy(this->test->file_500, this->test->storage_service_1000,
                                                     this->test->storage_service_100);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::STORAGE_NO_ENOUGH_SPACE) {
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

      // First delete the file to avoid the (already there error)
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
      this->simulation->getTerminator()->shutdownStorageService(this->test->storage_service_1000);

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
      this->simulation->getTerminator()->shutdownStorageService(this->test->storage_service_500);

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

      // since storage services have been manually stopped, we manually stop all other services
      this->simulation->getTerminator()->shutdownComputeService(this->compute_services);
      this->simulation->getTerminator()->shutdownStorageService(this->test->storage_service_100);
      this->simulation->getTerminator()->shutdownFileRegistryService(this->simulation->getFileRegistryService());
      this->simulation->getTerminator()->shutdownNetworkProximityService(
              this->simulation->getNetworkProximityService());
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

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr, {}))));

  // Create 3 Storage Services
  EXPECT_NO_THROW(storage_service_1000 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 1000.0))));

  EXPECT_NO_THROW(storage_service_500 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 500.0))));

  EXPECT_NO_THROW(storage_service_100 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0, {{"MAX_NUM_CONCURRENT_DATA_CONNECTIONS", "infinity"}}))));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new SimpleStorageServiceSynchronousFileCopyFailuresTestWMS(
                  this,  {
                          compute_service
                  }, {
                          storage_service_100, storage_service_500,
                          storage_service_1000
                  }, hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging file_500 on the 1000-byte storage service
  EXPECT_NO_THROW(simulation->stageFile(file_500, storage_service_1000));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

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
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    SimpleStorageServiceFunctionalTest *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

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
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::STORAGE_NO_ENOUGH_SPACE) {
            throw std::runtime_error("Got an expected exception, but an incorrect failure cause type " +
                                     std::to_string(event->failure_cause->getCauseType()));
          }
          wrench::StorageServiceNotEnoughSpace *real_cause = (wrench::StorageServiceNotEnoughSpace *) event->failure_cause.get();
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
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
            throw std::runtime_error("Got an expected exception, but an incorrect failure cause type " +
                                     std::to_string(event->failure_cause->getCauseType()));
          }
          wrench::FileNotFound *real_cause = (wrench::FileNotFound *) event->failure_cause.get();
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
      this->simulation->getTerminator()->shutdownStorageService(this->test->storage_service_1000);

      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->file_100, this->test->storage_service_1000,
                                                            this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an unexpected exception");
      }

      // Wait for the next execution event
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
            throw std::runtime_error("Got an expected exception, but an incorrect failure cause type " +
                                     std::to_string(event->failure_cause->getCauseType()));
          }
          wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) event->failure_cause.get();
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
      this->simulation->getTerminator()->shutdownStorageService(this->test->storage_service_500);

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

      // since storage services have been manually stopped, we manually stop all other services
      this->simulation->getTerminator()->shutdownComputeService(this->compute_services);
      this->simulation->getTerminator()->shutdownStorageService(this->test->storage_service_100);
      this->simulation->getTerminator()->shutdownFileRegistryService(this->simulation->getFileRegistryService());
      this->simulation->getTerminator()->shutdownNetworkProximityService(
              this->simulation->getNetworkProximityService());
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

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr, {}))));

  // Create 3 Storage Services
  EXPECT_NO_THROW(storage_service_1000 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 1000.0))));

  EXPECT_NO_THROW(storage_service_500 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 500.0))));

  EXPECT_NO_THROW(storage_service_100 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new SimpleStorageServiceAsynchronousFileCopyFailuresTestWMS(
                  this,  {
                          compute_service
                  }, {
                          storage_service_100, storage_service_500,
                          storage_service_1000
                  }, hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging file_500 on the 1000-byte storage service
  EXPECT_NO_THROW(simulation->stageFiles({{file_500->getId(), file_500}}, storage_service_1000));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
