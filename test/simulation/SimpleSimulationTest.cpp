/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../include/TestWithFork.h"

class SimpleSimulationTest : public ::testing::Test {

public:
    wrench::Workflow *workflow;
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

    void do_getReadyTasksTest_test();

protected:

    SimpleSimulationTest() {
      // Create the simplest workflow
      workflow = new wrench::Workflow();

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
      task1->setClusterID("ID1");
      task2->setClusterID("ID1");
      task3->setClusterID("ID1");
      task4->setClusterID("ID2");
      task5->setClusterID("ID2");

      // Add file-task dependencies
      task1->addInputFile(input_file);
      task2->addInputFile(input_file);
      task3->addInputFile(input_file);
      task4->addInputFile(input_file);
      task5->addInputFile(input_file);
      task6->addInputFile(input_file);

      task1->addOutputFile(output_file1);
      task2->addOutputFile(output_file2);
      task3->addOutputFile(output_file3);
      task4->addOutputFile(output_file4);
      task5->addOutputFile(output_file3);
      task6->addOutputFile(output_file4);

      workflow->addControlDependency(task4, task5);

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

    std::string platform_file_path = "/tmp/platform.xml";
};

/**********************************************************************/
/**            GET READY TASKS SIMULATION TEST ON ONE HOST           **/
/**********************************************************************/

class SimpleSimulationReadyTasksTestWMS : public wrench::WMS {

public:
    SimpleSimulationReadyTasksTestWMS(SimpleSimulationTest *test,
                                      const std::set<wrench::ComputeService *> &compute_services,
                                      const std::set<wrench::StorageService *> &storage_services,
                                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    SimpleSimulationTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a file registry service
      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      std::vector<wrench::WorkflowTask *> tasks = this->test->workflow->getReadyTasks();
      if (tasks.size() != 5) {
        throw std::runtime_error("Should have five tasks ready to run, due to dependencies");
      }

      std::map<std::string, std::vector<wrench::WorkflowTask *>> clustered_tasks = this->test->workflow->getReadyClusters();
      if (clustered_tasks.size() != 3) {
        throw std::runtime_error("Should have exactly three clusters");
      }

      for (auto task : tasks) {
        try {
          wrench::StandardJob *two_task_job = job_manager->createStandardJob({task}, {},
                                                                             {std::make_tuple(this->test->input_file, this->test->storage_service, wrench::ComputeService::SCRATCH)},
                                                                             {}, {});
          auto cs = (wrench::CloudService *) this->test->compute_service;
          std::string execution_host = cs->getExecutionHosts()[0];
          cs->createVM(execution_host, 2, 10);

          job_manager->submitJob(two_task_job, this->test->compute_service);
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error(e.what());
        }
      }

      // Wait for a workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event;
      try {
        event = this->getWorkflow()->waitForNextExecutionEvent();
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

      return 0;
    }
};

TEST_F(SimpleSimulationTest, SimpleSimulationReadyTasksTestWMS) {
  DO_TEST_WITH_FORK(do_getReadyTasksTest_test);
}

void SimpleSimulationTest::do_getReadyTasksTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("cloud_service_test");

  EXPECT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service (note the BOGUS property, which is for testing puposes
  //  and doesn't matter because we do not stop the service)
  EXPECT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0,
                                   {{wrench::SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, "BOGUS"}})));

  // Try to get a bogus property as string or double
  EXPECT_THROW(storage_service->getPropertyValueAsString("BOGUS"), std::invalid_argument);
  EXPECT_THROW(storage_service->getPropertyValueAsDouble("BOGUS"), std::invalid_argument);
  // Try to get a non-double double property (property value is "infinity", which is not a number)
  EXPECT_THROW(storage_service->getPropertyValueAsDouble(wrench::SimpleStorageServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD),
               std::invalid_argument);

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  EXPECT_NO_THROW(compute_service = simulation->add(
          new wrench::CloudService(hostname, execution_hosts, 100.0,
                                   { {wrench::MultihostMulticoreComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new SimpleSimulationReadyTasksTestWMS(this, {compute_service}, {storage_service}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  EXPECT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
