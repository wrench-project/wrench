/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <gtest/gtest.h>

#include <workflow/Workflow.h>
#include <simulation/Simulation.h>
#include <services/storage_services/simple_storage_service/SimpleStorageService.h>
#include <wms/scheduler/RandomScheduler.h>
#include <services/compute_services/multicore_compute_service/MulticoreComputeService.h>
#include <managers/data_movement_manager/DataMovementManager.h>
#include <wrench-dev.h>

#define EXIT_AFTER_TEST     exit((::testing::Test::HasFailure() ? 666 : 0))

class OneTaskTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_NoopSimulation_test();

    void do_ExecutionWithLocationMap_test();

    void do_ExecutionWithDefaultStorageService_test();

    void do_ExecutionWithPrePostCopies_test();


protected:
    OneTaskTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create two files
      input_file = workflow->addFile("input_file", 10000.0);
      output_file = workflow->addFile("output_file", 20000.0);

      // Create one task
      task = workflow->addTask("task", 3600);
      task->addInputFile(input_file);
      task->addOutputFile(output_file);

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
/**  NOOP SIMULATION TEST                                            **/
/**********************************************************************/

class NoopSimulationTestWMS : public wrench::WMS {

public:
    NoopSimulationTestWMS(OneTaskTest *test,
                          wrench::Workflow *workflow,
                          std::unique_ptr<wrench::Scheduler> scheduler,
                          std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }


private:

    OneTaskTest *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob(test->task,
                                                                {{test->input_file,  test->storage_service1},
                                                                 {test->output_file, test->storage_service1}});

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(OneTaskTest, NoopSimulation) {

  pid_t pid = fork();
  if (pid) {
    int exit_code;
    waitpid(pid, &exit_code, 0);
    ASSERT_EQ(exit_code, 0);
  } else {
    this->do_NoopSimulation_test();
    EXIT_AFTER_TEST;
  }
}

void OneTaskTest::do_NoopSimulation_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new NoopSimulationTestWMS(this, workflow,
                                                                 std::unique_ptr<wrench::Scheduler>(
                                                                         new wrench::RandomScheduler()),
                          hostname))));

  // Create a Compute Service
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      nullptr,
                                                      {}))));

  // Create a Storage Service
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Without a file registry service this should fail
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  ASSERT_THROW(simulation->stageFiles({input_file}, storage_service1), std::runtime_error);

  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));


  simulation->setFileRegistryService(std::move(file_registry_service));

  ASSERT_THROW(simulation->stageFiles({input_file}, nullptr), std::invalid_argument);
  ASSERT_THROW(simulation->stageFiles({nullptr}, storage_service1), std::invalid_argument);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
}

/**********************************************************************/
/** EXECUTION WITH LOCATION_MAP SIMULATION TEST                      **/
/**********************************************************************/

class ExecutionWithLocationMapTestWMS : public wrench::WMS {

public:
    ExecutionWithLocationMapTestWMS(OneTaskTest *test,
                                    wrench::Workflow *workflow,
                                    std::unique_ptr<wrench::Scheduler> scheduler,
                                    std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }


private:

    OneTaskTest *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob(test->task,
                                                                {{test->input_file,  test->storage_service1},
                                                                 {test->output_file, test->storage_service1}});

      // Submit the job
      job_manager->submitJob(job, test->compute_service);

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();
      if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
        throw std::runtime_error("Unexpected workflow execution event!");
      }

      if (!this->test->storage_service1->lookupFile(this->test->output_file)) {
        throw std::runtime_error("Output file not written to storage service");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(OneTaskTest, ExecutionWithLocationMap) {

  pid_t pid = fork();
  if (pid) {
    int exit_code;
    waitpid(pid, &exit_code, 0);
    ASSERT_EQ(exit_code, 0);
  } else {
    this->do_ExecutionWithLocationMap_test();
    EXIT_AFTER_TEST;
  }
}

void OneTaskTest::do_ExecutionWithLocationMap_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new ExecutionWithLocationMapTestWMS(this, workflow,
                                                                           std::unique_ptr<wrench::Scheduler>(
                                                                                   new wrench::RandomScheduler()),
                          hostname))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      nullptr,
                                                      {}))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Create a File Registry Service
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  EXPECT_NO_THROW(simulation->setFileRegistryService(std::move(file_registry_service)));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check that the output trace makes sense
  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
  ASSERT_EQ(task->getFailureCount(), 0);
  ASSERT_GT(task->getStartDate(), 0.0);
  ASSERT_GT(task->getEndDate(), 0.0);
  ASSERT_GT(task->getEndDate(), task->getStartDate());

  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
          simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getDate(),
            task->getEndDate());
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
            task);

  delete simulation;
}

/**********************************************************************/
/** EXECUTION WITH DEFAULT STORAGE SERVICE SIMULATION TEST           **/
/**********************************************************************/

class ExecutionWithDefaultStorageServiceTestWMS : public wrench::WMS {

public:
    ExecutionWithDefaultStorageServiceTestWMS(OneTaskTest *test,
                                              wrench::Workflow *workflow,
                                              std::unique_ptr<wrench::Scheduler> scheduler,
                                              std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }


private:

    OneTaskTest *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob(test->task,
                                                                {});

      // Submit the job
      job_manager->submitJob(job, test->compute_service);

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();
      if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
        throw std::runtime_error("Unexpected workflow execution event!");
      }

      if (!this->test->storage_service1->lookupFile(this->test->output_file)) {
        throw std::runtime_error("Output file not written to storage service");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(OneTaskTest, ExecutionWithDefaultStorageService) {

  pid_t pid = fork();
  if (pid) {
    int exit_code;
    waitpid(pid, &exit_code, 0);
    ASSERT_EQ(exit_code, 0);
  } else {
    this->do_ExecutionWithDefaultStorageService_test();
    EXIT_AFTER_TEST;
  }
}

void OneTaskTest::do_ExecutionWithDefaultStorageService_test() {
  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new ExecutionWithDefaultStorageServiceTestWMS(this, workflow,
                                                                                     std::unique_ptr<wrench::Scheduler>(
                                                                                             new wrench::RandomScheduler()),
                          hostname))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      storage_service1,
                                                      {}))));

  // Create a File Registry Service
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));
  EXPECT_NO_THROW(simulation->setFileRegistryService(std::move(file_registry_service)));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check that the output trace makes sense
  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
  ASSERT_EQ(task->getFailureCount(), 0);
  ASSERT_GT(task->getStartDate(), 0.0);
  ASSERT_GT(task->getEndDate(), 0.0);
  ASSERT_GT(task->getEndDate(), task->getStartDate());

  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
          simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getDate(),
            task->getEndDate());
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
            task);

  delete simulation;
}

/**********************************************************************/
/** EXECUTION WITH PRE/POST COPIES AND CLEANUP SIMULATION TEST       **/
/**********************************************************************/

class ExecutionWithPrePostCopiesAndCleanupTestWMS : public wrench::WMS {

public:
    ExecutionWithPrePostCopiesAndCleanupTestWMS(OneTaskTest *test,
                                                wrench::Workflow *workflow,
                                                std::unique_ptr<wrench::Scheduler> scheduler,
                                                std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }


private:

    OneTaskTest *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      // Create a job
      wrench::StandardJob *job = job_manager->createStandardJob({test->task},
                                                                {},
                                                                {{test->input_file, test->storage_service1, test->storage_service2}},
                                                                {{test->output_file, test->storage_service2, test->storage_service1}},
                                                                {{test->input_file, test->storage_service2},
                                                                 {test->output_file, test->storage_service2}});

      // Submit the job
      job_manager->submitJob(job, test->compute_service);

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();
      switch (event->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          break;
        }
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
          throw std::runtime_error("Unexpected job failure: " + event->failure_cause->toString());
        }
        default:{
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
        }
      }

      // Test file locations
      if (!this->test->storage_service1->lookupFile(this->test->input_file)) {
        throw std::runtime_error("Input file should be on Storage Service #1");
      }
      if (!this->test->storage_service1->lookupFile(this->test->output_file)) {
        throw std::runtime_error("Output file should be on Storage Service #1");
      }
      if (this->test->storage_service2->lookupFile(this->test->input_file)) {
        throw std::runtime_error("Input file should not be on Storage Service #2");
      }
      if (this->test->storage_service2->lookupFile(this->test->input_file)) {
        throw std::runtime_error("Output file should not be on Storage Service #2");
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(OneTaskTest, ExecutionWithPrePostCopies) {

  pid_t pid = fork();
  if (pid) {
    int exit_code;
    waitpid(pid, &exit_code, 0);
    ASSERT_EQ(exit_code, 0);
  } else {
    this->do_ExecutionWithPrePostCopies_test();
    EXIT_AFTER_TEST;
  }
}

void OneTaskTest::do_ExecutionWithPrePostCopies_test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  ASSERT_THROW(simulation->launch(), std::runtime_error);

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_THROW(simulation->launch(), std::runtime_error);
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
          std::unique_ptr<wrench::WMS>(new ExecutionWithPrePostCopiesAndCleanupTestWMS(this, workflow,
                                                                                       std::unique_ptr<wrench::Scheduler>(
                                                                                               new wrench::RandomScheduler()),
                          hostname))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Create another Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));


  // Create a Compute Service with default Storage Service #2
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      storage_service2,
                                                      {}))));

  // Create a File Registry Service
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));
  EXPECT_NO_THROW(simulation->setFileRegistryService(std::move(file_registry_service)));

  // Staging the input_file on storage service #1
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  // Check that the output trace makes sense
  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
  ASSERT_EQ(task->getFailureCount(), 0);
  ASSERT_GT(task->getStartDate(), 0.0);
  ASSERT_GT(task->getEndDate(), 0.0);
  ASSERT_GT(task->getEndDate(), task->getStartDate());

  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
          simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getDate(),
            task->getEndDate());
  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
            task);

  delete simulation;
}
