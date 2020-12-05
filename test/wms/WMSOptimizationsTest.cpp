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

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class WMSOptimizationsTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;

    void do_staticOptimization_test();

    void do_dynamicOptimization_test();

protected:
    WMSOptimizationsTest() {
      // Create a platform file
      std::string xml = "<?xml version='1.0'?>"
                        "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                        "<platform version=\"4.1\"> "
                        "   <zone id=\"AS0\" routing=\"Full\"> "
                        "       <host id=\"DualCoreHost\" speed=\"1f\" core=\"2\"> "
                        "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                        "             <prop id=\"size\" value=\"10000000000000B\"/>"
                        "             <prop id=\"mount\" value=\"/\"/>"
                        "          </disk>"
                        "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                        "             <prop id=\"size\" value=\"101B\"/>"
                        "             <prop id=\"mount\" value=\"/scratch\"/>"
                        "          </disk>"
                        "       </host>  "
                        "       <host id=\"QuadCoreHost\" speed=\"1f\" core=\"4\"> "
                        "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                        "             <prop id=\"size\" value=\"10000000000000B\"/>"
                        "             <prop id=\"mount\" value=\"/\"/>"
                        "          </disk>"
                        "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                        "             <prop id=\"size\" value=\"101B\"/>"
                        "             <prop id=\"mount\" value=\"/scratch\"/>"
                        "          </disk>"
                        "       </host>  "
                        "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                        "       <route src=\"DualCoreHost\" dst=\"QuadCoreHost\"> <link_ctn id=\"1\"/> </route>"
                        "   </zone> "
                        "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);
    }

    wrench::Workflow *createWorkflow() {
      wrench::Workflow *workflow;
      wrench::WorkflowFile *input_file;
      wrench::WorkflowFile *output_file1;
      wrench::WorkflowFile *output_file2;
      wrench::WorkflowFile *output_file3;
      wrench::WorkflowTask *task1;
      wrench::WorkflowTask *task2;
      wrench::WorkflowTask *task3;

      // Create the simplest workflow
      workflow = new wrench::Workflow();
      workflow_unique_ptrs.push_back(std::unique_ptr<wrench::Workflow>(workflow));

      // Create the tasks
      task1 = workflow->addTask("task_1_10s_1core", 10.0, 1, 1, 0);
      task2 = workflow->addTask("task_2_10s_1core", 10.0, 1, 1, 0);
      task3 = workflow->addTask("task_3_10s_1core", 10.0, 1, 1, 0);

      workflow->addControlDependency(task1, task2);
      workflow->addControlDependency(task1, task3);

      return workflow;
    }

    std::vector<std::unique_ptr<wrench::Workflow>> workflow_unique_ptrs;
    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**  TASK CLUSTERING STATIC OPTIMIZATION                             **/
/**********************************************************************/

class TaskClustering : public wrench::StaticOptimization {

public:
    void process(wrench::Workflow *workflow) override {
      for (auto task : workflow->getTasks()) {
        if (task->getID() != "task_1_10s_1core") {
          task->setClusterID("CLUSTER_TASK_2_AND_3");
        }
      }
    }
};

class StaticOptimizationsTestWMS : public wrench::WMS {

public:
    StaticOptimizationsTestWMS(WMSOptimizationsTest *test,
                               const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                               const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                               std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test"
            ) {
      this->test = test;
    }

private:

    WMSOptimizationsTest *test;

    int main() {
      // Create job and data movement managers
      auto data_movement_manager = this->createDataMovementManager();
      auto job_manager = this->createJobManager();

      // Perform static optimizations
      runStaticOptimizations();

      while (true) {
        // Get the ready clustered tasks
        std::map<std::string, std::vector<wrench::WorkflowTask *>> ready_clustered_tasks = this->getWorkflow()->getReadyClusters();

        // Get the available compute services
        auto compute_services = this->getAvailableComputeServices<wrench::ComputeService>();

        // Run ready tasks with defined scheduler implementation
        unsigned int job_count = 0;
        for (const auto &task_map : ready_clustered_tasks) {
          auto job = job_manager->createStandardJob(task_map.second, {}, {}, {}, {});
          job_manager->submitJob(job, this->test->compute_service);
          job_count++;
        }

        // Wait for all workflow execution events, and process them
        for (unsigned int i=0; i < job_count; i++) {
          try {
            this->waitForAndProcessNextEvent();
          } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
          }
        }

        // Are we done?
        if (this->getWorkflow()->isDone()) {
          break;
        }
      }

      return 0;
    }
};

TEST_F(WMSOptimizationsTest, StaticOptimizationsTestWMS) {
  DO_TEST_WITH_FORK(do_staticOptimization_test);
}

