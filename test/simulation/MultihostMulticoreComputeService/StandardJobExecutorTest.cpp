/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <random>
#include <wrench-dev.h>

#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"

#include "../../include/TestWithFork.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(standard_job_executor_test, "Log category for Simple SandardJobExecutorTest");

#define EPSILON 0.05

class StandardJobExecutorTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::Simulation *simulation;


    void do_StandardJobExecutorConstructorTest_test();

    void do_OneSingleCoreTaskTest_test();

    void do_OneSingleCoreTaskBogusPreFileCopyTest_test();

    void do_OneSingleCoreTaskMissingFileTest_test();

    void do_OneMultiCoreTaskTest_test();

    void do_DependentTasksTest_test();

    void do_TwoMultiCoreTasksTest_test();

    void do_MultiHostTest_test();

    void do_JobTerminationTestDuringAComputation_test();

    void do_JobTerminationTestDuringATransfer_test();

    void do_JobTerminationTestAtRandomTimes_test();

    void do_DEBUG_test();

    static bool isJustABitGreater(double base, double variable) {
      return ((variable > base) && (variable < base + EPSILON));
    }

protected:
    StandardJobExecutorTest() {

      // Create the simplest workflow
      workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
              "         <prop id=\"ram\" value=\"1024\"/> "
              "       </host>  "
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"0.1MBps\" latency=\"10us\"/>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
              "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
              "   </zone> "
              "</platform>";
      // Create a four-host 10-core platform file
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};


/**********************************************************************/
/**  DO CONSTRUCTOR TEST                                             **/
/**********************************************************************/

class StandardJobExecutorConstructorTestWMS : public wrench::WMS {

public:
    StandardJobExecutorConstructorTestWMS(StandardJobExecutorTest *test,
                                          const std::set<wrench::ComputeService *> &compute_services,
                                          const std::set<wrench::StorageService *> &storage_services,
                                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      std::shared_ptr<wrench::StandardJobExecutor> executor;

      // Create a sequential task that lasts one hour
      wrench::WorkflowTask *task = this->workflow->addTask("task", 3600, 1, 1, 1.0, 0);
      task->addInputFile(workflow->getFileByID("input_file"));
      task->addOutputFile(workflow->getFileByID("output_file"));

      // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
      wrench::StandardJob *job = job_manager->createStandardJob(
              {task},
              {
                      {*(task->getInputFiles().begin()),  this->test->storage_service1},
                      {*(task->getOutputFiles().begin()), this->test->storage_service2}
              },
              {},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                      workflow->getFileByID("output_file"), this->test->storage_service2,
                      this->test->storage_service1)},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("output_file"),
                                                                            this->test->storage_service2)});

      std::string my_mailbox = "test_callback_mailbox";

      double before = wrench::S4U_Simulation::getClock();
      double thread_startup_overhead = 10.0;

      bool success;

      // Create a bogus StandardJobExecutor (invalid host)
      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple("bogus", 2, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with a bogus host");
      }

      // Create a bogus StandardJobExecutor (nullptr job)
      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        nullptr,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 2, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with a nullptr job");
      }

      // Create a bogus StandardJobExecutor (no compute resources specified)
      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with no compute resources");
      }

      // Create a bogus StandardJobExecutor (no cores resources specified)
      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 0, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with zero cores on a resource");
      }

      // Create a bogus StandardJobExecutor (too many cores specified)
      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 100, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with more cores than available on a resource");
      }


      // Create a bogus StandardJobExecutor (negative RAM specified)
      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], wrench::ComputeService::ALL_CORES, -1)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with negative RAM on a resource");
      }

      // Create a bogus StandardJobExecutor (too much RAM specified)
      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple("Host4", wrench::ComputeService::ALL_CORES, 2048)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with more RAM than available on a resource");
      }


      // Create a bogus StandardJobExecutor (not enough Cores specified)
      wrench::WorkflowTask *task_too_many_cores = this->workflow->addTask("task_too_many_cores", 3600, 20, 20, 1.0, 0);
      task_too_many_cores->addInputFile(workflow->getFileByID("input_file"));
      task_too_many_cores->addOutputFile(workflow->getFileByID("output_file"));

