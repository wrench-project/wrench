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

#include "../../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_test, "Log category for BatchServiceTest");


class BatchServiceTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;
    wrench::Simulation *simulation;


    void do_BogusSetupTest_test();
    void do_OneStandardJobTaskTest_test();
    void do_TerminateStandardJobsTest_test();
    void do_TwoStandardJobSubmissionTest_test();
    void do_MultipleStandardTaskTest_test();
    void do_PilotJobTaskTest_test();
    void do_StandardPlusPilotJobTaskTest_test();
    void do_InsufficientCoresTaskTest_test();
    void do_BestFitTaskTest_test();
    void do_FirstFitTaskTest_test();
    void do_RoundRobinTask_test();
    void do_noArgumentsJobSubmissionTest_test();
    void do_StandardJobTimeOutTaskTest_test();
    void do_PilotJobTimeOutTaskTest_test();
    void do_StandardJobInsidePilotJobTimeOutTaskTest_test();
    void do_StandardJobInsidePilotJobSucessTaskTest_test();
    void do_InsufficientCoresInsidePilotJobTaskTest_test();
    void do_DifferentBatchAlgorithmsSubmissionTest_test();

protected:
    BatchServiceTest() {

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
              "       <host id=\"HostFast\" speed=\"100f\" core=\"10\"/> "
              "       <host id=\"HostManyCores\" speed=\"1f\" core=\"100\"/> "
              "       <host id=\"RAMHost\" speed=\"1f\" core=\"10\" > "
              "         <prop id=\"ram\" value=\"1024\" />"
              "       </host> "
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
/**  BOGUS SETUP TEST                                                **/
/**********************************************************************/

class BogusSetupTestWMS : public wrench::WMS {

public:
    BogusSetupTestWMS(BatchServiceTest *test,
                      const std::set<wrench::ComputeService *> &compute_services,
                      std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {

      // Do nothing
      return 0;
    }
};

TEST_F(BatchServiceTest, BogusSetupTest) {
  DO_TEST_WITH_FORK(do_BogusSetupTest_test);
}

void BatchServiceTest::do_BogusSetupTest_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Batch Service with a bogus scheduling algorithm
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "Host2", "Host3", "Host4"},
                                   {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "BOGUS"}})),
               std::invalid_argument);

  // Create a Batch Service with a bogus host list
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {},
                                   {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}})),
               std::invalid_argument);

  // Create a Batch Service with a non-homogeneous (speed) host list
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "HostFast"},
                                   {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}})),
               std::invalid_argument);


  // Create a Batch Service with a non-homogeneous (#cores) host list
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "HostManyCores"},
                                   {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}})),
               std::invalid_argument);

  // Create a Batch Service with a non-homogeneous (RAM) host list
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "RAMHost"},
                                   {{wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM, "FCFS"}})),
               std::invalid_argument);


  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  STANDARD JOB TERMINATION TEST                                   **/
/**********************************************************************/

class TerminateOneStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    TerminateOneStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                             const std::set<wrench::ComputeService *> &compute_services,
                                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      wrench::WorkflowTask *task1;
      wrench::WorkflowTask *task2;
      wrench::StandardJob *job1;
      wrench::StandardJob *job2;

      // Submit job1 that should start right away
      {
        // Create a sequential task that lasts one min and requires 2 cores
        task1 = this->workflow->addTask("task1", 60, 1, 1, 1.0);
        task1->addInputFile(this->workflow->getFileById("input_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        job1 = job_manager->createStandardJob(
                {task1},
                {
                        {*(task1->getInputFiles().begin()),  this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "10"; //number of cores per node
        try {
          job_manager->submitJob(job1, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

      }

      // Submit job2 that will be stuck in the queue
      {
        // Create a sequential task that lasts one min and requires 2 cores
        task2 = this->workflow->addTask("task2", 60, 1, 1, 1.0);
        task2->addInputFile(this->workflow->getFileById("input_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        job2 = job_manager->createStandardJob(
                {task2},
                {
                        {*(task2->getInputFiles().begin()),  this->test->storage_service1},
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "4";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "10"; //number of cores per node
        try {
          job_manager->submitJob(job2, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

      }

      this->simulation->sleep(1);

      // Terminate job2 (which is pending)
      job_manager->terminateJob(job2);

      std::unique_ptr<wrench::WorkflowExecutionEvent> event;

      // Terminate job1 (which is running)
      job_manager->terminateJob(job1);

      // Check task states
      if (task1->getState() != wrench::WorkflowTask::State::READY) {
        throw std::runtime_error("Unexpected task1 state: " + wrench::WorkflowTask::stateToString(task1->getState()));
      }

      if (task2->getState() != wrench::WorkflowTask::State::READY) {
        throw std::runtime_error("Unexpected task2 state: " + wrench::WorkflowTask::stateToString(task1->getState()));
      }

      return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_TerminateStandardJobsTest)
#else
TEST_F(BatchServiceTest, TerminateStandardJobsTest)
#endif
{
  DO_TEST_WITH_FORK(do_TerminateStandardJobsTest_test);
}

void BatchServiceTest::do_TerminateStandardJobsTest_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Batch Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new TerminateOneStandardJobSubmissionTestWMS(
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
/**  ONE STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST    **/
/**********************************************************************/

class OneStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    OneStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 2, 2, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "2";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }


        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }
        this->workflow->removeTask(task);

        // Shutdown the compute service, for testing purposes
        this->test->compute_service->stop();

        // Shutdown the compute service, for testing purposes, which should do nothing
        this->test->compute_service->stop();


      }

      return 0;
    }
};

TEST_F(BatchServiceTest, OneStandardJobSubmissionTest) {
  DO_TEST_WITH_FORK(do_OneStandardJobTaskTest_test);
}

void BatchServiceTest::do_OneStandardJobTaskTest_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));


  // Create a Batch Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new OneStandardJobSubmissionTestWMS(
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
/**  TWO STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST    **/
/**********************************************************************/

class TwoStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    TwoStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    const std::set<wrench::StorageService *> &storage_services,
                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname,
                        "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      std::vector<wrench::WorkflowTask *> tasks;
      std::map<std::string, std::string> batch_job_args;

      // Create and submit two jobs that should be able to run concurrently
      for (size_t i = 0; i < 2; i++) {
        double time_fudge = 1; // 1 second seems to make it all work!
        double task_flops = 10 * (1 * (1800 - time_fudge));
        int num_cores = 10;
        double parallel_efficiency = 1.0;
        tasks.push_back(workflow->addTask("test_job_1_task_" + std::to_string(i),
                                          task_flops,
                                          num_cores, num_cores, parallel_efficiency,
                                          0.0));
      }

      // Create a Standard Job with only the tasks
      wrench::StandardJob *standard_job_1;
      standard_job_1 = job_manager->createStandardJob(tasks, {});

      // Create the batch-specific argument
      batch_job_args["-N"] = std::to_string(2); // Number of nodes/tasks
      batch_job_args["-t"] = std::to_string(18000 / 60); // Time in minutes (at least 1 minute)
      batch_job_args["-c"] = std::to_string(10); //number of cores per task

      // Submit this job to the batch service
      job_manager->submitJob(standard_job_1, *(this->getAvailableComputeServices().begin()), batch_job_args);


      // Create and submit a job that needs 2 nodes and 30 minutes
      tasks.clear();
      for (size_t i = 0; i < 2; i++) {
        double time_fudge = 1; // 1 second seems to make it all work!
        double task_flops = 10 * (1 * (1800 - time_fudge));
        int num_cores = 10;
        double parallel_efficiency = 1.0;
        tasks.push_back(workflow->addTask("test_job_2_task_" + std::to_string(i),
                                          task_flops,
                                          num_cores, num_cores, parallel_efficiency,
                                          0.0));
      }

      // Create a Standard Job with only the tasks
      wrench::StandardJob *standard_job_2;
      standard_job_2 = job_manager->createStandardJob(tasks, {});

      // Create the batch-specific argument
      batch_job_args.clear();
      batch_job_args["-N"] = std::to_string(2); // Number of nodes/tasks
      batch_job_args["-t"] = std::to_string(18000 / 60); // Time in minutes (at least 1 minute)
      batch_job_args["-c"] = std::to_string(10); //number of cores per task

      // Submit this job to the batch service
      job_manager->submitJob(standard_job_2, *(this->getAvailableComputeServices().begin()), batch_job_args);


      // Wait for the two execution events
      for (auto job : {standard_job_1, standard_job_2}) {
        // Wait for the workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              if (dynamic_cast<wrench::StandardJobCompletedEvent*>(event.get())->standard_job != job) {
                throw std::runtime_error("Wrong job completion order: got " +
                                         dynamic_cast<wrench::StandardJobCompletedEvent*>(event.get())->standard_job->getName() + " but expected " + job->getName());
              }
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
        } catch (wrench::WorkflowExecutionException &e) {
          //ignore (network error or something)
        }

        double completion_time = this->simulation->getCurrentSimulatedDate();
        double expected_completion_time;
        if (job == standard_job_1) {
          expected_completion_time = 1800;
        } else if (job == standard_job_2) {
          expected_completion_time = 1800;
        } else {
          throw std::runtime_error("Phantom job completion!");
        }
        double delta = fabs(expected_completion_time - completion_time);
        double tolerance = 2;
        if (delta > tolerance) {
          throw std::runtime_error("Unexpected job completion time for job " + job->getName() + ": " +
                                   std::to_string(completion_time) + " (expected: " + std::to_string(expected_completion_time) + ")");
        }

      }
      delete workflow;
      return 0;
    }
};

TEST_F(BatchServiceTest, TwoStandardJobSubmissionTest) {
  DO_TEST_WITH_FORK(do_TwoStandardJobSubmissionTest_test);
}


