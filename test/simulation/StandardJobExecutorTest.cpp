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

#define EPSILON 0.05

class StandardJobExecutorTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::Simulation *simulation;


    void do_OneSingleCoreTaskTestWMS_test();
    void do_OneMultiCoreTaskTestWMS_test();


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
      double thread_startup_overhead = 10.0;
      wrench::StandardJobExecutor *executor = new wrench::StandardJobExecutor(
              test->simulation,
              my_mailbox,
              test->simulation->getHostnameList()[0],
              job,
              {std::tuple<std::string, unsigned long>{test->simulation->getHostnameList()[0], 2}},
              nullptr,
              {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(thread_startup_overhead)}}
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
      if ((task->getEndDate() < task->getFlops() + thread_startup_overhead) ||
              (task->getEndDate() > task->getFlops() + thread_startup_overhead + EPSILON)) {
        throw std::runtime_error("Unexpected task completion time (should be around " +
                                         std::to_string(task->getFlops() + thread_startup_overhead) + " but is " +
                                         std::to_string(task->getEndDate()) + ")");
      }

      // Doe the task-stored time information look good
      if ((task->getStartDate() > EPSILON) || (task->getEndDate() > wrench::S4U_Simulation::getClock()) ||
              (wrench::S4U_Simulation::getClock() - task->getEndDate() > EPSILON)) {
        throw std::runtime_error(
                "Case 1: Unexpected task start and/or end date (start = " + std::to_string(task->getStartDate()) +
                "; end = " + std::to_string(task->getEndDate()));
      }

      // Has the output file been created?
      if (!this->test->storage_service1->lookupFile(workflow->getFileById("output_file"))) {
        throw std::runtime_error("The output file has not been stored to the specified storage service");
      }

      workflow->removeTask(task);

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



/**********************************************************************/
/**  ONE MULTI-CORE TASK SIMULATION TEST                            **/
/**********************************************************************/

class OneMultiCoreTaskTestWMS : public wrench::WMS {

public:
    OneMultiCoreTaskTestWMS(StandardJobExecutorTest *test,
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



      /** Case 1: Create a multicore task with perfect parallel efficiency that lasts one hour **/
      {
        wrench::WorkflowTask *task = this->workflow->addTask("task1", 3600, 1, 10, 1.0);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                });

        std::string my_mailbox = "TEST_CALLBACK_MAILBOX";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        wrench::StandardJobExecutor *executor = new wrench::StandardJobExecutor(
                test->simulation,
                my_mailbox,
                test->simulation->getHostnameList()[0],
                job,
                {std::tuple<std::string, unsigned long>{test->simulation->getHostnameList()[0], 6}},
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

        double after = wrench::S4U_Simulation::getClock();

        double observed_task_duration = after - before;

        double expected_duration = task->getFlops() / 6;
        // Does the task completion time make sense?
        if ((observed_task_duration < expected_duration) || (observed_task_duration > expected_duration + EPSILON)) {
          throw std::runtime_error(
                  "Case 1: Unexpected task duration (should be around " + std::to_string(task->getFlops()) + " but is " +
                  std::to_string(observed_task_duration) + ")");
        }

        workflow->removeTask(task);
      }


       /** Case 2: Create a multicore task with 50% parallel efficiency that lasts one hour **/
      {
        wrench::WorkflowTask *task = this->workflow->addTask("task1", 3600, 1, 10, 0.5);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                });

        std::string my_mailbox = "TEST_CALLBACK_MAILBOX";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        wrench::StandardJobExecutor *executor = new wrench::StandardJobExecutor(
                test->simulation,
                my_mailbox,
                test->simulation->getHostnameList()[0],
                job,
                {std::tuple<std::string, unsigned long>{test->simulation->getHostnameList()[0], 10}},
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

        double after = wrench::S4U_Simulation::getClock();

        double observed_task_duration = after - before;

        double expected_duration = task->getFlops() / (10 * task->getParallelEfficiency());

        // Does the task completion time make sense?
        if ((observed_task_duration < expected_duration) || (observed_task_duration > expected_duration  + EPSILON)) {
          throw std::runtime_error(
                  "Case 2: Unexpected task duration (should be around " + std::to_string(task->getFlops()) + " but is " +
                  std::to_string(observed_task_duration) + ")");
        }

        workflow->removeTask(task);
      }

      /** Case 3: Create a multicore task with 50% parallel efficiency and include thread startup overhead **/
      {
        wrench::WorkflowTask *task = this->workflow->addTask("task1", 3600, 1, 10, 0.5);
        task->addInputFile(workflow->getFileById("input_file"));
        task->addOutputFile(workflow->getFileById("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                });

        std::string my_mailbox = "TEST_CALLBACK_MAILBOX";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        double thread_startup_overhead = 14;
        wrench::StandardJobExecutor *executor = new wrench::StandardJobExecutor(
                test->simulation,
                my_mailbox,
                test->simulation->getHostnameList()[0],
                job,
                {std::tuple<std::string, unsigned long>{test->simulation->getHostnameList()[0], 10}},
                nullptr,
                {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(thread_startup_overhead)}}
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

        double after = wrench::S4U_Simulation::getClock();

        double observed_task_duration = after - before;

        double expected_duration = 10 * thread_startup_overhead + task->getFlops() / (10 * task->getParallelEfficiency());

        // Does the task completion time make sense?
        if ((observed_task_duration < expected_duration) || (observed_task_duration > expected_duration  + EPSILON)) {
          throw std::runtime_error(
                  "Case 3: Unexpected task duration (should be around " + std::to_string(task->getFlops()) + " but is " +
                  std::to_string(observed_task_duration) + ")");
        }

        workflow->removeTask(task);
      }


      // Terminate everything
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneMultiCoreTaskTestWMS) {
  DO_TEST_WITH_FORK(do_OneMultiCoreTaskTestWMS_test);
}

void StandardJobExecutorTest::do_OneMultiCoreTaskTestWMS_test() {

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
          std::unique_ptr<wrench::WMS>(new OneMultiCoreTaskTestWMS(this, workflow,
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