//        // Forget the previous job!
      job_manager->forgetJob(job);

      // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
      job = job_manager->createStandardJob(
              {task_too_many_cores},
              {
                      {*(task->getInputFiles().begin()),  this->test->storage_service1},
                      {*(task->getOutputFiles().begin()), this->test->storage_service2}
              },
              {},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                      workflow->getFileByID("output_file"), this->test->storage_service2,
                      this->test->storage_service1)},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("output_file"),
                                                                            this->test->storage_service2)});

      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple("Host4", wrench::ComputeService::ALL_CORES, 100.00)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with insufficient RAM to run the job");
      }


      // Create a bogus StandardJobExecutor (not enough RAM specified)

      success = true;

      wrench::WorkflowTask *task_too_much_ram = this->workflow->addTask("task_too_much_ram", 3600, 1, 1, 1.0, 500.00);
      task_too_much_ram->addInputFile(workflow->getFileByID("input_file"));
      task_too_much_ram->addOutputFile(workflow->getFileByID("output_file"));

      // Forget the previous job!
      job_manager->forgetJob(job);

      // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
      job = job_manager->createStandardJob(
              {task_too_much_ram},
              {
                      {*(task->getInputFiles().begin()),  this->test->storage_service1},
                      {*(task->getOutputFiles().begin()), this->test->storage_service2}
              },
              {},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                      workflow->getFileByID("output_file"), this->test->storage_service2,
                      this->test->storage_service1)},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("output_file"),
                                                                            this->test->storage_service2)});

      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple("Host4", wrench::ComputeService::ALL_CORES, 100.00)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to create a standard job executor with insufficient RAM to run the job");
      }

      // Finally do one that works

      // Forget the previous job!
      job_manager->forgetJob(job);

      // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
      job = job_manager->createStandardJob(
              {task},
              {
                      {*(task->getInputFiles().begin()),  this->test->storage_service1},
                      {*(task->getOutputFiles().begin()), this->test->storage_service2}
              },
              {},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                      workflow->getFileByID("output_file"), this->test->storage_service2,
                      this->test->storage_service1)},
              {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("output_file"),
                                                                            this->test->storage_service2)});

      success = true;
      try {
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple("Host1", wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (not success) {
        throw std::runtime_error("Should  be able to create a valid standard job executor!");
      }

      executor->start(executor, true);

      // Wait for a message on my mailbox_name
      std::unique_ptr<wrench::SimulationMessage> message;
      try {
        message = wrench::S4U_Mailbox::getMessage(my_mailbox);
      } catch (std::shared_ptr<wrench::NetworkError> &cause) {
        throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
      }

      // Did we get the expected message?
      auto msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
      if (!msg) {
        throw std::runtime_error("Unexpected '" + message->getName() + "' message");
      }

      job_manager->forgetJob(job);

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, ConstructorTest) {
  DO_TEST_WITH_FORK(do_StandardJobExecutorConstructorTest_test);
}

void StandardJobExecutorTest::do_StandardJobExecutorConstructorTest_test() {

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

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new StandardJobExecutorConstructorTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
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
/**  ONE SINGLE-CORE TASK SIMULATION TEST ON ONE HOST                **/
/**********************************************************************/

class OneSingleCoreTaskTestWMS : public wrench::WMS {

public:
    OneSingleCoreTaskTestWMS(StandardJobExecutorTest *test,
                             const std::set<wrench::ComputeService *> &compute_services,
                             const std::set<wrench::StorageService *> &storage_services,
                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      {
        // Create a sequential task that lasts one hour
        wrench::WorkflowTask *task = this->workflow->addTask("task", 3600, 1, 1, 1.0, 0);
        task->addInputFile(workflow->getFileByID("input_file"));
        task->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service2}
                },
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("output_file"), this->test->storage_service2,
                        this->test->storage_service1)},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("output_file"),
                                                                              this->test->storage_service2)});

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();
        double thread_startup_overhead = 10.0;


        // Create a StandardJobExecutor that will run stuff on one host and one core
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 2, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = task->getFlops() + 3 * thread_startup_overhead;

        // Does the job completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(before + expected_duration, after)) {
          throw std::runtime_error("Unexpected job completion time (should be around " +
                                   std::to_string(before + expected_duration) + " but is " +
                                   std::to_string(after) + ")");
        }

        // Doe the task-stored time information look good
        if (!StandardJobExecutorTest::isJustABitGreater(before, task->getStartDate())) {
          throw std::runtime_error(
                  "Case 1: Unexpected task start date: " + std::to_string(task->getStartDate()));
        }

        // Note that we have to subtract the last thread startup overhead (for file deletions)
        if (!StandardJobExecutorTest::isJustABitGreater(task->getEndDate(), after - 2 * thread_startup_overhead)) {
          throw std::runtime_error(
                  "Case 1: Unexpected task end date: " + std::to_string(task->getEndDate()) +
                  " (expected: " + std::to_string(after - 2 * thread_startup_overhead) + ")");
        }

        // Has the output file been copied back to storage_service1?
        if (!this->test->storage_service1->lookupFile(workflow->getFileByID("output_file"))) {
          throw std::runtime_error("The output file has not been copied back to the specified storage service");
        }

        // Has the output file been erased from storage_service2?
        if (this->test->storage_service2->lookupFile(workflow->getFileByID("output_file"))) {
          throw std::runtime_error("The output file has not been erased from the specified storage service");
        }

        workflow->removeTask(task);
      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneSingleCoreTaskTest) {
  DO_TEST_WITH_FORK(do_OneSingleCoreTaskTest_test);
}

void StandardJobExecutorTest::do_OneSingleCoreTaskTest_test() {

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

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new OneSingleCoreTaskTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
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



/****************************************************************************/
/**  ONE SINGLE-CORE TASK SIMULATION TEST ON ONE HOST: BOGUS PRE FILE COPY **/
/****************************************************************************/

class OneSingleCoreTaskBogusPreFileCopyTestWMS : public wrench::WMS {

public:
    OneSingleCoreTaskBogusPreFileCopyTestWMS(StandardJobExecutorTest *test,
                                             const std::set<wrench::ComputeService *> &compute_services,
                                             const std::set<wrench::StorageService *> &storage_services,
                                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // WARNING: The StandardJobExecutor unique_ptr is declared here, so that
      // it's not automatically freed after the next basic block is over. In the internals
      // of WRENCH, this is typically take care in various ways (e.g., keep a list of "finished" executors)
      std::shared_ptr<wrench::StandardJobExecutor> executor;

      {
        // Create a sequential task that lasts one hour and requires 1 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 3600, 1, 1, 1.0, 0);
        task->addInputFile(workflow->getFileByID("input_file"));
        task->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service2}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service2,
                        this->test->storage_service1)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("output_file"),
                                                                              this->test->storage_service2)});


        std::string my_mailbox = "test_callback_mailbox";


        // Create a StandardJobExecutor that will run stuff on one host and two core
        double thread_startup_overhead = 10.0;
        bool success = true;
        try {
          executor = std::shared_ptr<wrench::StandardJobExecutor>(new wrench::StandardJobExecutor(
                  test->simulation,
                  my_mailbox,
                  test->simulation->getHostnameList()[0],
                  job,
                  {std::make_tuple(test->simulation->getHostnameList()[0], 2, wrench::ComputeService::ALL_RAM)},
                  nullptr,
                  false,
                  {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                          thread_startup_overhead)}}, {}
          ));
        } catch (std::runtime_error &e) {
          throw std::runtime_error("Should have been able to create standard job executor!");
        }
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = dynamic_cast<wrench::StandardJobExecutorFailedMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        if (msg->cause->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
          throw std::runtime_error("Unexpected failure cause type " +
                                   std::to_string(msg->cause->getCauseType()) + " (" + msg->cause->toString() + ")");
        }

        auto real_cause = (wrench::StorageServiceFileAlreadyThere *) msg->cause.get();
        if (real_cause->getFile() != workflow->getFileByID("input_file")) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
        }
        if (real_cause->getStorageService() != this->test->storage_service2) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
        }