void BatchServiceTest::do_TwoStandardJobSubmissionTest_test() {


  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new TwoStandardJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

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
/**  ONE PILOT JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class OnePilotJobSubmissionTestWMS : public wrench::WMS {

public:
    OnePilotJobSubmissionTestWMS(BatchServiceTest *test,
                                 const std::set<wrench::ComputeService *> &compute_services,
                                 const std::set<wrench::StorageService *> &storage_services,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a pilot job that needs 1 host, 1 code, 0 bytes of RAM and 30 seconds
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0, 30);

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "1"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node

        // Submit a pilot job
        try {
          job_manager->submitJob((wrench::WorkflowJob *) pilot_job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
            //std::cout<<"Got the pilot job started message\n";
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }

        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
            // success, do nothing for now
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

TEST_F(BatchServiceTest, OnePilotJobSubmissionTest) {
  DO_TEST_WITH_FORK(do_PilotJobTaskTest_test);
}

void BatchServiceTest::do_PilotJobTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true, {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a File Registry Service
  simulation->add(new wrench::FileRegistryService(hostname));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new OnePilotJobSubmissionTestWMS(
                  this,
                  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  STANDARD + PILOT JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class StandardPlusPilotJobSubmissionTestWMS : public wrench::WMS {

public:
    StandardPlusPilotJobSubmissionTestWMS(BatchServiceTest *test,
                                          const std::set<wrench::ComputeService *> &compute_services,
                                          const std::set<wrench::StorageService *> &storage_services,
                                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 2, 2, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "2"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }
        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }
        workflow->removeTask(task);
      }

      {
        // Create a pilot job that needs 1 host, 1 code, 0 bytes of RAM, and 30 seconds
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0.0, 30);

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "1"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node

        // Submit a pilot job
        try {
          job_manager->submitJob((wrench::WorkflowJob *) pilot_job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }

        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
            // success, do nothing for now
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

TEST_F(BatchServiceTest, StandardPlusPilotJobSubmissionTest) {
  DO_TEST_WITH_FORK(do_StandardPlusPilotJobTaskTest_test);
}

void BatchServiceTest::do_StandardPlusPilotJobTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new StandardPlusPilotJobSubmissionTestWMS(
                  this,
                  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  INSUFFICIENT CORES JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class InsufficientCoresJobSubmissionTestWMS : public wrench::WMS {

public:
    InsufficientCoresJobSubmissionTestWMS(BatchServiceTest *test,
                                          const std::set<wrench::ComputeService *> &compute_services,
                                          const std::set<wrench::StorageService *> &storage_services,
                                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 2, 12, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "2"; //time in minutes
        batch_job_args["-c"] = "12"; //number of cores per node, which is too many!

        bool success = true;
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          success = false;
          if (e.getCause()->getCauseType() != wrench::FailureCause::NOT_ENOUGH_COMPUTE_RESOURCES) {
            throw std::runtime_error("Got an exception, as expected, but the failure cause seems wrong");
          }
        }

        if (success) {
          throw std::runtime_error("Job Submission should have generated an exception");
        }


        workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(BatchServiceTest, InsufficientCoresJobSubmissionTest) {
  DO_TEST_WITH_FORK(do_InsufficientCoresTaskTest_test);
}

void BatchServiceTest::do_InsufficientCoresTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new InsufficientCoresJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  NO ARGUMENTS JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class NoArgumentsJobSubmissionTestWMS : public wrench::WMS {

public:
    NoArgumentsJobSubmissionTestWMS(BatchServiceTest *test,
                                    const std::set<wrench::ComputeService *> &compute_services,
                                    const std::set<wrench::StorageService *> &storage_services,
                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 2, 2, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        bool success = false;
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (std::invalid_argument e) {
          success = true;
        }
        if (not success) {
          throw std::runtime_error(
                  "Expecting a runtime error of not arguments but did not get any such exceptions"
          );
        }
        workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(BatchServiceTest, NoArgumentsJobSubmissionTest) {
  DO_TEST_WITH_FORK(do_noArgumentsJobSubmissionTest_test);
}

void BatchServiceTest::do_noArgumentsJobSubmissionTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new NoArgumentsJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  STANDARDJOB TIMEOUT TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class StandardJobTimeoutSubmissionTestWMS : public wrench::WMS {

public:
    StandardJobTimeoutSubmissionTestWMS(BatchServiceTest *test,
                                        const std::set<wrench::ComputeService *> &compute_services,
                                        const std::set<wrench::StorageService *> &storage_services,
                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 65, 1, 1, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "1"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }
        workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(BatchServiceTest, StandardJobTimeOutTaskTest) {
  DO_TEST_WITH_FORK(do_StandardJobTimeOutTaskTest_test);
}

void BatchServiceTest::do_StandardJobTimeOutTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new StandardJobTimeoutSubmissionTestWMS(
                  this, {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow( workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  PILOTJOB TIMEOUT TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class PilotJobTimeoutSubmissionTestWMS : public wrench::WMS {

public:
    PilotJobTimeoutSubmissionTestWMS(BatchServiceTest *test,
                                     const std::set<wrench::ComputeService *> &compute_services,
                                     const std::set<wrench::StorageService *> &storage_services,
                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a pilot job that needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0.0, 90);

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "1"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node

        // Submit a pilot job
        try {
          job_manager->submitJob((wrench::WorkflowJob *) pilot_job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }

        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
            // success, do nothing for now
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

TEST_F(BatchServiceTest, PilotJobTimeOutTaskTest) {
  DO_TEST_WITH_FORK(do_PilotJobTimeOutTaskTest_test);
}

void BatchServiceTest::do_PilotJobTimeOutTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new PilotJobTimeoutSubmissionTestWMS(
                  this, {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  BEST FIT STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class BestFitStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    BestFitStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                        const std::set<wrench::ComputeService *> &compute_services,
                                        const std::set<wrench::StorageService *> &storage_services,
                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 8 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 50, 8, 8, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        //Create another sequential task that lasts one min and requires 9 cores
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 50, 9, 9, 1.0);
        task1->addInputFile(workflow->getFileById("input_file_1"));
        task1->addOutputFile(workflow->getFileById("output_file_1"));

        //Create another sequential task that lasts one min and requires 1 cores
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 50, 1, 1, 1.0);
        task2->addInputFile(workflow->getFileById("input_file_2"));
        task2->addOutputFile(workflow->getFileById("output_file_2"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "2"; //time in minutes
        batch_job_args["-c"] = "8"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        wrench::StandardJob *job1 = job_manager->createStandardJob(
                {task1},
                {
                        {*(task1->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task1->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file_1"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file_1"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> task1_batch_job_args;
        task1_batch_job_args["-N"] = "1";
        task1_batch_job_args["-t"] = "2"; //time in minutes
        task1_batch_job_args["-c"] = "9"; //number of cores per node
        try {
          job_manager->submitJob(job1, this->test->compute_service, task1_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        wrench::StandardJob *job2 = job_manager->createStandardJob(
                {task2},
                {
                        {*(task2->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task2->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file_2"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file_2"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> task2_batch_job_args;
        task2_batch_job_args["-N"] = "1";
        task2_batch_job_args["-t"] = "2"; //time in minutes
        task2_batch_job_args["-c"] = "1"; //number of cores per node
        try {
          job_manager->submitJob(job2, this->test->compute_service, task2_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        //wait for three standard job completion events
        int num_events = 0;
        while (num_events < 3) {
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              // success, do nothing for now
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
          num_events++;
        }

        if (task1->getExecutionHost() != task2->getExecutionHost()) {
          throw std::runtime_error(
                  "BatchServiceTest::BestFitStandardJobSubmissionTest():: BestFit did not pick the right hosts"
          );
        }

        workflow->removeTask(task);
        workflow->removeTask(task1);
        workflow->removeTask(task2);
      }

      return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_BestFitStandardJobSubmissionTest)
#else
TEST_F(BatchServiceTest, BestFitStandardJobSubmissionTest)
#endif
{
  DO_TEST_WITH_FORK(do_BestFitTaskTest_test);
}



void BatchServiceTest::do_BestFitTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {{wrench::StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM, "BESTFIT"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new BestFitStandardJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow( workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
  wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);
  wrench::WorkflowFile *input_file_1 = this->workflow->addFile("input_file_1", 10000.0);
  wrench::WorkflowFile *output_file_1 = this->workflow->addFile("output_file_1", 20000.0);
  wrench::WorkflowFile *input_file_2 = this->workflow->addFile("input_file_2", 10000.0);
  wrench::WorkflowFile *output_file_2 = this->workflow->addFile("output_file_2", 20000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({{input_file->getId(), input_file},
                                          {input_file_1->getId(), input_file_1},
                                          {input_file_2->getId(), input_file_2}}, storage_service1));


  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  FIRST FIT STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class FirstFitStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    FirstFitStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                         const std::set<wrench::ComputeService *> &compute_services,
                                         const std::set<wrench::StorageService *> &storage_services,
                                         std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        //In this test, we create 40 tasks, each task uses one core. There are 4 hosts with 10 cores.
        //So the first 10 tasks should run on first host, next 10 tasks should run on second host,
        //next 10 tasks should run on third and last 10 tasks should run on fourth host
        //using first fit host selection scheduling

        int num_tasks = 40;
        int num_cores_in_each_task = 1;
        unsigned long num_hosts_in_platform = 4;
        unsigned long repetition = num_tasks/(num_cores_in_each_task*num_hosts_in_platform);
        std::vector<wrench::WorkflowTask*> tasks = {};
        std::vector<wrench::StandardJob*> jobs = {};
        for (int i = 0;i<num_tasks;i++) {
          tasks.push_back(this->workflow->addTask("task"+std::to_string(i),59,num_cores_in_each_task,num_cores_in_each_task,1.0));
          jobs.push_back(job_manager->createStandardJob(
                  {tasks[i]}, {}, {}, {}, {}));
          std::map<std::string, std::string> args;
          args["-N"] = "1";
          args["-t"] = "1";
          args["-c"] = std::to_string(num_cores_in_each_task);
          try {
            job_manager->submitJob(jobs[i], this->test->compute_service, args);
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Got some exception"
            );
          }
        }


        //wait for two standard job completion events
        int num_events = 0;
        while (num_events < num_tasks) {
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
          num_events++;
        }


        for (int i = 0;i<num_hosts_in_platform;i++) {
          for (int j = i*repetition;j<(((i+1)*repetition)-1);j++) {
            if (tasks[j]->getExecutionHost() != tasks[j+1]->getExecutionHost()) {
              throw std::runtime_error(
                      "BatchServiceTest::FirstFitStandardJobSubmissionTest():: The tasks did not execute on the right hosts"
              );
            }
          }
        }

        for (int i = 0;i<num_tasks;i++) {
          workflow->removeTask(tasks[i]);
        }
      }

      return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_FirstFitStandardJobSubmissionTest)
#else
TEST_F(BatchServiceTest, FirstFitStandardJobSubmissionTest)
#endif
{
  DO_TEST_WITH_FORK(do_FirstFitTaskTest_test);
}


void BatchServiceTest::do_FirstFitTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {{wrench::StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM, "BESTFIT"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new FirstFitStandardJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow( workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
  wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);
  wrench::WorkflowFile *input_file_1 = this->workflow->addFile("input_file_1", 10000.0);
  wrench::WorkflowFile *output_file_1 = this->workflow->addFile("output_file_1", 20000.0);
  wrench::WorkflowFile *input_file_2 = this->workflow->addFile("input_file_2", 10000.0);
  wrench::WorkflowFile *output_file_2 = this->workflow->addFile("output_file_2", 20000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({{input_file->getId(), input_file},
                                          {input_file_1->getId(), input_file_1},
                                          {input_file_2->getId(), input_file_2}}, storage_service1));


  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}



/**********************************************************************/
/**  ROUND ROBIN JOB SUBMISSION TASK SIMULATION TEST               **/
/**********************************************************************/

class RoundRobinStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    RoundRobinStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                           const std::set<wrench::ComputeService *> &compute_services,
                                           const std::set<wrench::StorageService *> &storage_services,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();
      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 60, 2, 2, 1.0);

        //Create another sequential task that lasts one min and requires 9 cores
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 360, 9, 9, 1.0);

        //Create another sequential task that lasts one min and requires 1 core
        wrench::WorkflowTask *task3 = this->workflow->addTask("task3", 59, 1, 1, 1.0);

        //Create another sequential task that lasts one min and requires 10 cores
        wrench::WorkflowTask *task4 = this->workflow->addTask("task4", 600, 10, 10, 1.0);

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1}, {}, {}, {}, {});

        wrench::StandardJob *job2 = job_manager->createStandardJob(
                {task2}, {}, {}, {}, {});

        wrench::StandardJob *job3 = job_manager->createStandardJob(
                {task3}, {}, {}, {}, {});

        wrench::StandardJob *job4 = job_manager->createStandardJob(
                {task4}, {}, {}, {}, {});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "2";
        batch_job_args["-t"] = "1"; //time in minutes
        batch_job_args["-c"] = "2"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        std::map<std::string, std::string> task2_batch_job_args;
        task2_batch_job_args["-N"] = "1";
        task2_batch_job_args["-t"] = "1"; //time in minutes
        task2_batch_job_args["-c"] = "9"; //number of cores per node
        try {
          job_manager->submitJob(job2, this->test->compute_service, task2_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        std::map<std::string, std::string> task3_batch_job_args;
        task3_batch_job_args["-N"] = "1";
        task3_batch_job_args["-t"] = "1"; //time in minutes
        task3_batch_job_args["-c"] = "1"; //number of cores per node
        try {
          job_manager->submitJob(job3, this->test->compute_service, task3_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }

        std::map<std::string, std::string> task4_batch_job_args;
        task4_batch_job_args["-N"] = "1";
        task4_batch_job_args["-t"] = "2"; //time in minutes
        task4_batch_job_args["-c"] = "10"; //number of cores per node
        try {
          job_manager->submitJob(job4, this->test->compute_service, task4_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }


        //wait for two standard job completion events
        int num_events = 0;
        while (num_events < 4) {
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
          num_events++;
        }

        WRENCH_INFO("Task1 completed on host %s", task1->getExecutionHost().c_str());
        WRENCH_INFO("Task2 completed on host %s", task2->getExecutionHost().c_str());
        WRENCH_INFO("Task3 completed on host %s", task3->getExecutionHost().c_str());
        WRENCH_INFO("Task4 completed on host %s", task4->getExecutionHost().c_str());

        double EPSILON = 1.0;
        double not_expected_date = 60; //FIRSTFIT and BESTFIT would complete in ~60 seconds but ROUNDROBIN would finish, in this case, in ~90 seconds
        if (fabs(this->simulation->getCurrentSimulatedDate() - not_expected_date) <= EPSILON) {
          throw std::runtime_error(
                  "BatchServiceTest::ROUNDROBINTEST():: The tasks did not finish on time: Simulation Date > Expected Date"
          );
        } else {
          //congrats, round robin works
          //however let's check further if the task1 hostname is equal to the task4 hostname
          if (task1->getExecutionHost() != task4->getExecutionHost()) {
            throw std::runtime_error(
                    "BatchServiceTest::ROUNDROBINTEST():: The tasks did not execute on the right hosts"
            );
          }
        }

        workflow->removeTask(task1);
        workflow->removeTask(task2);
        workflow->removeTask(task3);
        workflow->removeTask(task4);
      }

      {
        int num_tasks = 20;
        std::vector<wrench::WorkflowTask*> tasks = {};
        std::vector<wrench::StandardJob*> jobs = {};
        for (int i = 0;i<num_tasks;i++) {
          tasks.push_back(this->workflow->addTask("task"+std::to_string(i),59,1,1,1.0));
          jobs.push_back(job_manager->createStandardJob(
                  {tasks[i]}, {}, {}, {}, {}));
          std::map<std::string, std::string> args;
          args["-N"] = "1";
          args["-t"] = "1";
          args["-c"] = "1";
          try {
            job_manager->submitJob(jobs[i], this->test->compute_service, args);
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Exception: " + std::string(e.what())
            );
          }
        }


        //wait for two standard job completion events
        int num_events = 0;
        while (num_events < num_tasks) {
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
          num_events++;
        }

        unsigned long num_hosts = 4;
        for (int i = 0;i<num_tasks;i++) {
          if (tasks[i]->getExecutionHost() != tasks[(i+num_hosts)%num_hosts]->getExecutionHost()) {
            throw std::runtime_error(
                    "BatchServiceTest::ROUNDROBINTEST():: The tasks in the second test did not execute on the right hosts"
            );
          }
        }

        for (int i = 0;i<num_tasks;i++) {
          workflow->removeTask(tasks[i]);
        }
      }

      return 0;
    }


};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DISABLED_RoundRobinTaskTest)
#else
TEST_F(BatchServiceTest, RoundRobinTaskTest)
#endif
{
  DO_TEST_WITH_FORK(do_RoundRobinTask_test);
}


