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

#include "NoopScheduler.h"
#include "TestWithFork.h"

class CloudServiceTest : public ::testing::Test {

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

    void do_NumCoresTest_test();

protected:
    CloudServiceTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      input_file = workflow->addFile("input_file", 10.0);
      output_file1 = workflow->addFile("output_file1", 10.0);
      output_file2 = workflow->addFile("output_file2", 10.0);
      output_file3 = workflow->addFile("output_file3", 10.0);
      output_file4 = workflow->addFile("output_file4", 10.0);

      // Create the tasks
      task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 1.0);
      task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 1.0);
      task3 = workflow->addTask("task_3_10s_2cores", 10.0, 2, 2, 1.0);
      task4 = workflow->addTask("task_4_10s_2cores", 10.0, 2, 2, 1.0);
      task5 = workflow->addTask("task_5_30s_1_to_3_cores", 30.0, 1, 3, 1.0);
      task6 = workflow->addTask("task_6_10s_1_to_2_cores", 12.0, 1, 2, 1.0);

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

      // Create a platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <AS id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"/> "
              "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"/> "
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
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
/**  STANDARD JOB SUBMISSION TASK SIMULATION TEST ON ONE HOST        **/
/**********************************************************************/

class CloudOneStandardJobTestWMS : public wrench::WMS {

public:
    CloudOneStandardJobTestWMS(CloudServiceTest *test,
                               wrench::Workflow *workflow,
                               std::unique_ptr<wrench::Scheduler> scheduler,
                               std::string &hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    CloudServiceTest *test;

    int main() {
      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager =
              std::unique_ptr<wrench::DataMovementManager>(new wrench::DataMovementManager(this->workflow));

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager =
              std::unique_ptr<wrench::JobManager>(new wrench::JobManager(this->workflow));

      wrench::FileRegistryService *file_registry_service = this->simulation->getFileRegistryService();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {}, {},
                                                                         {}, {});

      // Submit the 2-task job for execution
      try {
        ((wrench::CloudService *) this->test->compute_service)->createVM(
                this->simulation->getHostnameList()[1],
                "vm_" + this->simulation->getHostnameList()[1],
                2, {}
        );
        job_manager->submitJob(two_task_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(CloudServiceTest, CloudOneStandardJobTestWMS) {
  DO_TEST_WITH_FORK(do_StandardJobTaskTest_test);
}

void CloudServiceTest::do_StandardJobTaskTest_test() {

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

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(std::unique_ptr<wrench::WMS>(
          new CloudOneStandardJobTestWMS(this, workflow, std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Cloud Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::CloudService>(new wrench::CloudService(hostname, true, true, storage_service, {}))));

  // Create a file registry
  EXPECT_NO_THROW(simulation->setFileRegistryService(
          std::unique_ptr<wrench::FileRegistryService>(new wrench::FileRegistryService(hostname))));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  NUM CORES TEST                                                  **/
/**********************************************************************/

class CloudNumCoresTestWMS : public wrench::WMS {

public:
    CloudNumCoresTestWMS(CloudServiceTest *test,
                         wrench::Workflow *workflow,
                         std::unique_ptr<wrench::Scheduler> scheduler,
                         std::string &hostname) :
            wrench::WMS(workflow, std::move(scheduler), hostname, "test") {
      this->test = test;
    }

private:

    CloudServiceTest *test;

    int main() {
      try {
        // no VMs
        unsigned long num_cores = this->test->compute_service->getNumCores();
        unsigned long num_idle_cores = this->test->compute_service->getNumIdleCores();
        if (num_cores != 0 || num_idle_cores != 0) {
          throw std::runtime_error("getNumCores() and getNumIdleCores() should be 0.");
        }

        // create a VM with the PM number of cores
        ((wrench::CloudService *) this->test->compute_service)->createVM(
                this->simulation->getHostnameList()[1], "vm_1" + this->simulation->getHostnameList()[1], 0, {});

        num_cores = this->test->compute_service->getNumCores();
        num_idle_cores = this->test->compute_service->getNumIdleCores();

        if (num_cores != 4 || num_idle_cores != 4) {
          throw std::runtime_error("getNumCores() and getNumIdleCores() should be 4.");
        }

        // create a VM with two cores
        ((wrench::CloudService *) this->test->compute_service)->createVM(
                this->simulation->getHostnameList()[1], "vm_2" + this->simulation->getHostnameList()[1], 2, {});

        num_cores = this->test->compute_service->getNumCores();
        num_idle_cores = this->test->compute_service->getNumIdleCores();

        if (num_cores != 6 || num_idle_cores != 6) {
          throw std::runtime_error("getNumCores() and getNumIdleCores() should be 6.");
        }

      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // Terminate
      this->simulation->shutdownAllComputeServices();
      this->simulation->shutdownAllStorageServices();
      this->simulation->getFileRegistryService()->stop();
      return 0;
    }
};

TEST_F(CloudServiceTest, CloudNumCoresTestWMS) {
  DO_TEST_WITH_FORK(do_NumCoresTest_test);
}

void CloudServiceTest::do_NumCoresTest_test() {

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

  // Create a WMS
  EXPECT_NO_THROW(wrench::WMS *wms = simulation->setWMS(std::unique_ptr<wrench::WMS>(
          new CloudNumCoresTestWMS(this, workflow, std::unique_ptr<wrench::Scheduler>(
                          new NoopScheduler()), hostname))));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(new wrench::SimpleStorageService(hostname, 100.0))));

  // Create a Cloud Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::CloudService>(new wrench::CloudService(hostname, true, true, storage_service, {}))));

  // Create a file registry
  EXPECT_NO_THROW(simulation->setFileRegistryService(
          std::unique_ptr<wrench::FileRegistryService>(new wrench::FileRegistryService(hostname))));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({input_file}, storage_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