void WMSOptimizationsTest::do_staticOptimization_test() {
  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("unit_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = wrench::Simulation::getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, {"/"})));

  // Create a MHMC Service
  std::vector<std::string> execution_hosts = {wrench::Simulation::getHostnameList()[1]};
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BareMetalComputeService(
                  hostname, execution_hosts, "/scratch",
                  {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  // Create a WMS
  wrench::Workflow *workflow = this->createWorkflow();
  std::shared_ptr<wrench::WMS> wms = nullptr;;
  ASSERT_NO_THROW(wms = simulation->add(
          new StaticOptimizationsTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));
  ASSERT_NO_THROW(wms->addStaticOptimization(std::unique_ptr<wrench::StaticOptimization>(new TaskClustering())));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
  for (auto const &f : workflow->getInputFiles()) {
      ASSERT_NO_THROW(simulation->stageFile(f, storage_service));
  }

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  for (auto t : simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>()) {
    if (t->getContent()->getTask()->getID() != "task_1_10s_1core") {
      ASSERT_EQ("CLUSTER_TASK_2_AND_3", t->getContent()->getTask()->getClusterID());
    }
  }

  delete simulation;
  for (int i=0; i < argc; i++)
     free(argv[i]);
  free(argv);
}

/**********************************************************************/
/**  TASK PARALLELIZATION DYNAMIC OPTIMIZATION                       **/
/**********************************************************************/

class TaskParallelization : public wrench::DynamicOptimization {

public:
    void process(wrench::Workflow *workflow) override {
      for (auto task : workflow->getTasks()) {
        if (task->getID() == "task_3_10s_1core") {
          task->setPriority(100);
        }
      }
    }
};

class DynamicOptimizationsTestWMS : public wrench::WMS {

public:
    DynamicOptimizationsTestWMS(WMSOptimizationsTest *test,
                                const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    WMSOptimizationsTest *test;

    struct TaskPriorityComparator {
        bool operator()(wrench::WorkflowTask *&lhs, wrench::WorkflowTask *&rhs) {
          return lhs->getPriority() > rhs->getPriority();
        }
    };

    int main() {
      // Create job and data movement managers
      auto data_movement_manager = this->createDataMovementManager();
      auto job_manager = this->createJobManager();

      while (true) {
        // Get the ready tasks
        std::vector<wrench::WorkflowTask *> ready_tasks = this->getWorkflow()->getReadyTasks();

        // Get the available compute services
        auto compute_services = this->getAvailableComputeServices<wrench::ComputeService>();

        // Perform static optimizations
        runDynamicOptimizations();

        // sorting tasks by priority
        std::sort(ready_tasks.begin(), ready_tasks.end(), TaskPriorityComparator());

        // Run ready tasks with defined scheduler implementation
        for (auto task : ready_tasks) {
          auto job = job_manager->createStandardJob(task, {});
          job_manager->submitJob(job, this->test->compute_service);
        }

        // Wait for a workflow execution event, and process it
        try {
          this->waitForAndProcessNextEvent();
        } catch (wrench::WorkflowExecutionException &e) {
          throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
        }
        if (this->getWorkflow()->isDone()) {
          break;
        }
      }

      return 0;
    }
};

TEST_F(WMSOptimizationsTest, DynamicOptimizationsTestWMS) {
  DO_TEST_WITH_FORK(do_dynamicOptimization_test);
}

void WMSOptimizationsTest::do_dynamicOptimization_test() {
  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  auto argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("unit_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = wrench::Simulation::getHostnameList()[0];

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService(hostname, {"/"})));

  // Create a MHMC Service
  std::vector<std::string> execution_hosts = {wrench::Simulation::getHostnameList()[1]};
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BareMetalComputeService(
                  hostname, execution_hosts, "/scratch",
                  {{wrench::BareMetalComputeServiceProperty::SUPPORTS_PILOT_JOBS, "false"}})));

  // Create a WMS
  wrench::Workflow *workflow = this->createWorkflow();
  std::shared_ptr<wrench::WMS> wms = nullptr;;
  ASSERT_NO_THROW(wms = simulation->add(
          new DynamicOptimizationsTestWMS(this, {compute_service}, {storage_service}, hostname)));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));
  ASSERT_NO_THROW(wms->addDynamicOptimization(std::unique_ptr<wrench::DynamicOptimization>(new TaskParallelization())));

  // Create a file registry
  ASSERT_NO_THROW(simulation->add(new wrench::FileRegistryService(hostname)));

  // Staging the input_file on the storage service
    // Staging the input_file on the storage service
    for (auto const &f : workflow->getInputFiles()) {
        ASSERT_NO_THROW(simulation->stageFile(f, storage_service));
    }

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  auto trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();

  ASSERT_EQ(trace[1]->getContent()->getTask()->getID(), "task_3_10s_1core");
  ASSERT_EQ(trace[2]->getContent()->getTask()->getID(), "task_2_10s_1core");
  ASSERT_GT(trace[2]->getContent()->getTask()->getStartDate(), trace[1]->getContent()->getTask()->getStartDate());

  delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
  free(argv);
}