//        this->test->storage_service2->deleteFile(workflow->getFileByID("input_file"));
        workflow->removeTask(task);

      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneSingleCoreTaskBogusPreFileCopyTest) {
  DO_TEST_WITH_FORK(do_OneSingleCoreTaskBogusPreFileCopyTest_test);
}

void StandardJobExecutorTest::do_OneSingleCoreTaskBogusPreFileCopyTest_test() {

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

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new OneSingleCoreTaskBogusPreFileCopyTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
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





/*************************************************************************/
/**  ONE SINGLE-CORE TASK SIMULATION TEST ON ONE HOST: MISSING FILE **/
/*************************************************************************/

class OneSingleCoreTaskMissingFileTestWMS : public wrench::WMS {

public:
    OneSingleCoreTaskMissingFileTestWMS(StandardJobExecutorTest *test,
                                        const std::set<wrench::ComputeService *> &compute_services,
                                        const std::set<wrench::StorageService *> &storage_services,
                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // WARNING: The StandardJobExecutor unique_ptr is declared here, so that
      // it's not automatically freed after the next basic block is over. In the internals
      // of WRENCH, this is typically take care in various ways (e.g., keep a list of "finished" executors)
      std::shared_ptr<wrench::StandardJobExecutor> executor;

      {
        // Create a sequential task that lasts one hour and requires 1 cores
        wrench::WorkflowTask *task = this->workflow->addTask("task", 3600, 1, 1, 1.0, 0);
        task->addInputFile(workflow->getFileByID("input_file"));
        task->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

        wrench::StandardJob *job = job_manager->createStandardJob(
                {task},
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service2},
                        {*(task->getOutputFiles().begin()), this->test->storage_service2}
                },
                {},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("output_file"),
                                                                              this->test->storage_service2)});


        std::string my_mailbox = "test_callback_mailbox";

        // Create a StandardJobExecutor that will run stuff on one host and two core
        double thread_startup_overhead = 10.0;
        bool success = true;

        try {
          executor = std::shared_ptr<wrench::StandardJobExecutor>(new wrench::StandardJobExecutor(
                  test->simulation,
                  my_mailbox,
                  test->simulation->getHostnameList()[1],
                  job,
                  {std::make_tuple(test->simulation->getHostnameList()[1], 2, wrench::ComputeService::ALL_RAM)},
                  nullptr,
                  false,
                  {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                          thread_startup_overhead)}}, {}
          ));
        } catch (std::runtime_error &e) {
          throw std::runtime_error("Should have been able to create standard job executor!");
        }
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        wrench::StandardJobExecutorFailedMessage *msg = dynamic_cast<wrench::StandardJobExecutorFailedMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        if (msg->cause->getCauseType() != wrench::FailureCause::FILE_NOT_FOUND) {
          throw std::runtime_error("Unexpected failure cause type " +
                                   std::to_string(msg->cause->getCauseType()) + " (" + msg->cause->toString() + ")");
        }

        wrench::FileNotFound *real_cause = (wrench::FileNotFound *) msg->cause.get();
        std::string error_msg = real_cause->toString();
        if (real_cause->getFile() != workflow->getFileByID("input_file")) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct file");
        }
        if (real_cause->getStorageService() != this->test->storage_service2) {
          throw std::runtime_error(
                  "Got the expected 'file not found' exception, but the failure cause does not point to the correct storage service");
        }

        workflow->removeTask(task);

      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneSingleCoreTaskMissingFileTest) {
  DO_TEST_WITH_FORK(do_OneSingleCoreTaskMissingFileTest_test);
}

void StandardJobExecutorTest::do_OneSingleCoreTaskMissingFileTest_test() {

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

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service = nullptr;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Service
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new OneSingleCoreTaskMissingFileTestWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService(hostname));

  // Create two workflow files
  wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000000.0);
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
/**  STANDARD JOB WITH DEPENDENT TASKS                               **/
/**********************************************************************/

