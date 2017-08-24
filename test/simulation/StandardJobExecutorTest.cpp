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
#include <services/compute_services/standard_job_executor/StandardJobExecutor.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <simulation/SimulationMessage.h>
#include <services/compute_services/standard_job_executor/StandardJobExecutorMessage.h>

#include "TestWithFork.h"


class StandardJobExecutorTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::Simulation *simulation;


    void do_OneSingleCoreTaskTestWMS_test();

//    void do_ExecutionWithDefaultStorageService_test();
//
//    void do_ExecutionWithPrePostCopies_test();


protected:
    StandardJobExecutorTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create a four-host 10-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
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
/**  ONE SINGLE-CORE TASK SIMULATION TEST                            **/
/**********************************************************************/

class OneSingleCoreTaskTestWMS : public wrench::WMS {

public:
    OneSingleCoreTaskTestWMS(StandardJobExecutorTest *test,
                wrench::Workflow *workflow,
                std::unique_ptr<wrench::Scheduler> scheduler,
                std::string hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

//      // Create a data movement manager
//      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
//              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a sequential task that lasts one hour
      wrench::WorkflowTask *task = this->workflow->addTask("task", 3600, 1, 1, 1.0);
      task->addInputFile(workflow->getFileById("input_file"));
      task->addOutputFile(workflow->getFileById("output_file"));

      // Create a StandardJob
      wrench::StandardJob *job = job_manager->createStandardJob(
              task,
              {
                      {*(task->getInputFiles().begin()), this->test->storage_service1},
                      {*(task->getOutputFiles().begin()), this->test->storage_service1}
              });

      std::string my_mailbox = "TEST_CALLBACK_MAILBOX";

      // Create a StandardJobExecutor that wil run stuff on one host and one core
      wrench::StandardJobExecutor *executor = new wrench::StandardJobExecutor(
              test->simulation,
              my_mailbox,
              test->simulation->getHostnameList()[0],
              job,
              {{test->simulation->getHostnameList()[0], 2}},
              nullptr,
              {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}
              );

      // Wait for a message on my mailbox
      std::unique_ptr<wrench::SimulationMessage> message;
      try {
        message = wrench::S4U_Mailbox::getMessage(my_mailbox);
      } catch (std::shared_ptr<wrench::NetworkError> cause) {
        throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
      }

      // Did we get the expected message?
      wrench::StandardJobExecutorDoneMessage *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
      if (!msg) {
        throw std::runtime_error("Unexpected '" + message->getName() + "' message");
      }

      // Does the task completion time make sense?
      if ((task->getEndDate() < task->getFlops()) || (task->getEndDate() > task->getFlops() + 0.5)) {
        throw std::runtime_error("Unexpected task completion time (should be around " + std::to_string(task->getFlops()) + " but is " + std::to_string(task->getEndDate()));
      }

      // Terminate everything
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneSingleCoreTaskTestWMS) {
  DO_TEST_WITH_FORK(do_OneSingleCoreTaskTestWMS_test);
}

void StandardJobExecutorTest::do_OneSingleCoreTaskTestWMS_test() {

  // Create and initialize a simulation
  simulation = new wrench::Simulation();
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
          std::unique_ptr<wrench::WMS>(new OneSingleCoreTaskTestWMS(this, workflow,
                                                       std::unique_ptr<wrench::Scheduler>(
                                                               new wrench::RandomScheduler()), hostname))));

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MulticoreComputeService>(
                  new wrench::MulticoreComputeService(hostname, true, true,
                                                      nullptr,
                                                      {}))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
  wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));


  // Running a "run a single task" simulation
  // Note that in these tests the WMS creates workflow tasks, which a user would
  // of course not be likely to do
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

