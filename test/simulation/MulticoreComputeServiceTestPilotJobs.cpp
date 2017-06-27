/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

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


class MulticoreComputeServiceTestPilotJobs : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::StorageService *storage_service = nullptr;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowFile *output_file4;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;

    wrench::ComputeService *compute_service = nullptr;

    void do_UnsupportedPilotJob_test();
    void do_OnePilotJobNoTimeoutWaitForExpiration_test();
    void do_OnePilotJobNoTimeoutCancel_test();
    void do_OnePilotJobNoTimeoutShutdownService_test();


protected:
    MulticoreComputeServiceTestPilotJobs() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      input_file = workflow->addFile("input_file", 10.0);
      output_file1 = workflow->addFile("output_file1", 10.0);

      // Create one task
      task1 = workflow->addTask("task_1_10s_1core",  10, 1);

      // Add file-task dependencies
      task1->addInputFile(input_file);

      task1->addOutputFile(output_file1);


      // Create a one-host dual-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"/> "
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
/**  UNSUPPORTED PILOT JOB                                           **/
/**********************************************************************/

class MulticoreComputeServicePilotJobUnsupportedTestWMS : public wrench::WMS {

public:
    MulticoreComputeServicePilotJobUnsupportedTestWMS(MulticoreComputeServiceTestPilotJobs *test,
                                                   wrench::Workflow *workflow,
                                                   std::unique_ptr<wrench::Scheduler> scheduler,
                                                   std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestPilotJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a pilot job
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 3600);

      // Submit a pilot job
      bool success = true;
      try {
        job_manager->submitJob(pilot_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        if (e.getCause()->getCauseType() != wrench::WorkflowExecutionFailureCause::JOB_TYPE_NOT_SUPPORTED) {
          throw std::runtime_error("Didn't get the expected exception");
        }
        success = false;
      }

      if (success) {
        throw std::runtime_error("Should not be able to submit a pilot job to a compute service that does not support them");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, UnsupportedPilotJob) {
  DO_TEST_WITH_FORK(do_UnsupportedPilotJob_test);
}

void MulticoreComputeServiceTestPilotJobs::do_UnsupportedPilotJob_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("unsupported_pilot_job_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServicePilotJobUnsupportedTestWMS(this, workflow,
                                                                                          std::unique_ptr<wrench::Scheduler>(
                          new wrench::RandomScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, false, storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));



  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}


/**********************************************************************/
/**  ONE PILOT JOB, NO TIMEOUT, WAIT FOR EXPIRATION                  **/
/**********************************************************************/

class MulticoreComputeServiceOnePilotJobNoTimeoutWaitForExpirationTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceOnePilotJobNoTimeoutWaitForExpirationTestWMS(MulticoreComputeServiceTestPilotJobs *test,
                                                         wrench::Workflow *workflow,
                                                         std::unique_ptr<wrench::Scheduler> scheduler,
                                                         std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestPilotJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a pilot job
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 3600);

