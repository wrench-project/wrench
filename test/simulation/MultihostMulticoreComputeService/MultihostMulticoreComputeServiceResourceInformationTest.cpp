/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../TestWithFork.h"

#define EPSILON 0.005


class MultihostMulticoreComputeServiceTestResourceInformation : public ::testing::Test {


public:
    // Default
    wrench::ComputeService *compute_service1 = nullptr;
    wrench::ComputeService *compute_service2 = nullptr;
    wrench::ComputeService *compute_service3 = nullptr;

    void do_ResourceInformation_test();

protected:
    MultihostMulticoreComputeServiceTestResourceInformation() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create a two-host quad-core platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"4\"> "
              "         <prop id=\"ram\" value=\"2048\"/> "
              "       </host>  "
              "       <host id=\"Host2\" speed=\"10Gf\" core=\"4\"> "
              "         <prop id=\"ram\" value=\"1024\"/> "
              "       </host>  "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
              "       </host>  "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
              "         <prop id=\"ram\" value=\"1024\"/> "
              "       </host>  "
              "        <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"1\"/> </route>"
              "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);

    }

    std::string platform_file_path = "/tmp/platform.xml";
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  RESOURCE INFORMATION SIMULATION TEST                            **/
/**********************************************************************/

class ResourceInformationTestWMS : public wrench::WMS {

public:
    ResourceInformationTestWMS(MultihostMulticoreComputeServiceTestResourceInformation *test,
                               const std::set<wrench::ComputeService *> &compute_services,
                               const std::set<wrench::StorageService *> &storage_services,
                               std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    MultihostMulticoreComputeServiceTestResourceInformation *test;

    int main() {

      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Ask questions about resources

      // Get number of Hosts
      unsigned long num_hosts;

      num_hosts = this->test->compute_service1->getNumHosts();
      if (num_hosts != 2) {
        throw std::runtime_error("getNumHosts() should return 2 for compute service #1");
      }

      num_hosts = this->test->compute_service2->getNumHosts();
      if (num_hosts != 2) {
        throw std::runtime_error("getNumHosts() should return 2 for compute service #2");
      }


      // Get number of Cores
      std::vector<unsigned long> num_cores;

      num_cores = this->test->compute_service1->getNumCores();
      if ((num_cores.size() != 2) or (num_cores[0] != 4) or (num_cores[1] != 4)) {
        throw std::runtime_error("getNumCores() should return {4,4} for compute service #1");
      }

      num_cores = this->test->compute_service2->getNumCores();
      if ((num_cores.size() != 2) or (num_cores[0] != 8) or (num_cores[1] != 8)) {
        throw std::runtime_error("getNumCores() should return {8,8} for compute service #1");
      }

      // Get Ram capacities
      std::vector<double> ram_capacities;

      ram_capacities = this->test->compute_service1->getMemoryCapacity();
      std::sort(ram_capacities.begin(), ram_capacities.end());
      if ((ram_capacities.size() != 2) or
          (fabs(ram_capacities[0] - 1024) > EPSILON) or
          (fabs(ram_capacities[1] - 2048) > EPSILON)) {
        throw std::runtime_error("getHostMemoryCapacity() should return {1024,2048} or {2048,1024} for compute service #1");
      }

      // Get Core flop rates
      std::vector<double> core_flop_rates = this->test->compute_service1->getCoreFlopRate();
      std::sort(core_flop_rates.begin(), core_flop_rates.end());
      if ((core_flop_rates.size() != 2) or
          (fabs(core_flop_rates[0] - 1.0) > EPSILON) or
          (fabs(core_flop_rates[1] - 1e+10) > EPSILON)) {
        throw std::runtime_error("getCoreFlopRate() should return {1,10} or {10,1} for compute service #1");

      }

      // Get the TTL
      if (this->test->compute_service1->getTTL() < DBL_MAX) {
        throw std::runtime_error("getTTL() should return +inf for compute service #1");
      }

      // Create a job that will use cores on compute service #1
      wrench::WorkflowTask *t1 = this->workflow->addTask("task1", 60.0000, 3, 3, 1.0);
      wrench::WorkflowTask *t2 = this->workflow->addTask("task2", 60.0001, 2, 2, 1.0);

      std::vector<wrench::WorkflowTask *> tasks;
      tasks.push_back(t1);
      tasks.push_back(t2);
      wrench::StandardJob *job = job_manager->createStandardJob(tasks, {}, {}, {}, {});
      job_manager->submitJob(job, this->test->compute_service1);

      wrench::Simulation::sleep(1.0);

      // Get number of idle cores
      std::vector<unsigned long> num_idle_cores = this->test->compute_service1->getNumIdleCores();
      std::sort(num_idle_cores.begin(), num_idle_cores.end());
      if ((num_idle_cores.size() != 2) or
          (num_idle_cores[0] != 1) or
          (num_idle_cores[1] != 2)) {
        throw std::runtime_error("getNumIdleCores() should return {1,2} or {1,2} for compute service #1");
      }

      // Wait for the workflow execution event
      std::unique_ptr<wrench::WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();
      if (event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_COMPLETION) {
        throw std::runtime_error("Unexpected workflow execution event!");
      }


      workflow->removeTask(t1);
      workflow->removeTask(t2);

      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};


TEST_F(MultihostMulticoreComputeServiceTestResourceInformation, ResourceInformation) {
  DO_TEST_WITH_FORK(do_ResourceInformation_test);
}

void MultihostMulticoreComputeServiceTestResourceInformation::do_ResourceInformation_test() {

  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create 1 Compute Service that manages Host1 and Host2
  EXPECT_NO_THROW(compute_service1 = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService("Host1", true, true,
                                                               {{std::make_tuple("Host1", 4, wrench::ComputeService::ALL_RAM)},
                                                                {std::make_tuple("Host2", 4, wrench::ComputeService::ALL_RAM)}},
                                                               nullptr
                  ))));

  // Create 1 Compute Service that manages Host3 and Host4
  EXPECT_NO_THROW(compute_service2 = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService("Host1", true, true,
                                                               {{std::make_tuple("Host3", 8, wrench::ComputeService::ALL_RAM)},
                                                                {std::make_tuple("Host4", 8, wrench::ComputeService::ALL_RAM)}},
                                                               nullptr
                  ))));
  std::set<wrench::ComputeService *> compute_services;
  compute_services.insert(compute_service1);
  compute_services.insert(compute_service2);

  // Create the WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new ResourceInformationTestWMS(
                  this,  compute_services, {}, "Host1"))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