///**********************************************************************/
///** EXECUTION WITH LOCATION_MAP SIMULATION TEST                      **/
///**********************************************************************/
//
//class ExecutionWithLocationMapTestWMS : public wrench::WMS {
//
//public:
//    ExecutionWithLocationMapTestWMS(OneTaskTest *test,
//                                    wrench::Workflow *workflow,
//                                    std::unique_ptr<wrench::Scheduler> scheduler,
//                                    std::string hostname) :
//            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
//      this->test = test;
//    }
//
//
//private:
//
//    OneTaskTest *test;
//
//    int main() {
//
//      // Create a job manager
//      std::unique_ptr<wrench::JobManager> job_manager =
//              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
//
//      // Create a job
//      wrench::StandardJob *job = job_manager->createStandardJob(test->task,
//                                                                {{test->input_file,  test->storage_service1},
//                                                                 {test->output_file, test->storage_service1}});
//
//      // Submit the job
//      job_manager->submitJob(job, test->compute_service);
//
//      // Wait for the workflow execution event
//      std::unique_ptr<wrench::WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();
//      if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
//        throw std::runtime_error("Unexpected workflow execution event!");
//      }
//
//      if (!this->test->storage_service1->lookupFile(this->test->output_file)) {
//        throw std::runtime_error("Output file not written to storage service");
//      }
//
//      // Terminate
//      this->simulation->shutdownAllComputeServices();
//      this->simulation->shutdownAllStorageServices();
//      this->simulation->getFileRegistryService()->stop();
//      return 0;
//    }
//};
//
//TEST_F(OneTaskTest, ExecutionWithLocationMap) {
//  DO_TEST_WITH_FORK(do_ExecutionWithLocationMap_test);
//}
//
//void OneTaskTest::do_ExecutionWithLocationMap_test() {
//
//  // Create and initialize a simulation
//  wrench::Simulation *simulation = new wrench::Simulation();
//  int argc = 1;
//  char **argv = (char **) calloc(1, sizeof(char *));
//  argv[0] = strdup("one_task_test");
//
//  simulation->init(&argc, argv);
//
//  // Setting up the platform
//  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
//
//  // Get a hostname
//  std::string hostname = simulation->getHostnameList()[0];
//
//  // Create a WMS
//  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
//          std::unique_ptr<wrench::WMS>(new ExecutionWithLocationMapTestWMS(this, workflow,
//                                                                           std::unique_ptr<wrench::Scheduler>(
//                                                                                   new wrench::RandomScheduler()),
//                          hostname))));
//
//  // Create a Compute Service
//  EXPECT_NO_THROW(compute_service = simulation->add(
//          std::unique_ptr<wrench::MulticoreComputeService>(
//                  new wrench::MulticoreComputeService(hostname, true, true,
//                                                      nullptr,
//                                                      {}))));
//
//  // Create a Storage Service
//  EXPECT_NO_THROW(storage_service1 = simulation->add(
//          std::unique_ptr<wrench::SimpleStorageService>(
//                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));
//
//  // Create a File Registry Service
//  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
//          new wrench::FileRegistryService(hostname));
//
//  EXPECT_NO_THROW(simulation->setFileRegistryService(std::move(file_registry_service)));
//
//  // Staging the input_file on the storage service
//  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));
//
//
//  // Running a "run a single task" simulation
//  EXPECT_NO_THROW(simulation->launch());
//
//  // Check that the output trace makes sense
//  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
//  ASSERT_EQ(task->getFailureCount(), 0);
//  ASSERT_GT(task->getStartDate(), 0.0);
//  ASSERT_GT(task->getEndDate(), 0.0);
//  ASSERT_GT(task->getEndDate(), task->getStartDate());
//
//  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
//          simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>();
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getDate(),
//            task->getEndDate());
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
//            task);
//
//  delete simulation;
//}
//
///**********************************************************************/
///** EXECUTION WITH DEFAULT STORAGE SERVICE SIMULATION TEST           **/
///**********************************************************************/
//
//class ExecutionWithDefaultStorageServiceTestWMS : public wrench::WMS {
//
//public:
//    ExecutionWithDefaultStorageServiceTestWMS(OneTaskTest *test,
//                                              wrench::Workflow *workflow,
//                                              std::unique_ptr<wrench::Scheduler> scheduler,
//                                              std::string hostname) :
//            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
//      this->test = test;
//    }
//
//
//private:
//
//    OneTaskTest *test;
//
//    int main() {
//
//      // Create a job manager
//      std::unique_ptr<wrench::JobManager> job_manager =
//              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
//
//      // Create a job
//      wrench::StandardJob *job = job_manager->createStandardJob(test->task,
//                                                                {});
//
//      // Submit the job
//      job_manager->submitJob(job, test->compute_service);
//
//      // Wait for the workflow execution event
//      std::unique_ptr<wrench::WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();
//      if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
//        throw std::runtime_error("Unexpected workflow execution event!");
//      }
//
//      if (!this->test->storage_service1->lookupFile(this->test->output_file)) {
//        throw std::runtime_error("Output file not written to storage service");
//      }
//
//      // Terminate
//      this->simulation->shutdownAllComputeServices();
//      this->simulation->shutdownAllStorageServices();
//      this->simulation->getFileRegistryService()->stop();
//      return 0;
//    }
//};
//
//TEST_F(OneTaskTest, ExecutionWithDefaultStorageService) {
//
//  DO_TEST_WITH_FORK(do_ExecutionWithDefaultStorageService_test);
//}
//
//void OneTaskTest::do_ExecutionWithDefaultStorageService_test() {
//  // Create and initialize a simulation
//  wrench::Simulation *simulation = new wrench::Simulation();
//  int argc = 1;
//  char **argv = (char **) calloc(1, sizeof(char *));
//  argv[0] = strdup("one_task_test");
//
//  ASSERT_THROW(simulation->launch(), std::runtime_error);
//
//  simulation->init(&argc, argv);
//
//  // Setting up the platform
//  ASSERT_THROW(simulation->launch(), std::runtime_error);
//  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
//  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);
//
//  // Get a hostname
//  std::string hostname = simulation->getHostnameList()[0];
//
//  // Create a WMS
//  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
//          std::unique_ptr<wrench::WMS>(new ExecutionWithDefaultStorageServiceTestWMS(this, workflow,
//                                                                                     std::unique_ptr<wrench::Scheduler>(
//                                                                                             new wrench::RandomScheduler()),
//                          hostname))));
//
//  // Create a Storage Service
//  EXPECT_NO_THROW(storage_service1 = simulation->add(
//          std::unique_ptr<wrench::SimpleStorageService>(
//                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));
//
//  // Create a Compute Service
//  EXPECT_NO_THROW(compute_service = simulation->add(
//          std::unique_ptr<wrench::MulticoreComputeService>(
//                  new wrench::MulticoreComputeService(hostname, true, true,
//                                                      storage_service1,
//                                                      {}))));
//
//  // Create a File Registry Service
//  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
//          new wrench::FileRegistryService(hostname));
//  EXPECT_NO_THROW(simulation->setFileRegistryService(std::move(file_registry_service)));
//
//  // Staging the input_file on the storage service
//  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));
//
//
//  // Running a "run a single task" simulation
//  EXPECT_NO_THROW(simulation->launch());
//
//  // Check that the output trace makes sense
//  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
//  ASSERT_EQ(task->getFailureCount(), 0);
//  ASSERT_GT(task->getStartDate(), 0.0);
//  ASSERT_GT(task->getEndDate(), 0.0);
//  ASSERT_GT(task->getEndDate(), task->getStartDate());
//
//  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
//          simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>();
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getDate(),
//            task->getEndDate());
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
//            task);
//
//  delete simulation;
//}
//
///**********************************************************************/
///** EXECUTION WITH PRE/POST COPIES AND CLEANUP SIMULATION TEST       **/
///**********************************************************************/
//
//class ExecutionWithPrePostCopiesAndCleanupTestWMS : public wrench::WMS {
//
//public:
//    ExecutionWithPrePostCopiesAndCleanupTestWMS(OneTaskTest *test,
//                                                wrench::Workflow *workflow,
//                                                std::unique_ptr<wrench::Scheduler> scheduler,
//                                                std::string hostname) :
//            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
//      this->test = test;
//    }
//
//
//private:
//
//    OneTaskTest *test;
//
//    int main() {
//
//      // Create a job manager
//      std::unique_ptr<wrench::JobManager> job_manager =
//              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));
//
//      // Create a job
//      wrench::StandardJob *job = job_manager->createStandardJob({test->task},
//                                                                {},
//                                                                {std::tuple<wrench::WorkflowFile*, wrench::StorageService*, wrench::StorageService*> {test->input_file, test->storage_service1, test->storage_service2}},
//                                                                {std::tuple<wrench::WorkflowFile*, wrench::StorageService*, wrench::StorageService*> {test->output_file, test->storage_service2, test->storage_service1}},
//                                                                {std::tuple<wrench::WorkflowFile*, wrench::StorageService*> {test->input_file, test->storage_service2},
//                                                                 std::tuple<wrench::WorkflowFile*, wrench::StorageService*> {test->output_file, test->storage_service2}});
//      // Submit the job
//      job_manager->submitJob(job, test->compute_service);
//
//      // Wait for the workflow execution event
//      std::unique_ptr<wrench::WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();
//      switch (event->type) {
//        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
//          break;
//        }
//        case wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
//          throw std::runtime_error("Unexpected job failure: " + event->failure_cause->toString());
//        }
//        default:{
//          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string(event->type));
//        }
//      }
//
//      // Test file locations
//      if (!this->test->storage_service1->lookupFile(this->test->input_file)) {
//        throw std::runtime_error("Input file should be on Storage Service #1");
//      }
//      if (!this->test->storage_service1->lookupFile(this->test->output_file)) {
//        throw std::runtime_error("Output file should be on Storage Service #1");
//      }
//      if (this->test->storage_service2->lookupFile(this->test->input_file)) {
//        throw std::runtime_error("Input file should not be on Storage Service #2");
//      }
//      if (this->test->storage_service2->lookupFile(this->test->input_file)) {
//        throw std::runtime_error("Output file should not be on Storage Service #2");
//      }
//
//      // Terminate
//      this->simulation->shutdownAllComputeServices();
//      this->simulation->shutdownAllStorageServices();
//      this->simulation->getFileRegistryService()->stop();
//      return 0;
//    }
//};
//
//TEST_F(OneTaskTest, ExecutionWithPrePostCopies) {
//  DO_TEST_WITH_FORK(do_ExecutionWithPrePostCopies_test)
//}
//
//void OneTaskTest::do_ExecutionWithPrePostCopies_test() {
//
//  // Create and initialize a simulation
//  wrench::Simulation *simulation = new wrench::Simulation();
//  int argc = 1;
//  char **argv = (char **) calloc(1, sizeof(char *));
//  argv[0] = strdup("one_task_test");
//
//  ASSERT_THROW(simulation->launch(), std::runtime_error);
//
//  simulation->init(&argc, argv);
//
//  // Setting up the platform
//  ASSERT_THROW(simulation->launch(), std::runtime_error);
//  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
//  ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);
//
//  // Get a hostname
//  std::string hostname = simulation->getHostnameList()[0];
//
//  // Create a WMS
//  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(
//          std::unique_ptr<wrench::WMS>(new ExecutionWithPrePostCopiesAndCleanupTestWMS(this, workflow,
//                                                                                       std::unique_ptr<wrench::Scheduler>(
//                                                                                               new wrench::RandomScheduler()),
//                          hostname))));
//
//  // Create a Storage Service
//  EXPECT_NO_THROW(storage_service1 = simulation->add(
//          std::unique_ptr<wrench::SimpleStorageService>(
//                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));
//
//  // Create another Storage Service
//  EXPECT_NO_THROW(storage_service2 = simulation->add(
//          std::unique_ptr<wrench::SimpleStorageService>(
//                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));
//
//
//  // Create a Compute Service with default Storage Service #2
//  EXPECT_NO_THROW(compute_service = simulation->add(
//          std::unique_ptr<wrench::MulticoreComputeService>(
//                  new wrench::MulticoreComputeService(hostname, true, true,
//                                                      storage_service2,
//                                                      {}))));
//
//  // Create a File Registry Service
//  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
//          new wrench::FileRegistryService(hostname));
//  EXPECT_NO_THROW(simulation->setFileRegistryService(std::move(file_registry_service)));
//
//  // Staging the input_file on storage service #1
//  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service1));
//
//  // Running a "run a single task" simulation
//  EXPECT_NO_THROW(simulation->launch());
//
//  // Check that the output trace makes sense
//  ASSERT_EQ(task->getState(), wrench::WorkflowTask::COMPLETED);
//  ASSERT_EQ(task->getFailureCount(), 0);
//  ASSERT_GT(task->getStartDate(), 0.0);
//  ASSERT_GT(task->getEndDate(), 0.0);
//  ASSERT_GT(task->getEndDate(), task->getStartDate());
//
//  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *> task_completion_trace =
//          simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>();
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>().size(), 1);
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getDate(),
//            task->getEndDate());
//  ASSERT_EQ(simulation->output.getTrace<wrench::SimulationTimestampTaskCompletion>()[0]->getContent()->getTask(),
//            task);
//
//  delete simulation;
//}