      // Submit a pilot job
      try {
        job_manager->submitJob(pilot_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Unexpected exception: " + e.getCause()->toString());
      }

      // Wait for the pilot job start
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Create a 1-task standard job
      wrench::StandardJob *one_task_job = job_manager->createStandardJob({this->test->task1}, {}, {}, {}, {});

      // Submit the standard job for execution
      try {
        job_manager->submitJob(one_task_job, pilot_job->getComputeService());
      } catch (std::exception &e) {
        throw std::runtime_error("Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
      }

      // Wait for the standard job completion
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Check completion states and times
      if ((this->test->task1->getState() != wrench::WorkflowTask::COMPLETED)) {
        throw std::runtime_error("Unexpected task states");
      }

      // Wait for the pilot job expiration
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Terminate while the pilot job is still running
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, OnePilotJobNoTimeoutWaitForExpiration) {
  DO_TEST_WITH_FORK(do_OnePilotJobNoTimeoutWaitForExpiration_test);
}

void MulticoreComputeServiceTestPilotJobs::do_OnePilotJobNoTimeoutWaitForExpiration_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_pilot_job_no_timeout_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceOnePilotJobNoTimeoutWaitForExpirationTestWMS(this, workflow,
                                                                                                std::unique_ptr<wrench::Scheduler>(
                          new wrench::RandomScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service

  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, false, true, storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}


/**********************************************************************/
/**  ONE PILOT JOB, NO TIMEOUT, CANCEL PILOT JOB                     **/
/**********************************************************************/

class MulticoreComputeServiceOnePilotJobNoTimeoutCancelTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceOnePilotJobNoTimeoutCancelTestWMS(MulticoreComputeServiceTestPilotJobs *test,
                                                                        wrench::Workflow *workflow,
                                                                        std::unique_ptr<wrench::Scheduler> scheduler,
                                                                        std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestPilotJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a pilot job
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 3600);

      // Submit a pilot job
      try {
        job_manager->submitJob(pilot_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Unexpected exception: " + e.getCause()->toString());
      }

      // Wait for the pilot job start
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Create a 1-task standard job
      wrench::StandardJob *one_task_job = job_manager->createStandardJob({this->test->task1}, {}, {}, {}, {});

      // Submit the standard job for execution
      try {
        job_manager->submitJob(one_task_job, pilot_job->getComputeService());
      } catch (std::exception &e) {
        throw std::runtime_error("Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
      }

      // Wait for the standard job completion
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Check completion states and times
      if ((this->test->task1->getState() != wrench::WorkflowTask::COMPLETED)) {
        throw std::runtime_error("Unexpected task state");
      }

      // Cancel the pilot job before it expires
      try {
        job_manager->terminateJob(pilot_job);
      } catch (std::exception &e) {
        throw std::runtime_error("Unexpected exception while canceling pilot job: " + std::string(e.what()));
      }

      // Terminate while the pilot job is still running
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, DISABLED_OnePilotJobNoTimeoutCancel) {
  DO_TEST_WITH_FORK(do_OnePilotJobNoTimeoutCancel_test);
}

void MulticoreComputeServiceTestPilotJobs::do_OnePilotJobNoTimeoutCancel_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_pilot_job_no_timeout_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceOnePilotJobNoTimeoutCancelTestWMS(this, workflow,
                                                                                                               std::unique_ptr<wrench::Scheduler>(
                          new wrench::RandomScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service

  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, false, true, storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}


/**********************************************************************/
/**  ONE PILOT JOB, NO TIMEOUT, SHUTDOWN SERVICE                     **/
/**********************************************************************/

class MulticoreComputeServiceOnePilotJobNoTimeoutShutdownServiceTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceOnePilotJobNoTimeoutShutdownServiceTestWMS(MulticoreComputeServiceTestPilotJobs *test,
                                                             wrench::Workflow *workflow,
                                                             std::unique_ptr<wrench::Scheduler> scheduler,
                                                             std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    MulticoreComputeServiceTestPilotJobs *test;

    int main() {

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job  manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a pilot job
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 3600);

      // Submit a pilot job
      try {
        job_manager->submitJob(pilot_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Unexpected exception: " + e.getCause()->toString());
      }

      // Wait for the pilot job start
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Create a 1-task standard job
      wrench::StandardJob *one_task_job = job_manager->createStandardJob({this->test->task1}, {}, {}, {}, {});

      // Submit the standard job for execution
      try {
        job_manager->submitJob(one_task_job, pilot_job->getComputeService());
      } catch (std::exception &e) {
        throw std::runtime_error("Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
      }

      // Wait for the standard job completion
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Check completion states and times
      if ((this->test->task1->getState() != wrench::WorkflowTask::COMPLETED)) {
        throw std::runtime_error("Unexpected task state");
      }

      // Terminate while the pilot job is still running
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, OnePilotJobNoTimeoutShutdownService) {
  DO_TEST_WITH_FORK(do_OnePilotJobNoTimeoutShutdownService_test);
}

void MulticoreComputeServiceTestPilotJobs::do_OnePilotJobNoTimeoutShutdownService_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_pilot_job_no_timeout_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceOnePilotJobNoTimeoutShutdownServiceTestWMS(this, workflow,
                                                                                                    std::unique_ptr<wrench::Scheduler>(
                          new wrench::RandomScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service

  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, false, true, storage_service, {}))));

  // Create a file registry
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));


  // Staging the input file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}
