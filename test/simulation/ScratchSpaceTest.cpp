/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchService.h>
#include <wrench/services/compute/batch/BatchServiceMessage.h>
#include "wrench/workflow/job/PilotJob.h"

#include "../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(scratch_service_test, "Log category for ScratchServiceTest");


class ScratchSpaceTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::ComputeService *compute_service1 = nullptr;
    wrench::ComputeService *compute_service2 = nullptr;
    wrench::Simulation *simulation;

    void do_SimpleScratchSpace_test();

    void do_ScratchSpaceFailure_test();

    void do_PilotJobScratchSpace_test();

protected:
    ScratchSpaceTest() {

      // Create the simplest workflow
      workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

      // Create a four-host 10-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
              "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
              "       <link id=\"3\" bandwidth=\"0.0001MBps\" latency=\"1000000us\"/>"
              "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host4\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
              "/> </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**                     SIMPLE SCRATCH SPACE TEST                    **/
/**********************************************************************/

class SimpleScratchSpaceTestWMS : public wrench::WMS {

public:
    SimpleScratchSpaceTestWMS(ScratchSpaceTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    ScratchSpaceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 1 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 1, 1, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));

        // Create a StandardJob with some pre-copies
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {},
                {std::make_tuple(this->workflow->getFileById("input_file"), this->test->storage_service1, wrench::ComputeService::SCRATCH)},
                {},
                {});

        // Submit the job for execution
        job_manager->submitJob(job, this->test->compute_service);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            if (this->test->compute_service->getFreeScratchSpaceSize() < 10000000000000.0) {
              throw std::runtime_error("ScratchSpaceTest::do_SimpleScratchSpace_test():File was not deleted from scratch" );
            }
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }
      }

      return 0;
    }
};

TEST_F(ScratchSpaceTest, SimpleScratchSpaceTest) {
  DO_TEST_WITH_FORK(do_SimpleScratchSpace_test);
}


void ScratchSpaceTest::do_SimpleScratchSpace_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("scratch_space_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {}, 10000000000000.0)));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new SimpleScratchSpaceTestWMS(
                  this,  {compute_service}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
  wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**                  SIMPLE SCRATCH SPACE FAILURE TEST               **/
/**********************************************************************/

class SimpleScratchSpaceFailureTestWMS : public wrench::WMS {

public:
    SimpleScratchSpaceFailureTestWMS(ScratchSpaceTest *test,
                              const std::set<wrench::ComputeService *> &compute_services,
                              std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    ScratchSpaceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 1 cores
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 60, 1, 1, 1.0);
        task1->addInputFile(this->workflow->getFileById("input_file1"));

        // Create a sequential task that lasts one min and requires 1 cores
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 60, 1, 1, 1.0);
        task2->addInputFile(this->workflow->getFileById("input_file2"));

        // Create a StandardJob with SOME pre-copies from public storage to scratch
        wrench::StandardJob *job1 = job_manager->createStandardJob(
                {task1},
                {},
                {std::make_tuple(this->workflow->getFileById("input_file1"), this->test->storage_service1, wrench::ComputeService::SCRATCH)},
                {},
                {});

        // Create a StandardJob with NO pre-copies from public storage to scrach
        wrench::StandardJob *job2 = job_manager->createStandardJob(
                {task2},
                {},
                {std::make_tuple(this->workflow->getFileById("input_file2"), this->test->storage_service1, wrench::ComputeService::SCRATCH)},
                {},
                {});

        // Submit the job for execution to the compute service with NO scratch, thus expecting an exception
        job_manager->submitJob(job1, this->test->compute_service);

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }

        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
            if (dynamic_cast<wrench::StandardJobFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::NO_SCRATCH_SPACE) {
              throw std::runtime_error("Got a job failure event, but the failure cause seems wrong");
            }
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }

        // Submit the job for execution to the compute service which has some scratch, but not enough space, thus expecting an exception
        job_manager->submitJob(job1, this->test->compute_service1);
        // Wait for a workflow execution event
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
            if (dynamic_cast<wrench::StandardJobFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::STORAGE_NOT_ENOUGH_SPACE) {
              throw std::runtime_error("Got a job failure event, but the failure cause seems wrong");
            }
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }


        // Submit two jobs for execution to the same compute service which has just enough scratch for only one job and so
        // one should succeed and the second one should fail
        job_manager->submitJob(job1, this->test->compute_service2);
        wrench::S4U_Simulation::sleep(1);
        job_manager->submitJob(job2, this->test->compute_service2);
        // Wait for a workflow execution event
        int num_events = 0;
        int prev_event = -1;
        int i = 0;
        while (i<2) {
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              if (prev_event == -1) {
                prev_event = 0;
                num_events++;
              } else if (prev_event == 1) {
                num_events++;
              }
              break;
            }
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
              if (prev_event == -1) {
                prev_event = 1;
                num_events++;
              } else if (prev_event == 0) {
                num_events++;
              }
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event or here: " + std::to_string((int) (event->type)));
            }
          }
          i++;
        }

        if (num_events != 2) {
          throw std::runtime_error("Did not get enough expected execution events");
        }
      }

      return 0;
    }
};

TEST_F(ScratchSpaceTest, SimpleScratchSpaceFailureTest) {
  DO_TEST_WITH_FORK(do_ScratchSpaceFailure_test);
}