void BatchServiceTest::do_RoundRobinTask_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {{wrench::StandardJobExecutorProperty::HOST_SELECTION_ALGORITHM, "ROUNDROBIN"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new RoundRobinStandardJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow( workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
  wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);
  wrench::WorkflowFile *input_file_1 = this->workflow->addFile("input_file_1", 10000.0);
  wrench::WorkflowFile *output_file_1 = this->workflow->addFile("output_file_1", 20000.0);
  wrench::WorkflowFile *input_file_2 = this->workflow->addFile("input_file_2", 10000.0);
  wrench::WorkflowFile *output_file_2 = this->workflow->addFile("output_file_2", 20000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({{input_file->getId(), input_file},
                                          {input_file_1->getId(), input_file_1},
                                          {input_file_2->getId(), input_file_2}}, storage_service1));


  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}



/***********************************************************************************************/
/**  STANDARDJOB INSIDE PILOT JOB FAILURE TASK SIMULATION TEST ON ONE-ONE HOST                **/
/***********************************************************************************************/

class StandardJobInsidePilotJobTimeoutSubmissionTestWMS : public wrench::WMS {

public:
    StandardJobInsidePilotJobTimeoutSubmissionTestWMS(BatchServiceTest *test,
                                                      const std::set<wrench::ComputeService *> &compute_services,
                                                      const std::set<wrench::StorageService *> &storage_services,
                                                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a pilot job that needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0.0, 90);

        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 2, 2, 1.0);
        wrench::WorkflowFile* file1 = workflow->getFileById("input_file");
        wrench::WorkflowFile* file2 = workflow->getFileById("output_file");
        task->addInputFile(file1);
        task->addOutputFile(file2);

        std::map<std::string, std::string> pilot_batch_job_args;
        pilot_batch_job_args["-N"] = "1";
        pilot_batch_job_args["-t"] = "2"; //time in minutes
        pilot_batch_job_args["-c"] = "4"; //number of cores per node

        // Submit a pilot job
        try {
          job_manager->submitJob((wrench::WorkflowJob *) pilot_job, this->test->compute_service, pilot_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception " + std::string(e.what())
          );
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }

        // Create a StandardJob with some pre-copies and post-deletions
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task}, {{file1,this->test->storage_service1}}, {}, {}, {});

        std::map<std::string, std::string> standard_batch_job_args;
        standard_batch_job_args["-N"] = "1";
        standard_batch_job_args["-t"] = "1"; //time in minutes
        standard_batch_job_args["-c"] = "2"; //number of cores per node
        try {
          job_manager->submitJob(job, pilot_job->getComputeService(), standard_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
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
            if (dynamic_cast<wrench::StandardJobFailedEvent*>(event.get())->failure_cause->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
              throw std::runtime_error("Got a job failure event, but the failure cause seems wrong");
            }
            wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) (dynamic_cast<wrench::StandardJobFailedEvent*>(event.get())->failure_cause.get());
            std::string error_msg = real_cause->toString();
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

        workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(BatchServiceTest, StandardJobInsidePilotJobTimeOutTaskTest) {
  DO_TEST_WITH_FORK(do_StandardJobInsidePilotJobTimeOutTaskTest_test);
}

void BatchServiceTest::do_StandardJobInsidePilotJobTimeOutTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"}, {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new StandardJobInsidePilotJobTimeoutSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow( workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  STANDARDJOB INSIDE PILOT JOB SUCESS TASK SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class StandardJobInsidePilotJobSucessSubmissionTestWMS : public wrench::WMS {

public:
    StandardJobInsidePilotJobSucessSubmissionTestWMS(BatchServiceTest *test,
                                                     const std::set<wrench::ComputeService *> &compute_services,
                                                     const std::set<wrench::StorageService *> &storage_services,
                                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a pilot job tbat needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0.0, 90);

        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 2, 2, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        std::map<std::string, std::string> pilot_batch_job_args;
        pilot_batch_job_args["-N"] = "1";
        pilot_batch_job_args["-t"] = "2"; //time in minutes
        pilot_batch_job_args["-c"] = "4"; //number of cores per node

        // Submit a pilot job
        try {
          job_manager->submitJob((wrench::WorkflowJob *) pilot_job, this->test->compute_service, pilot_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception " + std::string(e.what())
          );
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }


        // Create a StandardJob with some pre-copies and post-deletions
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> standard_batch_job_args;
        standard_batch_job_args["-N"] = "1";
        standard_batch_job_args["-t"] = "1"; //time in minutes
        standard_batch_job_args["-c"] = "2"; //number of cores per node
        try {
          job_manager->submitJob(job, pilot_job->getComputeService(), standard_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }


        // Wait for the standard job success notification
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Error while getting and execution event: " + std::to_string(e.getCause()->getCauseType()));
        }
        bool success = false;
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            success = true;
            break;
          }
          default: {
            success = false;
          }
        }

        if (not success) {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }

        workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(BatchServiceTest, StandardJobInsidePilotJobSucessTaskTest) {
  DO_TEST_WITH_FORK(do_StandardJobInsidePilotJobSucessTaskTest_test);
}