class DependentTasksTestWMS : public wrench::WMS {

public:
    DependentTasksTestWMS(StandardJobExecutorTest *test,
                          const std::set<wrench::ComputeService *> &compute_services,
                          const std::set<wrench::StorageService *> &storage_services,
                          std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      {

        //   t1 -> f1 -> t2 -> f2 -> t4
        //         f1 -> t3 -> f3 -> t4

        // Create two workflow files
        wrench::WorkflowFile *f1 = this->workflow->addFile("f1", 1.0);
        wrench::WorkflowFile *f2 = this->workflow->addFile("f2", 1.0);
        wrench::WorkflowFile *f3 = this->workflow->addFile("f3", 1.0);

        // Create sequential tasks
        wrench::WorkflowTask *t1 = this->workflow->addTask("t1", 100, 1, 1, 1.0, 0);
        wrench::WorkflowTask *t2 = this->workflow->addTask("t2", 100, 1, 1, 1.0, 0);
        wrench::WorkflowTask *t3 = this->workflow->addTask("t3", 150, 1, 1, 1.0, 0);
        wrench::WorkflowTask *t4 = this->workflow->addTask("t4", 100, 1, 1, 1.0, 0);

        t1->addOutputFile(f1);
        t2->addInputFile(f1);
        t3->addInputFile(f1);
        t2->addOutputFile(f2);
        t3->addOutputFile(f3);
        t4->addInputFile(f2);
        t4->addInputFile(f3);

        // Create a BOGUS StandardJob (just for testing)
        bool success = true;
        try {
          wrench::StandardJob *job = job_manager->createStandardJob(
                  {t1, t2, t4},
                  {},
                  {},
                  {},
                  {});
        } catch (std::invalid_argument &e) {
          success = false;
        }
        if (success) {
          throw std::runtime_error("Should not be able to create a standard job with t1, t2, t3 only");
        }


        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                {t1, t2, t3, t4},
                {},
                {},
                {},
                {});

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();
        double thread_startup_overhead = 0.0;

        // Create a StandardJobExecutor that will run stuff on one host and 2 cores
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 2, wrench::ComputeService::ALL_RAM)},
                        this->test->storage_service1 , // This should be a scratch space of a compute service, but since this
                        //standard job executor is being created direclty (not by any Compute Service), we pass a dummy storage as a scratch space
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}

                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        if (!StandardJobExecutorTest::isJustABitGreater(100, t1->getEndDate())) {
          throw std::runtime_error("Unexpected completion time for t1: " +
                                   std::to_string(t1->getEndDate()) + "(should be 100)");
        }
        if (!StandardJobExecutorTest::isJustABitGreater(200, t2->getEndDate())) {
          throw std::runtime_error("Unexpected completion time for t2: " +
                                   std::to_string(t2->getEndDate()) + "(should be 200)");
        }
        if (!StandardJobExecutorTest::isJustABitGreater(250, t3->getEndDate())) {
          throw std::runtime_error("Unexpected completion time for t3: " +
                                   std::to_string(t3->getEndDate()) + "(should be 250)");
        }
        if (!StandardJobExecutorTest::isJustABitGreater(350, t4->getEndDate())) {
          throw std::runtime_error("Unexpected completion time for t4: " +
                                   std::to_string(t4->getEndDate()) + "(should be 350)");
        }

        if ((t1->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED) ||
            (t2->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED) ||
            (t3->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED) ||
            (t4->getInternalState() != wrench::WorkflowTask::InternalState::TASK_COMPLETED)) {
          throw std::runtime_error("Unexpected task states!");
        }

        workflow->removeTask(t1);
        workflow->removeTask(t2);
        workflow->removeTask(t3);
        workflow->removeTask(t4);
      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, DependentTasksTest) {
  DO_TEST_WITH_FORK(do_DependentTasksTest_test);
}

void StandardJobExecutorTest::do_DependentTasksTest_test() {

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

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Compute Service
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new DependentTasksTestWMS(
                  this,  {compute_service}, {storage_service1}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


// TODO:  SPLIT THE TESTS

/**********************************************************************/
/**  ONE MULTI-CORE TASK SIMULATION TEST                            **/
/**********************************************************************/

class OneMultiCoreTaskTestWMS : public wrench::WMS {

public:
    OneMultiCoreTaskTestWMS(StandardJobExecutorTest *test,
                            const std::set<wrench::ComputeService *> &compute_services,
                            const std::set<wrench::StorageService *> &storage_services,
                            std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      /** Case 1: Create a multicore task with perfect parallel efficiency that lasts one hour **/
      {
        wrench::WorkflowTask *task = this->workflow->addTask("task1", 3600, 1, 10, 1.0, 0);
        task->addInputFile(workflow->getFileByID("input_file"));
        task->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                });

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[1],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[1], 6, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = task->getFlops() / 6;
        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 1: Unexpected task duration (should be around " + std::to_string(expected_duration) +
                  " but is " +
                  std::to_string(observed_duration) + ")");
        }

        this->test->storage_service1->deleteFile(workflow->getFileByID("output_file"));

        workflow->removeTask(task);
      }

      /** Case 2: Create a multicore task with 50% parallel efficiency that lasts one hour **/
      {
        wrench::WorkflowTask *task = this->workflow->addTask("task1", 3600, 1, 10, 0.5, 0);
        task->addInputFile(workflow->getFileByID("input_file"));
        task->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                });

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[1],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[1], 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          auto *msg = dynamic_cast<wrench::StandardJobExecutorFailedMessage *>(message.get());
          std::string error_msg = msg->cause->toString();
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = task->getFlops() / (10 * task->getParallelEfficiency());

        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 2: Unexpected task duration (should be around " + std::to_string(expected_duration) +
                  " but is " +
                  std::to_string(observed_duration) + ")");
        }

        workflow->removeTask(task);

        this->test->storage_service1->deleteFile(workflow->getFileByID("output_file"));
      }

      /** Case 3: Create a multicore task with 50% parallel efficiency and include thread startup overhead **/
      {
        wrench::WorkflowTask *task = this->workflow->addTask("task1", 3600, 1, 10, 0.5, 0);
        task->addInputFile(workflow->getFileByID("input_file"));
        task->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob
        wrench::StandardJob *job = job_manager->createStandardJob(
                task,
                {
                        {*(task->getInputFiles().begin()),  this->test->storage_service1},
                        {*(task->getOutputFiles().begin()), this->test->storage_service1}
                });

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and 6 core
        double thread_startup_overhead = 14;
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[1],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[1], 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, std::to_string(
                                thread_startup_overhead)}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          std::string error_msg = cause->toString();
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration =
                10 * thread_startup_overhead + task->getFlops() / (10 * task->getParallelEfficiency());

        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 3: Unexpected job duration (should be around " + std::to_string(expected_duration) +
                  " but is " +
                  std::to_string(observed_duration) + ")");
        }

        workflow->removeTask(task);

        this->test->storage_service1->deleteFile(workflow->getFileByID("output_file"));
      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, OneMultiCoreTaskTest) {
  DO_TEST_WITH_FORK(do_OneMultiCoreTaskTest_test);
}