void ScratchSpaceTest::do_ScratchSpaceFailure_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("scratch_space_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service that does not have scratch space
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a Compute Service that has smaller scratch space than the files to be stored
  EXPECT_NO_THROW(compute_service1 = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {}, 100.0)));

  // Create a Compute Service that has enough scratch space to store the files
  EXPECT_NO_THROW(compute_service2 = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {}, 10000.0)));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new SimpleScratchSpaceFailureTestWMS(
                  this,  {compute_service}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


  // Create two workflow files
  wrench::WorkflowFile *input_file1 = this->workflow->addFile("input_file1", 10000.0);
  wrench::WorkflowFile *input_file2 = this->workflow->addFile("input_file2", 10000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFile(input_file1, storage_service1));
  EXPECT_NO_THROW(simulation->stageFile(input_file2, storage_service1));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**             SCRATCH SPACE FAILURE TEST FOR PILOT JOBS            **/
/**********************************************************************/

class PilotJobScratchSpaceTestWMS : public wrench::WMS {

public:
    PilotJobScratchSpaceTestWMS(ScratchSpaceTest *test,
                                     const std::set<wrench::ComputeService *> &compute_services,
                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    ScratchSpaceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();
      {
        // Create a job  manager
        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

        // Create a pilot job that requires 1 host, 1 core per host, 1 bytes of RAM per host, and 1 hour
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 1.0, 3600);


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

        // Create a sequential task that lasts one min and requires 1 cores
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 60, 1, 1, 1.0);
        task1->addInputFile(this->workflow->getFileById("input_file1"));

        // Create a sequential task that lasts one min and requires 1 cores
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 360, 1, 1, 1.0);
        task2->addInputFile(this->workflow->getFileById("input_file2"));

        // Create a sequential task that lasts one min and requires 1 cores
        wrench::WorkflowTask *task3 = this->workflow->addTask("task3", 600, 1, 1, 1.0);
        task3->addInputFile(this->workflow->getFileById("input_file3"));

        // Create a StandardJob with SOME pre-copies from public storage to scratch
        wrench::StandardJob *job1 = job_manager->createStandardJob(
                {task1},
                {},
                {std::make_tuple(this->workflow->getFileById("input_file1"), this->test->storage_service1, wrench::ComputeService::SCRATCH)},
                {},
                {});

        // Create a StandardJob with SOME pre-copies from public storage to scratch
        wrench::StandardJob *job2 = job_manager->createStandardJob(
                {task2},
                {},
                {std::make_tuple(this->workflow->getFileById("input_file2"), this->test->storage_service1, wrench::ComputeService::SCRATCH)},
                {},
                {});

        // Create a StandardJob with SOME pre-copies from public storage to scratch
        wrench::StandardJob *job3 = job_manager->createStandardJob(
                {task3},
                {},
                {std::make_tuple(this->workflow->getFileById("input_file3"), this->test->storage_service1, wrench::ComputeService::SCRATCH)},
                {},
                {});


        // Submit the standard job for execution
        try {
          job_manager->submitJob(job1, pilot_job->getComputeService());
          job_manager->submitJob(job2, pilot_job->getComputeService());
          job_manager->submitJob(job3, pilot_job->getComputeService());
        } catch (std::exception &e) {
          throw std::runtime_error(
                  "Unexpected exception while submitting standard job to pilot job: " + std::string(e.what()));
        }

        int i = 0;
        while (i<3) {
          // Wait for the standard job completion
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              // success, check if the scratch space size is not full again or not, it should not be
              if (pilot_job->getComputeService()->getFreeScratchSpaceSize() == 3000.0) {
                throw std::runtime_error(
                        "Pilot Job is expected to clear its scratch space only after all the standard job finishes");
              }
              break;
            }
            default: {
              throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
            }
          }
          i++;
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
            // success, check if the scratch space size is full again or not, it should be full
            wrench::S4U_Simulation::sleep(10); //sleep for some time to ensure everything is deleted
            if (pilot_job->getComputeService()->getFreeScratchSpaceSize() != 3000.0) {
              throw std::runtime_error(
                      "Scratch space should be full after this pilot job expires but it is not now");
            }
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
          }
        }

      }

      return 0;
    }
};

TEST_F(ScratchSpaceTest, PilotJobScratchSpaceTest) {
  DO_TEST_WITH_FORK(do_PilotJobScratchSpace_test);
}


void ScratchSpaceTest::do_PilotJobScratchSpace_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("scratch_space_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Compute Service that does not have scratch space
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {}, 3000.0)));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new PilotJobScratchSpaceTestWMS(
                  this,  {compute_service}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));


  // Create two workflow files
  wrench::WorkflowFile *input_file1 = this->workflow->addFile("input_file1", 1000.0);
  wrench::WorkflowFile *input_file2 = this->workflow->addFile("input_file2", 1000.0);
  wrench::WorkflowFile *input_file3 = this->workflow->addFile("input_file3", 1000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFile(input_file1, storage_service1));
  EXPECT_NO_THROW(simulation->stageFile(input_file2, storage_service1));
  EXPECT_NO_THROW(simulation->stageFile(input_file3, storage_service1));

  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
