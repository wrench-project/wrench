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

// Convenient macro to launch a test inside a separate process
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
    wrench::StorageService *storage_service_100 = nullptr;
    wrench::StorageService *storage_service_500 = nullptr;
    wrench::StorageService *storage_service_1000 = nullptr;

    wrench::ComputeService *compute_service = nullptr;

    void do_BasicFunctionality_test();


protected:
    SimpleStorageServiceTest() {

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
/**  BASIC FUNCTIONALITY SIMULATION TEST                             **/
/**********************************************************************/

class SimpleStorageServiceBasicFunctionalityTestWMS : public wrench::WMS {

public:
    SimpleStorageServiceBasicFunctionalityTestWMS(SimpleStorageServiceTest *test,
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

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();


      // Do a few lookups from the file registry service
      for (auto f : {this->test->file_1, this->test->file_10, this->test->file_100, this->test->file_500}) {
        if (file_registry_service->lookupEntry(f) !=
            std::set<wrench::StorageService *>({this->test->storage_service_1000})) {
          throw std::runtime_error("File registry service should that that file " + f->getId() + " is on storage service " + this->test->storage_service_1000->getName());
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
      bool success = true;
      try {
        this->test->storage_service_100->copyFile(this->test->file_500, this->test->storage_service_1000);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to store a file to a storage service that doesn't have enough capacity");
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
        throw std::runtime_error("Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 90.0");
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
        throw std::runtime_error("Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 100.0");
      }


      // Do a valid asynchronous file copy
      try {
        data_movement_manager->submitFileCopy(this->test->file_1,
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
        throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          // success, do nothing
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int)(event->type)));
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
        throw std::runtime_error("Free space on storage service is wrong (" + std::to_string(free_space) + ") instead of 99.0");
      }

      // Do an INVALID asynchronous file copy (file too big)
      try {
        data_movement_manager->submitFileCopy(this->test->file_500,
                                              this->test->storage_service_1000,
                                              this->test->storage_service_100);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while submitting a file copy operations");
      }

      // Wait for a workflow execution event
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::WorkflowExecutionFailureCause::STORAGE_NO_ENOUGH_SPACE) {
            throw std::runtime_error("Should have gotten a 'out of space' failure cause");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int)(event->type)));
        }
      }

      // Do an INVALID asynchronous file copy (file not there)
      try {
        data_movement_manager->submitFileCopy(this->test->file_500,
                                              this->test->storage_service_100,
                                              this->test->storage_service_500);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while submitting a file copy operations");
      }

      // Wait for a workflow execution event
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
      }

      switch (event->type) {
        case wrench::WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::WorkflowExecutionFailureCause::FILE_NOT_FOUND) {
            throw std::runtime_error("Should have gotten a 'file not found' failure cause");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int)(event->type)));
        }
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(SimpleStorageServiceTest, BasicFunctionality) {
  DO_TEST_WITH_FORK(do_BasicFunctionality_test);
}

void SimpleStorageServiceTest::do_BasicFunctionality_test() {

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

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new SimpleStorageServiceBasicFunctionalityTestWMS(this, workflow,
                                                                     std::unique_ptr<wrench::Scheduler>(
                          new wrench::RandomScheduler()), hostname))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true, nullptr, {}))));

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