void StandardJobExecutorTest::do_OneMultiCoreTaskTest_test() {

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

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new OneMultiCoreTaskTestWMS(
                  this,  {compute_service}, {storage_service1}, hostname)));

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
/**  TWO MULTI-CORE TASKS SIMULATION TEST ON ONE HOST               **/
/**********************************************************************/

class TwoMultiCoreTasksTestWMS : public wrench::WMS {

public:
    TwoMultiCoreTasksTestWMS(StandardJobExecutorTest *test,
                             const std::set<wrench::ComputeService *> &compute_services,
                             const std::set<wrench::StorageService *> &storage_services,
                             std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      /** Case 1: Create two tasks that will run in sequence with the default scheduling options **/
      {
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 2, 6, 1.0, 0);
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 300, 6, 6, 1.0, 0);
        task1->addInputFile(workflow->getFileByID("input_file"));
        task1->addOutputFile(workflow->getFileByID("output_file"));
        task2->addInputFile(workflow->getFileByID("input_file"));

        // Create a StandardJob with both tasks
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1, task2},
                {
                        {workflow->getFileByID("input_file"),  this->test->storage_service1},
                        {workflow->getFileByID("output_file"), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("input_file"),
                                                                              this->test->storage_service2)}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = task1->getFlops() / 6 + task2->getFlops() / 6;

        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 1: Unexpected job duration (should be around " +
                  std::to_string(expected_duration) + " but is " +
                  std::to_string(observed_duration) + ")");
        }

        // Do individual task completion times make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6, task1->getEndDate())) {
          throw std::runtime_error("Case 1: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
        }

        if (!StandardJobExecutorTest::isJustABitGreater(task1->getFlops() / 6 + task2->getFlops() / 6,
                                                        task2->getEndDate())) {
          throw std::runtime_error("Case 1: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
        }

        workflow->removeTask(task1);
        workflow->removeTask(task2);

        this->test->storage_service1->deleteFile(workflow->getFileByID("output_file"));
      }


      /** Case 2: Create two tasks that will run in parallel with the default scheduling options **/
      {
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 6, 6, 1.0, 0);
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 300, 2, 6, 1.0, 0);
        task1->addInputFile(workflow->getFileByID("input_file"));
        task1->addOutputFile(workflow->getFileByID("output_file"));
        task2->addInputFile(workflow->getFileByID("input_file"));

        // Create a StandardJob with both tasks
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1, task2},
                {
                        {workflow->getFileByID("input_file"),  this->test->storage_service1},
                        {workflow->getFileByID("output_file"), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("input_file"),
                                                                              this->test->storage_service2)}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = MAX(task1->getFlops() / 6, task2->getFlops() / 4);

        // Does the overall completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 2: Unexpected job duration (should be around " +
                  std::to_string(expected_duration) + " but is " +
                  std::to_string(observed_duration) + ")");
        }

        // Do individual task completion times make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6, task1->getEndDate())) {
          throw std::runtime_error("Case 2: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
        }

        if (!StandardJobExecutorTest::isJustABitGreater(before + task2->getFlops() / 4, task2->getEndDate())) {
          throw std::runtime_error("Case 2: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
        }

        workflow->removeTask(task1);
        workflow->removeTask(task2);
        this->test->storage_service1->deleteFile(workflow->getFileByID("output_file"));

      }


      /** Case 3: Create three tasks that will run in parallel and then sequential with the default scheduling options **/
      {
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 6, 6, 1.0, 0);
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 400, 2, 6, 1.0, 0);
        wrench::WorkflowTask *task3 = this->workflow->addTask("task3", 300, 10, 10, 0.6, 0);
        task1->addInputFile(workflow->getFileByID("input_file"));
        task1->addOutputFile(workflow->getFileByID("output_file"));
        task2->addInputFile(workflow->getFileByID("input_file"));
        task3->addInputFile(workflow->getFileByID("input_file"));

        // Create a StandardJob with all three tasks
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1, task2, task3},
                {
                        {workflow->getFileByID("input_file"),  this->test->storage_service1},
                        {workflow->getFileByID("output_file"), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("input_file"),
                                                                              this->test->storage_service2)}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = MAX(task1->getFlops() / 6, task2->getFlops() / 4) +
                                   task3->getFlops() / (task3->getParallelEfficiency() * 10);

        // Does the job completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 3: Unexpected job duration (should be around " +
                  std::to_string(expected_duration) + " but is " +
                  std::to_string(observed_duration) + ")");
        }

        // Do the individual task completion times make sense
        if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6.0, task1->getEndDate())) {
          throw std::runtime_error("Case 3: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
        }
        if (!StandardJobExecutorTest::isJustABitGreater(before + task2->getFlops() / 4.0, task2->getEndDate())) {
          throw std::runtime_error("Case 3: Unexpected task1 end date: " + std::to_string(task2->getEndDate()));
        }
        if (!StandardJobExecutorTest::isJustABitGreater(
                task1->getEndDate() + task3->getFlops() / (task3->getParallelEfficiency() * 10.0),
                task3->getEndDate())) {
          throw std::runtime_error("Case 3: Unexpected task3 end date: " + std::to_string(task3->getEndDate()));
        }

        workflow->removeTask(task1);
        workflow->removeTask(task2);
        workflow->removeTask(task3);
        this->test->storage_service1->deleteFile(workflow->getFileByID("output_file"));

      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, TwoMultiCoreTasksTest) {
  DO_TEST_WITH_FORK(do_TwoMultiCoreTasksTest_test);
}

