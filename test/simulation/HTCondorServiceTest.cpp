/**
 * Copyright (c) 2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <numeric>
#include <thread>
#include <chrono>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class HTCondorServiceTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file1;
    wrench::WorkflowFile *output_file2;
    wrench::WorkflowFile *output_file3;
    wrench::WorkflowFile *output_file4;
    wrench::WorkflowTask *task1;
    wrench::WorkflowTask *task2;
    wrench::WorkflowTask *task3;
    wrench::WorkflowTask *task4;
    wrench::WorkflowTask *task5;
    wrench::WorkflowTask *task6;
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service = nullptr;

    void do_StandardJobTaskTest_test();

    void do_SimpleServiceTest_test();

protected:
    HTCondorServiceTest() {

      // Create the simplest workflow
      workflow_unique_ptr = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
      workflow = workflow_unique_ptr.get();

      // Create the files
      input_file = workflow->addFile("input_file", 10.0);
      output_file1 = workflow->addFile("output_file1", 10.0);
      output_file2 = workflow->addFile("output_file2", 10.0);
      output_file3 = workflow->addFile("output_file3", 10.0);
      output_file4 = workflow->addFile("output_file4", 10.0);

      // Create the tasks
      task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0, 0);
      task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0, 0);
      task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 1.0, 0);
      task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 1.0, 0);
      task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 1.0, 0);
      task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 1.0, 0);

      // Add file-task dependencies
      task1->addInputFile(input_file);
      task2->addInputFile(input_file);
      task3->addInputFile(input_file);
      task4->addInputFile(input_file);
      task5->addInputFile(input_file);
      task6->addInputFile(input_file);

      task1->addOutputFile(output_file1);

      // Create a platform file
      std::string xml = "<?xml version='1.0'?>"
                        "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                        "<platform version=\"4.1\"> "
                        "   <zone id=\"AS0\" routing=\"Full\"> "
                        "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"/> "
                        "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"/> "
                        "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                        "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
                        "   </zone> "
                        "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;
    std::unique_ptr<wrench::Workflow> workflow_unique_ptr;
};

/**********************************************************************/
/**  STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class HTCondorStandardJobTestWMS : public wrench::WMS {

public:
    HTCondorStandardJobTestWMS(HTCondorServiceTest *test,
                               const std::set<wrench::ComputeService *> &compute_services,
                               const std::set<wrench::StorageService *> &storage_services,
                               std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1},
                                                                         {},
                                                                         {std::make_tuple(this->test->input_file,
                                                                                          this->test->storage_service,
                                                                                          wrench::ComputeService::SCRATCH)},
                                                                         {}, {});

      // Submit the 2-task job for execution
      try {
        job_manager->submitJob(two_task_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // Wait for a workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
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

      return 0;
    }
};

TEST_F(HTCondorServiceTest, HTCondorStandardJobTestWMS) {
  DO_TEST_WITH_FORK(do_StandardJobTaskTest_test);
}

void HTCondorServiceTest::do_StandardJobTaskTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("htcondor_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create list of compute services
  std::set<wrench::ComputeService *> compute_services;
  std::string execution_host = simulation->getHostnameList()[1];
  std::vector<std::string> execution_hosts;
  execution_hosts.push_back(execution_host);
  compute_services.insert(new wrench::BareMetalComputeService(
          execution_host,
          {std::make_pair(
                  execution_host,
                  std::make_tuple(wrench::Simulation::getHostNumCores(execution_host),
                                  wrench::Simulation::getHostMemoryCapacity(execution_host)))},
          100000000000.0));

  // Create a HTCondor Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::HTCondorService(hostname, "local", std::move(compute_services),
                                      {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new HTCondorStandardJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  SIMPLE SERVICE TESTS                                            **/
/**********************************************************************/

class HTCondorSimpleServiceTestWMS : public wrench::WMS {

public:
    HTCondorSimpleServiceTestWMS(HTCondorServiceTest *test,
                                 const std::set<wrench::ComputeService *> &compute_services,
                                 const std::set<wrench::StorageService *> &storage_services,
                                 std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    HTCondorServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob(
              {this->test->task1}, {},
              {std::make_tuple(this->test->input_file,
                               ((wrench::HTCondorService *) this->test->compute_service)->getLocalStorageService(),
                               wrench::ComputeService::SCRATCH)},
              {}, {});

      // Submit the 2-task job for execution
      try {
        job_manager->submitJob(two_task_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // Wait for a workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
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

      this->test->compute_service->stop();

      return 0;
    }
};

TEST_F(HTCondorServiceTest, HTCondorSimpleServiceTestWMS) {
  DO_TEST_WITH_FORK(do_SimpleServiceTest_test);
}

void HTCondorServiceTest::do_SimpleServiceTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("htcondor_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(hostname, 100.0)));

  // Create list of compute services
  std::set<wrench::ComputeService *> compute_services;
  std::string execution_host = simulation->getHostnameList()[1];
  std::vector<std::string> execution_hosts;
  execution_hosts.push_back(execution_host);
  compute_services.insert(new wrench::CloudService(hostname, execution_hosts,
                                                   100000000000.0));

  // Create a HTCondor Service
  ASSERT_THROW(simulation->add(new wrench::HTCondorService(hostname, "", {})), std::runtime_error);
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::HTCondorService(hostname, "local", std::move(compute_services),
                                      {{wrench::HTCondorServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  ((wrench::HTCondorService *) compute_service)->setLocalStorageService(storage_service);

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new HTCondorSimpleServiceTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