void BatchServiceTest::do_StandardJobInsidePilotJobSucessTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new StandardJobInsidePilotJobSucessSubmissionTestWMS(
                  this, {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow( workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  INSUFFICIENT CORES INSIDE PILOT JOB SIMULATION TEST ON ONE-ONE HOST                **/
/**********************************************************************/

class InsufficientCoresInsidePilotJobSubmissionTestWMS : public wrench::WMS {

public:
    InsufficientCoresInsidePilotJobSubmissionTestWMS(BatchServiceTest *test,
                                                     const std::set<wrench::ComputeService *> &compute_services,
                                                     const std::set<wrench::StorageService *> &storage_services,
                                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a pilot job that needs 1 host, 1 core, 0 bytes of RAM, and 90 seconds
        wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0, 90);

        // Create a sequential task that lasts one min and requires 5 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 5, 5, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        std::map<std::string, std::string> pilot_batch_job_args;
        pilot_batch_job_args["-N"] = "1";
        pilot_batch_job_args["-t"] = "2"; //time in minutes
        pilot_batch_job_args["-c"] = "4"; //number of cores per node

        // Submit a pilot job
        try {
          job_manager->submitJob((wrench::WorkflowJob *) pilot_job, this->test->compute_service, pilot_batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Got some exception " + std::string(e.what())
          );
        }

        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::PILOT_JOB_START: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }


        // Create a StandardJob with some pre-copies and post-deletions
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> standard_batch_job_args;
        standard_batch_job_args["-N"] = "1";
        standard_batch_job_args["-t"] = "1"; //time in minutes
        standard_batch_job_args["-c"] = "5"; //number of cores per node
        bool success = false;
        try {
          job_manager->submitJob(job, pilot_job->getComputeService(), standard_batch_job_args);
        } catch (wrench::WorkflowExecutionException e) {
          success = true;
        }

        if (not success) {
          throw std::runtime_error(
                  "Expected a runtime error of insufficient cores in pilot job"
          );
        }

        workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(BatchServiceTest, InsufficientCoresInsidePilotJobTaskTest) {
  DO_TEST_WITH_FORK(do_InsufficientCoresInsidePilotJobTaskTest_test);
}

void BatchServiceTest::do_InsufficientCoresInsidePilotJobTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new InsufficientCoresInsidePilotJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  MULTIPLE STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST                **/
/**********************************************************************/

class MultipleStandardJobSubmissionTestWMS : public wrench::WMS {

public:
    MultipleStandardJobSubmissionTestWMS(BatchServiceTest *test,
                                         const std::set<wrench::ComputeService *> &compute_services,
                                         const std::set<wrench::StorageService *> &storage_services,
                                         std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {

        int num_standard_jobs = 10;
        int each_task_time = 60; //in seconds
        std::vector<wrench::StandardJob *> jobs;
        std::vector<wrench::WorkflowTask *> tasks;
        for (int i = 0; i < num_standard_jobs; i++) {
          // Create a sequential task that lasts for random minutes and requires 2 cores
          wrench::WorkflowTask *task = this->workflow->addTask("task" + std::to_string(i), each_task_time, 2, 2, 1.0);
          wrench::StandardJob *job = job_manager->createStandardJob(
                  {task}, {}, {}, {}, {});
          tasks.push_back(std::move(task));
          jobs.push_back(std::move(job));
        }


        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = std::to_string((each_task_time / 60) * num_standard_jobs); //time in minutes
        batch_job_args["-c"] = "2"; //number of cores per node
        for (auto standard_jobs:jobs) {
          try {
            job_manager->submitJob(standard_jobs, this->test->compute_service, batch_job_args);
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Exception: " + std::string(e.what())
            );
          }
        }

        for (int i = 0; i < num_standard_jobs; i++) {

          // Wait for a workflow execution event
          std::unique_ptr<wrench::WorkflowExecutionEvent> event;
          try {
            event = workflow->waitForNextExecutionEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
          switch (event->type) {
            case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
              // success, do nothing for now
              break;
            }
            default: {
              throw std::runtime_error(
                      "Unexpected workflow execution event: " + std::to_string((int) (event->type)));
            }
          }
        }

        for (auto each_task:tasks) {
          workflow->removeTask(each_task);
        }
      }

      return 0;
    }
};

TEST_F(BatchServiceTest, MultipleStandardJobSubmissionTest) {
  DO_TEST_WITH_FORK(do_MultipleStandardTaskTest_test);
}


void BatchServiceTest::do_MultipleStandardTaskTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname, true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},
                                   {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new MultipleStandardJobSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow( workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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
/**  DIFFERENT BATCHSERVICE ALGORITHMS SUBMISSION TASK SIMULATION TEST **/
/**********************************************************************/

class DifferentBatchAlgorithmsSubmissionTestWMS : public wrench::WMS {

public:
    DifferentBatchAlgorithmsSubmissionTestWMS(BatchServiceTest *test,
                                              const std::set<wrench::ComputeService *> &compute_services,
                                              const std::set<wrench::StorageService *> &storage_services,
                                              std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      {
        // Create a sequential task that lasts one min and requires 2 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 60, 2, 2, 1.0);
        task->addInputFile(this->workflow->getFileById("input_file"));
        task->addOutputFile(this->workflow->getFileById("output_file"));


        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        this->workflow->getFileById("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(this->workflow->getFileById("input_file"),
                                                                              this->test->storage_service2)});

        std::map<std::string, std::string> batch_job_args;
        batch_job_args["-N"] = "1";
        batch_job_args["-t"] = "5"; //time in minutes
        batch_job_args["-c"] = "4"; //number of cores per node
        try {
          job_manager->submitJob(job, this->test->compute_service, batch_job_args);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(
                  "Exception: " + std::string(e.what())
          );
        }


        // Wait for a workflow execution event
        std::unique_ptr<wrench::WorkflowExecutionEvent> event;
        try {
          event = this->workflow->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        switch (event->type) {
          case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            // success, do nothing for now
            break;
          }
          default: {
            throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event->type)));
          }
        }
        this->workflow->removeTask(task);
      }

      return 0;
    }
};


#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, DifferentBatchAlgorithmsSubmissionTest)
#else
TEST_F(BatchServiceTest, DISABLED_DifferentBatchAlgorithmsSubmissionTest)
#endif
{
  DO_TEST_WITH_FORK(do_DifferentBatchAlgorithmsSubmissionTest_test);
}


void BatchServiceTest::do_DifferentBatchAlgorithmsSubmissionTest_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("batch_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = "Host1";

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Batch Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::BatchService(hostname,
                                   true, true,
                                   {"Host1", "Host2", "Host3", "Host4"},  {
                                           {wrench::BatchServiceProperty::BATCH_SCHEDULING_ALGORITHM,     "filler"},
                                           {wrench::BatchServiceProperty::BATCH_QUEUE_ORDERING_ALGORITHM, "fcfs"}
                                   })));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new DifferentBatchAlgorithmsSubmissionTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(std::move(workflow).get()));

  simulation->add(new wrench::FileRegistryService(hostname));

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


