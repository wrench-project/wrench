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

#include "wrench/workflow/job/PilotJob.h"
#include "NoopScheduler.h"
#include "TestWithFork.h"


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

    void do_UnsupportedPilotJobs_test();

    void do_OnePilotJobNoTimeoutWaitForExpiration_test();

    void do_OnePilotJobNoTimeoutShutdownService_test();

    void do_NonSubmittedPilotJobTermination_test();

    void do_IdlePilotJobTermination_test();

    void do_NonIdlePilotJobTermination_test();


protected:
    MulticoreComputeServiceTestPilotJobs() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      input_file = workflow->addFile("input_file", 10.0);
      output_file1 = workflow->addFile("output_file1", 10.0);

      // Create one task
      task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0);

      // Add file-task dependencies
      task1->addInputFile(input_file);

      task1->addOutputFile(output_file1);


      // Create a one-host dual-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
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

class MulticoreComputeServiceUnsupportedPilotJobsTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceUnsupportedPilotJobsTestWMS(MulticoreComputeServiceTestPilotJobs *test,
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
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 1, 3600);

      // Submit a pilot job
      bool success = true;
      try {
        job_manager->submitJob(pilot_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        if (e.getCause()->getCauseType() != wrench::FailureCause::JOB_TYPE_NOT_SUPPORTED) {
          throw std::runtime_error("Didn't get the expected exception");
        }
        success = false;
      }

      if (success) {
        throw std::runtime_error(
                "Should not be able to submit a pilot job to a compute service that does not support them");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, UnsupportedPilotJobs) {
  DO_TEST_WITH_FORK(do_UnsupportedPilotJobs_test);
}

void MulticoreComputeServiceTestPilotJobs::do_UnsupportedPilotJobs_test() {

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
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceUnsupportedPilotJobsTestWMS(this, workflow,
                                                                                              std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, false,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

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
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 1, 3600);

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
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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
        throw std::runtime_error(
                "Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
      }

      // Wait for the standard job completion
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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

      // Terminate
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
          std::unique_ptr<wrench::WMS>(
                  new MulticoreComputeServiceOnePilotJobNoTimeoutWaitForExpirationTestWMS(this, workflow,
                                                                                          std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, false, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

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
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 1, 3600);

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
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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
        throw std::runtime_error(
                "Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
      }

      // Wait for the standard job completion
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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
          std::unique_ptr<wrench::WMS>(
                  new MulticoreComputeServiceOnePilotJobNoTimeoutShutdownServiceTestWMS(this, workflow,
                                                                                        std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service

  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, false, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

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
/**  TERMINATE NON-SUBMITTED PILOT JOB                               **/
/**********************************************************************/

class MulticoreComputeServiceNonSubmittedPilotJobTerminationTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceNonSubmittedPilotJobTerminationTestWMS(MulticoreComputeServiceTestPilotJobs *test,
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
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 1, 3600);

      // Try to terminate it right now, which is stupid
//      bool success = true;
      try {
        job_manager->terminateJob(pilot_job);
      } catch (wrench::WorkflowExecutionException &e) {
//        success = false;
        if (e.getCause()->getCauseType() != wrench::FailureCause::JOB_CANNOT_BE_TERMINATED) {
          throw std::runtime_error(
                  "Got an exception, as expected, but it does not have the correct failure cause type");
        }
        wrench::JobCannotBeTerminated *real_cause = (wrench::JobCannotBeTerminated *) e.getCause().get();
        if (real_cause->getJob() != pilot_job) {
          throw std::runtime_error(
                  "Got the expected exception and failure cause, but the failure cause does not point to the right job");
        }
      }

      // Terminate everything
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, NonSubmittedPilotJobTermination) {
  DO_TEST_WITH_FORK(do_NonSubmittedPilotJobTermination_test);
}

void MulticoreComputeServiceTestPilotJobs::do_NonSubmittedPilotJobTermination_test() {

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
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceNonSubmittedPilotJobTerminationTestWMS(this, workflow,
                                                                                                         std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service

  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, false, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

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
/**  TERMINATE IDLE PILOT JOB                                        **/
/**********************************************************************/

class MulticoreComputeServiceIdlePilotJobTerminationTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceIdlePilotJobTerminationTestWMS(MulticoreComputeServiceTestPilotJobs *test,
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
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 1, 3600);

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
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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
        throw std::runtime_error(
                "Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
      }

      // Wait for the standard job completion
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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

      // Terminate the pilot job before it expires
      try {
        job_manager->terminateJob(pilot_job);
      } catch (std::exception &e) {
        throw std::runtime_error("Unexpected exception while terminating pilot job: " + std::string(e.what()));
      }

      // Terminate while the pilot job is still running
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, IdlePilotJobTermination) {
  DO_TEST_WITH_FORK(do_IdlePilotJobTermination_test);
}

void MulticoreComputeServiceTestPilotJobs::do_IdlePilotJobTermination_test() {

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
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceIdlePilotJobTerminationTestWMS(this, workflow,
                                                                                                 std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service

  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, false, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

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
/**  TERMINATE NON-IDLE PILOT JOB                                    **/
/**********************************************************************/

class MulticoreComputeServiceNonIdlePilotJobTerminationTestWMS : public wrench::WMS {

public:
    MulticoreComputeServiceNonIdlePilotJobTerminationTestWMS(MulticoreComputeServiceTestPilotJobs *test,
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
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(this->workflow, 1, 1, 3600);

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
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
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
        throw std::runtime_error(
                "Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
      }

      // Terminate the pilot job while it's running a standard job
      try {
        job_manager->terminateJob(pilot_job);
      } catch (std::exception &e) {
        throw std::runtime_error("Unexpected exception while terminating pilot job: " + std::string(e.what()));
      }

      // Wait for the standard job failure notification
      try {
        event = workflow->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(
                "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
      }
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
          if (event->failure_cause->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
            throw std::runtime_error("Got a job failure event, but the failure cause seems wrong");
          }
          wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) (event->failure_cause.get());
          if (real_cause->getService() != this->test->compute_service) {
            std::runtime_error(
                    "Got the correct failure even, a correct cause type, but the cause points to the wrong service");
          }
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Check completion states and times
      if ((this->test->task1->getState() != wrench::WorkflowTask::READY)) {
        throw std::runtime_error("Unexpected task state");
      }

      // Terminate while the pilot job is still running
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(MulticoreComputeServiceTestPilotJobs, NonIdlePilotJobTermination) {
  DO_TEST_WITH_FORK(do_NonIdlePilotJobTermination_test);
}

void MulticoreComputeServiceTestPilotJobs::do_NonIdlePilotJobTermination_test() {

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
          std::unique_ptr<wrench::WMS>(new MulticoreComputeServiceNonIdlePilotJobTerminationTestWMS(this, workflow,
                                                                                                    std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create A Storage Services
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Compute Service

  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, false, true,
                                                      {std::pair<std::string, unsigned long>(hostname,0)},
                                                      storage_service, {}))));

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

