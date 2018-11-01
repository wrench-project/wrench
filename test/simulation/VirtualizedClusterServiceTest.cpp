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
#include <numeric>
#include <thread>
#include <chrono>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(virtualized_cluster_service_test, "Log category for VirtualizedClusterServiceTest");

#define EPSILON 0.001

class VirtualizedClusterServiceTest : public ::testing::Test {

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

    void do_VMMigrationTest_test();

    void do_PilotJobTaskTest_test();

    void do_NumCoresTest_test();

    void do_StopAllVMsTest_test();

    void do_ShutdownVMTest_test();

    void do_ShutdownVMAndThenShutdownServiceTest_test();

    void do_SubmitToVMTest_test();

protected:
    VirtualizedClusterServiceTest() {

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
      task2->addOutputFile(output_file2);
      task3->addOutputFile(output_file3);
      task4->addOutputFile(output_file4);
      task5->addOutputFile(output_file3);
      task6->addOutputFile(output_file4);

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

class CloudStandardJobTestWMS : public wrench::WMS {

public:
    CloudStandardJobTestWMS(VirtualizedClusterServiceTest *test,
                            const std::set<wrench::ComputeService *> &compute_services,
                            const std::set<wrench::StorageService *> &storage_services,
                            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob({this->test->task1, this->test->task2}, {},
                                                                         {std::make_tuple(this->test->input_file,
                                                                                          this->test->storage_service,
                                                                                          wrench::ComputeService::SCRATCH)},
                                                                         {}, {});


      // Submit the 2-task job for execution
      try {
        auto cs = (wrench::CloudService *) this->test->compute_service;
        cs->createVM(2, 10);
        job_manager->submitJob(two_task_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
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

TEST_F(VirtualizedClusterServiceTest, CloudStandardJobTestWMS) {
  DO_TEST_WITH_FORK(do_StandardJobTaskTest_test);
}

void VirtualizedClusterServiceTest::do_StandardJobTaskTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("cloud_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::CloudService(hostname, execution_hosts, 100,
                                   {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new CloudStandardJobTestWMS(this, {compute_service}, {storage_service}, hostname)));

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
/**                   VM MIGRATION SIMULATION TEST                   **/
/**********************************************************************/

class VirtualizedClusterVMMigrationTestWMS : public wrench::WMS {

public:
    VirtualizedClusterVMMigrationTestWMS(VirtualizedClusterServiceTest *test,
                                         const std::set<wrench::ComputeService *> &compute_services,
                                         const std::set<wrench::StorageService *> &storage_services,
                                         std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a 2-task job
      wrench::StandardJob *two_task_job = job_manager->createStandardJob(
              {this->test->task1, this->test->task2}, {},
              {std::make_tuple(this->test->input_file, this->test->storage_service, wrench::ComputeService::SCRATCH)},
              {}, {});

      // Submit the 2-task job for execution
      try {
        auto cs = (wrench::VirtualizedClusterService *) this->test->compute_service;
        std::string src_host = cs->getExecutionHosts()[0];
        std::string vm_host = cs->createVM(src_host, 2, 10);

        job_manager->submitJob(two_task_job, this->test->compute_service);

        // migrating the VM
        std::string dest_host = cs->getExecutionHosts()[1];
        cs->migrateVM(vm_host, dest_host);

      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
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

TEST_F(VirtualizedClusterServiceTest, VirtualizedClusterVMMigrationTestWMS) {
  DO_TEST_WITH_FORK(do_VMMigrationTest_test);
}

void VirtualizedClusterServiceTest::do_VMMigrationTest_test() {
  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("virtualized_cluster_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));


  // Create a Virtualized Cluster Service with no hosts
  std::vector<std::string> nothing;
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::VirtualizedClusterService(hostname, nothing, 100.0,
                                                {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,
                                                         "false"}})), std::invalid_argument);

  // Create a Virtualized Cluster Service
  std::vector<std::string> execution_hosts = simulation->getHostnameList();
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::VirtualizedClusterService(hostname, execution_hosts, 100.0,
                                                {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS,
                                                         "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new VirtualizedClusterVMMigrationTestWMS(this, {compute_service}, {storage_service}, hostname)));

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
/**  NUM CORES TEST                                                  **/
/**********************************************************************/

class CloudNumCoresTestWMS : public wrench::WMS {

public:
    CloudNumCoresTestWMS(VirtualizedClusterServiceTest *test,
                         const std::set<wrench::ComputeService *> &compute_services,
                         const std::set<wrench::StorageService *> &storage_services,
                         std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
      try {
        // no VMs
        std::map<std::string, unsigned long> num_cores = this->test->compute_service->getNumCores();
        unsigned long sum_num_cores = 0;
        for (auto const &c : num_cores) {
          sum_num_cores += c.second;
        }

        std::map<std::string, unsigned long> num_idle_cores = this->test->compute_service->getNumIdleCores();
        unsigned long sum_num_idle_cores = 0;
        for (auto const &c : num_idle_cores) {
          sum_num_idle_cores += c.second;
        }

        if (sum_num_cores != 0 || sum_num_idle_cores != 0) {
          throw std::runtime_error("getHostNumCores() and getNumIdleCores() should be 0.");
        }

        // create a VM with the PM number of cores
        auto cs = (wrench::CloudService *) this->test->compute_service;
        cs->createVM(0, 10);
        num_cores = cs->getNumCores();
        sum_num_cores = 0;
        for (auto const &c : num_cores) {
          sum_num_cores += c.second;
        }

        num_idle_cores = cs->getNumIdleCores();
        sum_num_idle_cores = 0;
        for (auto const &c : num_idle_cores) {
          sum_num_idle_cores += c.second;
        }

        if (sum_num_cores != 4 || sum_num_idle_cores != 4) {
          throw std::runtime_error("getHostNumCores() and getNumIdleCores() should be 4.");
        }

        // create a VM with two cores
        cs->createVM(2, 10);
        num_cores = cs->getNumCores();
        sum_num_cores = 0;
        for (auto const &c : num_cores) {
          sum_num_cores += c.second;
        }
        num_idle_cores = cs->getNumIdleCores();
        sum_num_idle_cores = 0;
        for (auto const &c : num_idle_cores) {
          sum_num_idle_cores += c.second;
        }

        if (sum_num_cores != 6 || sum_num_idle_cores != 6) {
          throw std::runtime_error("getHostNumCores() and getNumIdleCores() should be 6.");
        }

      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, CloudNumCoresTestWMS) {
  DO_TEST_WITH_FORK(do_NumCoresTest_test);
}

void VirtualizedClusterServiceTest::do_NumCoresTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("cloud_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::CloudService(hostname, execution_hosts, 0,
                                   {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new CloudNumCoresTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(
          new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  STOP ALL VMS TEST                                               **/
/**********************************************************************/

class StopAllVMsTestWMS : public wrench::WMS {

public:
    std::map<std::string, std::string> default_messagepayload_values = {
            {wrench::ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD, "1024"}
    };

    StopAllVMsTestWMS(VirtualizedClusterServiceTest *test,
                      const std::set<wrench::ComputeService *> &compute_services,
                      const std::set<wrench::StorageService *> &storage_services,
                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

      this->test = test;
      this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a bunch of VMs
      try {
        auto cs = (wrench::VirtualizedClusterService *) this->test->compute_service;
        std::string execution_host = cs->getExecutionHosts()[0];

        cs->createVM(1, 10);
        cs->createVM(execution_host, 1, 10);
        cs->createVM(execution_host, 1, 10);
        cs->createVM(execution_host, 1, 10);

      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      this->simulation->sleep(10);

      // stop all VMs
      this->test->compute_service->stop();

      return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, StopAllVMsTestWMS) {
  DO_TEST_WITH_FORK(do_StopAllVMsTest_test);
}

void VirtualizedClusterServiceTest::do_StopAllVMsTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("virtualized_cluster_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::VirtualizedClusterService(hostname, execution_hosts, 0,
                                                {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new StopAllVMsTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(
          new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  VM SHUTDOWN, START, SUSPEND, RESUME TEST                        **/
/**********************************************************************/

class ShutdownVMTestWMS : public wrench::WMS {

public:
    std::map<std::string, std::string> default_messagepayload_values = {
            {wrench::VirtualizedClusterServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD,  "1024"},
            {wrench::VirtualizedClusterServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, "1024"}
    };

    ShutdownVMTestWMS(VirtualizedClusterServiceTest *test,
                      const std::set<wrench::ComputeService *> &compute_services,
                      const std::set<wrench::StorageService *> &storage_services,
                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

      this->test = test;
      this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a pilot job that requests 1 host, 1 code, 0 bytes, and 1 minute
//      wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0.0, 60.0);

      std::vector<std::string> vm_list;

      auto cs = (wrench::VirtualizedClusterService *) this->test->compute_service;

      // Create VMs
      try {
        std::string execution_host = cs->getExecutionHosts()[0];

        vm_list.push_back(cs->createVM(execution_host, 1, 10));
        vm_list.push_back(cs->createVM(execution_host, 1, 10));
        vm_list.push_back(cs->createVM(execution_host, 1, 10));
        vm_list.push_back(cs->createVM(execution_host, 1, 10));

      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      this->simulation->sleep(1);
      // shutdown VMs
      try {
        for (auto &vm : vm_list) {
          if (!cs->shutdownVM(vm)) {
            throw std::runtime_error("Unable to shutdown VM");
          }
        }
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // Create a one-task job
      wrench::StandardJob *job = job_manager->createStandardJob(this->test->task1,
                                                                {std::make_pair(this->test->input_file,
                                                                                 this->test->storage_service),
                                                                 std::make_pair(this->test->output_file1,
                                                                                this->test->storage_service)});

      // Submit a job
      try {
        job_manager->submitJob(job, this->test->compute_service);
        throw std::runtime_error("should have thrown an exception since there are no resources available");
      } catch (wrench::WorkflowExecutionException &e) {
        // do nothing, should have thrown an exception since there are no resources available
      }


      try {
        cs->startVM(vm_list[3]);
      } catch (std::runtime_error &e) {
        throw std::runtime_error(e.what());
      }

      try {
        cs->suspendVM(vm_list[3]);
        job_manager->submitJob(job, this->test->compute_service);
        throw std::runtime_error("should have thrown an exception since there are no resources available");
      } catch (wrench::WorkflowExecutionException &e) {
        // do nothing, should have thrown an exception since there are no resources available
      }

      try {
        if (!cs->resumeVM(vm_list[3])) {
          throw std::runtime_error("Could not resume the VM");
        }
      } catch (std::runtime_error &e) {
        throw std::runtime_error(e.what());
      }

      if (cs->resumeVM(vm_list[3])) {
        throw std::runtime_error("VM was already resumed, so an exception was expected!");
      }

      job_manager->submitJob(job, this->test->compute_service);

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

      // Submit a job and suspend the VM before that job finishes
      wrench::StandardJob *other_job = job_manager->createStandardJob(this->test->task2,
                                                                {std::make_pair(this->test->input_file,
                                                                                this->test->storage_service),
                                                                 std::make_pair(this->test->output_file2,
                                                                                this->test->storage_service)});

      try {
        WRENCH_INFO("Submitting a job");
        job_manager->submitJob(other_job, this->test->compute_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Should be able to submit the other job");
      }
      double job_start_date = this->simulation->getCurrentSimulatedDate();

      WRENCH_INFO("Sleeping for 5 seconds");
      this->simulation->sleep(5);

      try {
        WRENCH_INFO("Suspending the one running VM (which is thus running the job)");
        cs->suspendVM(vm_list[3]);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Should be able to suspend VM");
      }


      WRENCH_INFO("Sleeping for 100 seconds");
      this->simulation->sleep(100);

      try {
        WRENCH_INFO("Resuming the VM");
        cs->resumeVM(vm_list[3]);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Should be able to resume VM");
      }

      // Wait for a workflow execution event
      WRENCH_INFO("Waiting for job completion");
      std::unique_ptr<wrench::WorkflowExecutionEvent> event2;
      try {
        event2 = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
      }
      switch (event2->type) {
        case wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          // success, do nothing for now
          break;
        }
        default: {
          throw std::runtime_error("Unexpected workflow execution event: " + std::to_string((int) (event2->type)));
        }
      }

      double job_turnaround_time = this->simulation->getCurrentSimulatedDate() - job_start_date;
      if (fabs(job_turnaround_time - 110) > EPSILON) {
        throw std::runtime_error("Unexpected job turnaround time " + std::to_string(job_turnaround_time));
      }


      // attempt to shutdown non-existent VM
      try {
        if (cs->shutdownVM("NON_EXISTENT_VM")) {
          throw std::runtime_error("Cannot shutdown a non-existent VM");
        }

      } catch (wrench::WorkflowExecutionException &e) {
        // do nothing, since it is the expected behavior
      }

      return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, ShutdownVMTestWMS) {
  DO_TEST_WITH_FORK(do_ShutdownVMTest_test);
}

void VirtualizedClusterServiceTest::do_ShutdownVMTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("virtualized_cluster_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  ASSERT_THROW(compute_service = simulation->add(
          new wrench::VirtualizedClusterService(hostname, execution_hosts, 0,
                                                {{wrench::VirtualizedClusterServiceProperty::SUPPORTS_PILOT_JOBS, "true"}})), std::invalid_argument);

  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::VirtualizedClusterService(hostname, execution_hosts, 0,
                                                {{wrench::VirtualizedClusterServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ShutdownVMTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(
          new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}



/**********************************************************************/
/**  VM START-SHUTDOWN, and then SERVICE SHUTDOWN                    **/
/**********************************************************************/

class ShutdownVMAndThenShutdownServiceTestWMS : public wrench::WMS {

public:
    std::map<std::string, std::string> default_messagepayload_values = {
            {wrench::VirtualizedClusterServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD,  "1024"},
            {wrench::VirtualizedClusterServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, "1024"}
    };

    ShutdownVMAndThenShutdownServiceTestWMS(VirtualizedClusterServiceTest *test,
                                            const std::set<wrench::ComputeService *> &compute_services,
                                            const std::set<wrench::StorageService *> &storage_services,
                                            std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

      this->test = test;
      this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a pilot job that requests 1 host, 1 code, 0 bytes, and 1 minute
      wrench::PilotJob *pilot_job = job_manager->createPilotJob(1, 1, 0.0, 60.0);

      std::vector<std::string> vm_list;

      auto cs = (wrench::VirtualizedClusterService *) this->test->compute_service;

      // Create VMs
      try {
        std::string execution_host = cs->getExecutionHosts()[0];

        vm_list.push_back(cs->createVM(execution_host, 1, 10));
        vm_list.push_back(cs->createVM(execution_host, 1, 10));
        vm_list.push_back(cs->createVM(execution_host, 1, 10));
        vm_list.push_back(cs->createVM(execution_host, 1, 10));

      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // shutdown some VMs
      try {
        if (!cs->shutdownVM(vm_list.at(0))) {
          throw std::runtime_error("Unable to shutdown VM");
        }
        if (!cs->shutdownVM(vm_list.at(2))) {
          throw std::runtime_error("Unable to shutdown VM");
        }
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      // stop service
      cs->stop();

      // Sleep a bit
      this->simulation->sleep(10.0);

      return 0;
    }
};

TEST_F(VirtualizedClusterServiceTest, ShutdownVMAndThenShutdownServiceTestWMS) {
  DO_TEST_WITH_FORK(do_ShutdownVMAndThenShutdownServiceTest_test);
}

void VirtualizedClusterServiceTest::do_ShutdownVMAndThenShutdownServiceTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("virtualized_cluster_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::VirtualizedClusterService(hostname, execution_hosts, 0,
                                                {{wrench::BareMetalComputeServiceProperty::SUPPORTS_STANDARD_JOBS, "false"}})));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new ShutdownVMAndThenShutdownServiceTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(
          new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  SUBMIT TO VM TEST                                               **/
/**********************************************************************/

class SubmitToVMTestWMS : public wrench::WMS {

public:
    std::map<std::string, std::string> default_messagepayload_values = {
            {wrench::VirtualizedClusterServiceMessagePayload::SHUTDOWN_VM_ANSWER_MESSAGE_PAYLOAD,  "1024"},
            {wrench::VirtualizedClusterServiceMessagePayload::SHUTDOWN_VM_REQUEST_MESSAGE_PAYLOAD, "1024"}
    };

    SubmitToVMTestWMS(VirtualizedClusterServiceTest *test,
                      const std::set<wrench::ComputeService *> &compute_services,
                      const std::set<wrench::StorageService *> &storage_services,
                      std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {

      this->test = test;
      this->setMessagePayloads(this->default_messagepayload_values, {});
    }

private:

    VirtualizedClusterServiceTest *test;

    int main() {
      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create standard jobs
      wrench::StandardJob *job1 = job_manager->createStandardJob({this->test->task1}, {},
                                                                 {std::make_tuple(this->test->input_file,
                                                                                  this->test->storage_service,
                                                                                  wrench::ComputeService::SCRATCH)},
                                                                 {}, {});
      wrench::StandardJob *job2 = job_manager->createStandardJob({this->test->task2}, {},
                                                                 {std::make_tuple(this->test->input_file,
                                                                                  this->test->storage_service,
                                                                                  wrench::ComputeService::SCRATCH)},
                                                                 {}, {});
      std::vector<std::string> vm_list;

      auto cs = (wrench::VirtualizedClusterService *) this->test->compute_service;

      // Submit standard job for execution
      try {
        std::string execution_host = cs->getExecutionHosts()[0];

        vm_list.push_back(cs->createVM(execution_host, 1, 10));
        vm_list.push_back(cs->createVM(execution_host, 1, 10));

      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      std::map<std::string, std::string> properties_map;
      properties_map.insert(std::make_pair("-vm", vm_list[1]));

      try {
        for (auto &vm_name : vm_list) {
          if (!cs->shutdownVM(vm_name)) {
            throw std::runtime_error("Could not shutdown VM " + vm_name);
          }
        }
        job_manager->submitJob(job1, this->test->compute_service, properties_map);
        throw std::runtime_error("should not be able to run job since VMs are stopped");
      } catch (std::runtime_error &e) {
        throw std::runtime_error(e.what());
      } catch (wrench::WorkflowExecutionException &e) {
        // do nothing, expected behavior
      }

      try {
        cs->startVM(vm_list[1]);
      } catch (std::runtime_error &e) {
        throw std::runtime_error(e.what());
      }

      try {
        job_manager->submitJob(job1, this->test->compute_service, properties_map);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error(e.what());
      }

      std::map<std::string, std::string> invalid_properties_map;
      invalid_properties_map.insert(std::make_pair("-vm", "non-existent-vm"));

      try {
        job_manager->submitJob(job2, this->test->compute_service, invalid_properties_map);
      } catch (wrench::WorkflowExecutionException &e) {
        // do nothing, expected behavior
      } catch (std::runtime_error &e) {
        throw std::runtime_error(e.what());
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

TEST_F(VirtualizedClusterServiceTest, SubmitToVMTestWMS) {
  DO_TEST_WITH_FORK(do_SubmitToVMTest_test);
}

void VirtualizedClusterServiceTest::do_SubmitToVMTest_test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("virtualized_cluster_service_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, 100.0)));

  // Create a Cloud Service
  std::vector<std::string> execution_hosts = {simulation->getHostnameList()[1]};
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::VirtualizedClusterService(hostname, execution_hosts, 1000)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new SubmitToVMTestWMS(this, {compute_service}, {storage_service}, hostname)));

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