void StandardJobExecutorTest::do_TwoMultiCoreTasksTest_test() {

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

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a Storage Services
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Services
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new TwoMultiCoreTasksTestWMS(
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
/**  MULTI-HOST TEST                                                 **/
/**********************************************************************/

class MultiHostTestWMS : public wrench::WMS {

public:
    MultiHostTestWMS(StandardJobExecutorTest *test,
                     const std::set<wrench::ComputeService *> &compute_services,
                     const std::set<wrench::StorageService *> &storage_services,
                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }



    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      // WARNING: The StandardJobExecutor unique_ptr is declared here, so that
      // it's not automatically freed after the next basic block is over. In the internals
      // of WRENCH, this is typically take care in various ways (e.g., keep a list of "finished" executors)
      std::shared_ptr<wrench::StandardJobExecutor> executor;

      /** Case 1: Create two tasks that will each run on a different host **/
      {
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 6, 6, 1.0, 0);
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 3600, 6, 6, 1.0, 0);
        task1->addInputFile(workflow->getFileByID("input_file"));
        task1->addOutputFile(workflow->getFileByID("output_file"));
        task2->addInputFile(workflow->getFileByID("input_file"));

        // Create a StandardJob with both tasks
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1, task2},
                {
                        {workflow->getFileByID("input_file"),  this->test->storage_service1},
                        {workflow->getFileByID("output_file"), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("input_file"),
                                                                              this->test->storage_service2)}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 10, wrench::ComputeService::ALL_RAM),
                         std::make_tuple(test->simulation->getHostnameList()[1], 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          throw std::runtime_error("Unexpected '" + message->getName() + "' message");
        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = MAX(task1->getFlops() / 6, task2->getFlops() / 6);

        // Does the task completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 1: Unexpected job duration (should be around " +
                  std::to_string(expected_duration) + " but is " +
                  std::to_string(observed_duration) + ")");
        }

        // Do individual task completion times make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops() / 6, task1->getEndDate())) {
          throw std::runtime_error("Case 1: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
        }

        if (!StandardJobExecutorTest::isJustABitGreater(task2->getFlops() / 6, task2->getEndDate())) {
          throw std::runtime_error("Case 1: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
        }

        workflow->removeTask(task1);
        workflow->removeTask(task2);
        this->test->storage_service1->deleteFile(workflow->getFileByID("output_file"));

      }

      /** Case 2: Create 4 tasks that will run in best fit manner **/
      {
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 6, 6, 1.0, 0);
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 1000, 2, 2, 1.0, 0);
        wrench::WorkflowTask *task3 = this->workflow->addTask("task3", 800, 7, 7, 1.0, 0);
        wrench::WorkflowTask *task4 = this->workflow->addTask("task4", 600, 2, 2, 1.0, 0);
        task1->addInputFile(workflow->getFileByID("input_file"));
        task1->addOutputFile(workflow->getFileByID("output_file"));
        task2->addInputFile(workflow->getFileByID("input_file"));

        // Create a StandardJob with both tasks
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1, task2, task3, task4},
                {
                        {workflow->getFileByID("input_file"),  this->test->storage_service1},
                        {workflow->getFileByID("output_file"), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("input_file"),
                                                                              this->test->storage_service2)}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        executor = std::shared_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        test->simulation->getHostnameList()[0],
                        job,
                        {std::make_tuple(test->simulation->getHostnameList()[0], 10, wrench::ComputeService::ALL_RAM),
                         std::make_tuple(test->simulation->getHostnameList()[1], 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Wait for a message on my mailbox_name
        std::unique_ptr<wrench::SimulationMessage> message;
        try {
          message = wrench::S4U_Mailbox::getMessage(my_mailbox);
        } catch (std::shared_ptr<wrench::NetworkError> &cause) {
          throw std::runtime_error("Network error while getting reply from StandardJobExecutor!" + cause->toString());
        }

        // Did we get the expected message?
        auto *msg = dynamic_cast<wrench::StandardJobExecutorDoneMessage *>(message.get());
        if (!msg) {
          auto *msg = dynamic_cast<wrench::StandardJobExecutorFailedMessage *>(message.get());
          std::string error_msg = msg->cause->toString();

          throw std::runtime_error("Unexpected '" + msg->cause->toString() + "' error");

        }

        double after = wrench::S4U_Simulation::getClock();

        double observed_duration = after - before;

        double expected_duration = MAX(MAX(MAX(task1->getFlops() / 6, task2->getFlops() / 2), task4->getFlops() / 2),
                                       task3->getFlops() / 8);

        // Does the overall completion time make sense?
        if (!StandardJobExecutorTest::isJustABitGreater(expected_duration, observed_duration)) {
          throw std::runtime_error(
                  "Case 2: Unexpected job duration (should be around " +
                  std::to_string(expected_duration) + " but is " +
                  std::to_string(observed_duration) + ")");
        }

//        // Do individual task completion times make sense?
//        if (!StandardJobExecutorTest::isJustABitGreater(before + task1->getFlops()/6, task1->getEndDate())) {
//          throw std::runtime_error("Case 2: Unexpected task1 end date: " + std::to_string(task1->getEndDate()));
//        }
//
//        if (!StandardJobExecutorTest::isJustABitGreater(before + task2->getFlops()/4, task2->getEndDate())) {
//          throw std::runtime_error("Case 2: Unexpected task2 end date: " + std::to_string(task2->getEndDate()));
//        }

        workflow->removeTask(task1);
        workflow->removeTask(task2);
        workflow->removeTask(task3);
        workflow->removeTask(task4);
      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, MultiHostTest) {
  DO_TEST_WITH_FORK(do_MultiHostTest_test);
}

void StandardJobExecutorTest::do_MultiHostTest_test() {

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

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService(hostname,
                                                       {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a Storage Services
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create another Storage Services
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new MultiHostTestWMS(
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
/**  TERMINATION TEST    #1  (DURING COMPUTATION)                    **/
/**********************************************************************/

class JobTerminationTestDuringAComputationWMS : public wrench::WMS {

public:
    JobTerminationTestDuringAComputationWMS(StandardJobExecutorTest *test,
                                            const std::set<wrench::ComputeService *> &compute_services,
                                            const std::set<wrench::StorageService *> &storage_services,
                                            std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      /**  Create a 4-task job and kill it **/
      {
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 6, 6, 1.0, 0);
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 1000, 2, 2, 1.0, 0);
        wrench::WorkflowTask *task3 = this->workflow->addTask("task3", 800, 7, 7, 1.0, 0);
        wrench::WorkflowTask *task4 = this->workflow->addTask("task4", 600, 2, 2, 1.0, 0);
        task1->addInputFile(workflow->getFileByID("input_file"));
        task1->addOutputFile(workflow->getFileByID("output_file"));
        task2->addInputFile(workflow->getFileByID("input_file"));
        task2->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob with both tasks
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1, task2, task3, task4},
//                {task1},
                {
                        {workflow->getFileByID("input_file"),  this->test->storage_service1},
                        {workflow->getFileByID("output_file"), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("input_file"),
                                                                              this->test->storage_service2)}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        "Host3",
                        job,
                        {std::make_tuple("Host3", 10, wrench::ComputeService::ALL_RAM),
                         std::make_tuple("Host4", 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Sleep 1 second
        wrench::Simulation::sleep(5);

        // Terminate the job
        executor->kill();

        // We should be good now, with nothing running

        workflow->removeTask(task1);
        workflow->removeTask(task2);
        workflow->removeTask(task3);
        workflow->removeTask(task4);

      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, JobTerminationTestDuringAComputation) {
  DO_TEST_WITH_FORK(do_JobTerminationTestDuringAComputation_test);
}

void StandardJobExecutorTest::do_JobTerminationTestDuringAComputation_test() {

  // Create and initialize a simulation
  simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService("Host3",
                                                       {std::make_tuple("Host3", wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a Storage Services
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService("Host4", 10000000000000.0)));

  // Create another Storage Services
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService("Host4", 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new JobTerminationTestDuringAComputationWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, "Host3")));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService("Host3"));

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
/**  TERMINATION TEST    #2 (DURING A TRANSFER)                      **/
/**********************************************************************/

class JobTerminationTestDuringATransferWMS : public wrench::WMS {

public:
    JobTerminationTestDuringATransferWMS(StandardJobExecutorTest *test,
                                         const std::set<wrench::ComputeService *> &compute_services,
                                         const std::set<wrench::StorageService *> &storage_services,
                                         std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }


private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();


      /**  Create a 4-task job and kill it **/
      {
        wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 6, 6, 1.0, 0);
        wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 1000, 2, 2, 1.0, 0);
        wrench::WorkflowTask *task3 = this->workflow->addTask("task3", 800, 7, 7, 1.0, 0);
        wrench::WorkflowTask *task4 = this->workflow->addTask("task4", 600, 2, 2, 1.0, 0);
        task1->addInputFile(workflow->getFileByID("input_file"));
        task1->addOutputFile(workflow->getFileByID("output_file"));
        task2->addInputFile(workflow->getFileByID("input_file"));
        task2->addOutputFile(workflow->getFileByID("output_file"));

        // Create a StandardJob with both tasks
        wrench::StandardJob *job = job_manager->createStandardJob(
                {task1, task2, task3, task4},
                {
                        {workflow->getFileByID("input_file"),  this->test->storage_service1},
                        {workflow->getFileByID("output_file"), this->test->storage_service1}
                },
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
                        workflow->getFileByID("input_file"), this->test->storage_service1,
                        this->test->storage_service2)},
                {},
                {std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(workflow->getFileByID("input_file"),
                                                                              this->test->storage_service2)}
        );

        std::string my_mailbox = "test_callback_mailbox";

        double before = wrench::S4U_Simulation::getClock();

        // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
        std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                new wrench::StandardJobExecutor(
                        test->simulation,
                        my_mailbox,
                        "Host3",
                        job,
                        {std::make_tuple("Host3", 10, wrench::ComputeService::ALL_RAM),
                         std::make_tuple("Host4", 10, wrench::ComputeService::ALL_RAM)},
                        nullptr,
                        false,
                        {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                ));
        executor->start(executor, true);

        // Sleep 48.20 second
        wrench::Simulation::sleep(48.20);

        // Terminate the job
        executor->kill();

        // We should be good now, with nothing running

        workflow->removeTask(task1);
        workflow->removeTask(task2);
        workflow->removeTask(task3);
        workflow->removeTask(task4);
      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, JobTerminationTestDuringATransfer) {
  DO_TEST_WITH_FORK(do_JobTerminationTestDuringATransfer_test);
}

void StandardJobExecutorTest::do_JobTerminationTestDuringATransfer_test() {

  // Create and initialize a simulation
  simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService("Host3",
                                                       {std::make_tuple("Host3", wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a Storage Services
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService("Host4", 10000000000000.0)));

  // Create another Storage Services
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService("Host4", 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new JobTerminationTestDuringATransferWMS(
                  this, {compute_service}, {storage_service1, storage_service2}, "Host3")));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService("Host3"));

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
/**  TERMINATION TEST    #3 (RANDOM TIME)                            **/
/**********************************************************************/

class JobTerminationTestAtRandomTimesWMS : public wrench::WMS {

public:
    JobTerminationTestAtRandomTimesWMS(StandardJobExecutorTest *test,
                                       const std::set<wrench::ComputeService *> &compute_services,
                                       const std::set<wrench::StorageService *> &storage_services,
                                       std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    StandardJobExecutorTest *test;

    int main() {

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      //Type of random number distribution
      std::uniform_real_distribution<double> dist(0, 600);  //(min, max)
      //Mersenne Twister: Good quality random number generator
      std::mt19937 rng;
      //Initialize with non-deterministic seeds
      rng.seed(std::random_device{}());

      for (int trial = 0; trial < 500; trial++) {
        WRENCH_INFO("Trial %d", trial);

        /**  Create a 4-task job and kill it **/
        {
          wrench::WorkflowTask *task1 = this->workflow->addTask("task1", 3600, 6, 6, 1.0, 0);
          wrench::WorkflowTask *task2 = this->workflow->addTask("task2", 1000, 2, 2, 1.0, 0);
          wrench::WorkflowTask *task3 = this->workflow->addTask("task3", 800, 7, 7, 1.0, 0);
          wrench::WorkflowTask *task4 = this->workflow->addTask("task4", 600, 2, 2, 1.0, 0);

          task1->addInputFile(workflow->getFileByID("input_file"));
//          task1->addOutputFile(workflow->getFileByID("output_file"));

          // Create a StandardJob with both tasks
          wrench::StandardJob *job = job_manager->createStandardJob(
//                {task1, task2, task3, task4},
                  {task1, task3},
//                {task1},
                  {
                          {workflow->getFileByID("input_file"), this->test->storage_service1},
//                          {workflow->getFileByID("output_file"), this->test->storage_service1}
                  },
                  {
                          std::make_tuple(workflow->getFileByID("input_file"), this->test->storage_service1,
                                          this->test->storage_service2)
//                        std::tuple<wrench::WorkflowFile *, wrench::StorageService *, wrench::StorageService *>(
//                          workflow->getFileByID("input_file"), this->test->storage_service1,
//                          this->test->storage_service2)
                  },
                  {},
                  {
                          std::tuple<wrench::WorkflowFile *, wrench::StorageService *>(
                                  workflow->getFileByID("input_file"),
                                  this->test->storage_service2)
                  }
          );

          std::string my_mailbox = "test_callback_mailbox";

          double before = wrench::S4U_Simulation::getClock();
          WRENCH_INFO("BEFORE = %lf", before);

          // Create a StandardJobExecutor that will run stuff on one host and all 10 cores
          std::shared_ptr<wrench::StandardJobExecutor> executor = std::unique_ptr<wrench::StandardJobExecutor>(
                  new wrench::StandardJobExecutor(
                          test->simulation,
                          my_mailbox,
                          "Host3",
                          job,
                          {std::make_tuple("Host3", 10, wrench::ComputeService::ALL_RAM),
                           std::make_tuple("Host4", 10, wrench::ComputeService::ALL_RAM)},
                          nullptr,
                          false,
                          {{wrench::StandardJobExecutorProperty::THREAD_STARTUP_OVERHEAD, "0"}}, {}
                  ));
          executor->start(executor, true);

          // Sleep some random number of seconds
          double sleep_time = dist(rng);
          WRENCH_INFO("Sleeping for %.3lf seconds", sleep_time);
          wrench::Simulation::sleep(sleep_time);

          // Terminate the executor
          WRENCH_INFO("Killing the standard executor");
          executor->kill();

          // We should be good now, with nothing running
          workflow->removeTask(task1);
          workflow->removeTask(task2);
          workflow->removeTask(task3);
          workflow->removeTask(task4);
        }
      }

      return 0;
    }
};

TEST_F(StandardJobExecutorTest, JobTerminationTestAtRandomTimes) {
  DO_TEST_WITH_FORK(do_JobTerminationTestAtRandomTimes_test);
}

void StandardJobExecutorTest::do_JobTerminationTestAtRandomTimes_test() {

  // Create and initialize a simulation
  simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a Compute Service (we don't use it)
  wrench::ComputeService *compute_service = nullptr;
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::MultihostMulticoreComputeService("Host3",
                                                       {std::make_tuple("Host3", wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                       {})));
  // Create a Storage Services
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService("Host4", 10000000000000.0)));

  // Create another Storage Services
  EXPECT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService("Host4", 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new JobTerminationTestAtRandomTimesWMS(
                  this,  {compute_service}, {storage_service1, storage_service2}, "Host3")));

  EXPECT_NO_THROW(wms->addWorkflow(workflow.get()));

  simulation->add(new wrench::FileRegistryService("Host3"));

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

