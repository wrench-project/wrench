/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>

#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

#define EPSILON 0.005


class BareMetalComputeServiceTestResourceInformation : public ::testing::Test {


public:
    // Default
    std::shared_ptr<wrench::ComputeService> compute_service1 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service2 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service3 = nullptr;

    void do_ResourceInformation_test();

protected:
    BareMetalComputeServiceTestResourceInformation() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create a two-host quad-core platform file
      std::string xml = "<?xml version='1.0'?>"
                        "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                        "<platform version=\"4.1\"> "
                        "   <zone id=\"AS0\" routing=\"Full\"> "
                        "       <host id=\"Host1\" speed=\"1f\" core=\"4\"> "
                        "         <prop id=\"ram\" value=\"2048B\"/> "
                        "       </host>  "
                        "       <host id=\"Host2\" speed=\"10Gf\" core=\"4\"> "
                        "         <prop id=\"ram\" value=\"1024B\"/> "
                        "       </host>  "
                        "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                        "       </host>  "
                        "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
                        "         <prop id=\"ram\" value=\"1024B\"/> "
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

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;
};


/**********************************************************************/
/**  RESOURCE INFORMATION SIMULATION TEST                            **/
/**********************************************************************/

class ResourceInformationTestWMS : public wrench::WMS {

public:
    ResourceInformationTestWMS(BareMetalComputeServiceTestResourceInformation *test,
                               const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                               const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                               std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
      this->test = test;
    }

private:

    BareMetalComputeServiceTestResourceInformation *test;

    int main() {

      // Create a job manager
      auto job_manager = this->createJobManager();

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

      // Get Host list
      std::vector<std::string> host_list;
      host_list = this->test->compute_service1->getHosts();
      if (host_list.size() != 2) {
          throw std::runtime_error("getHosts() should return a list with 2 items for compute service #1");
      }
      if (std::find(host_list.begin(), host_list.end(), "Host1") == host_list.end()) {
          throw std::runtime_error("getHosts() should return a list that contains 'Host1' for compute service #1");
      }
        if (std::find(host_list.begin(), host_list.end(), "Host2") == host_list.end()) {
            throw std::runtime_error("getHosts() should return a list that contains 'Host2' for compute service #1");
        }

      // Get number of Cores
      std::map<std::string, unsigned long> num_cores;

      num_cores = this->test->compute_service1->getPerHostNumCores();
      if ((num_cores.size() != 2) or ((*(num_cores.begin())).second != 4) or ((*(++num_cores.begin())).second != 4)) {
        throw std::runtime_error("getHostNumCores() should return {4,4} for compute service #1");
      }

      num_cores = this->test->compute_service2->getPerHostNumCores();
      if ((num_cores.size() != 2) or ((*(num_cores.begin())).second != 8) or ((*(++num_cores.begin())).second != 8)) {
        throw std::runtime_error("getHostNumCores() should return {8,8} for compute service #1");
      }

      if (this->test->compute_service1->getTotalNumCores() != 8) {
        throw std::runtime_error("getHostTotalNumCores() should return 8 for compute service #1");
      }

      if (this->test->compute_service2->getTotalNumCores() != 16) {
        throw std::runtime_error("getHostTotalNumCores() should return 16 for compute service #2");
      }

      // Get Ram capacities
      std::map<std::string, double> ram_capacities;

      ram_capacities = this->test->compute_service1->getMemoryCapacity();
      std::vector<double> sorted_ram_capacities;
      for (auto const &r : ram_capacities) {
        sorted_ram_capacities.push_back(r.second);
      }
      std::sort(sorted_ram_capacities.begin(), sorted_ram_capacities.end());
      if ((sorted_ram_capacities.size() != 2) or
          (std::abs(sorted_ram_capacities.at(0) - 1024) > EPSILON) or
          (std::abs(sorted_ram_capacities.at(1) - 2048) > EPSILON)) {
        throw std::runtime_error("getHostMemoryCapacity() should return {1024,2048} or {2048,1024} for compute service #1");
      }

      // Get Core flop rates
      std::map<std::string, double> core_flop_rates = this->test->compute_service1->getCoreFlopRate();
      std::vector<double> sorted_core_flop_rates;
      for (auto const &f : core_flop_rates) {
        sorted_core_flop_rates.push_back(f.second);
      }
      std::sort(sorted_core_flop_rates.begin(), sorted_core_flop_rates.end());
      if ((sorted_core_flop_rates.size() != 2) or
          (std::abs(sorted_core_flop_rates.at(0) - 1.0) > EPSILON) or
          (std::abs(sorted_core_flop_rates.at(1) - 1e+10) > EPSILON)) {
        throw std::runtime_error("getCoreFlopRate() should return {1,10} or {10,1} for compute service #1");

      }

      // Get the TTL
      if (this->test->compute_service1->getTTL() < DBL_MAX) {
        throw std::runtime_error("getTTL() should return +inf for compute service #1");
      }

      // Create a job that will use cores on compute service #1
      wrench::WorkflowTask *t1 = this->getWorkflow()->addTask("task1", 60.0000, 3, 3, 0);
      wrench::WorkflowTask *t2 = this->getWorkflow()->addTask("task2", 60000000000.0001, 2, 2, 0);

      std::vector<wrench::WorkflowTask *> tasks;
      tasks.push_back(t1);
      tasks.push_back(t2);
      auto job = job_manager->createStandardJob(tasks);

      job_manager->submitJob(job, this->test->compute_service1, {{"task1", "Host1:3"},
                                                                 {"task2", "Host2:2"}});

      wrench::Simulation::sleep(1.0);

      // Get number of idle cores
      std::map<std::string, unsigned long> num_idle_cores = this->test->compute_service1->getPerHostNumIdleCores();
      std::vector<unsigned long> idle_core_counts;
      for (auto const &c : num_idle_cores) {
        idle_core_counts.push_back(c.second);
      }
      std::sort(idle_core_counts.begin(), idle_core_counts.end());

      if ((idle_core_counts.size() != 2) or
          (idle_core_counts[0] != 1) or
          (idle_core_counts[1] != 2)) {
        throw std::runtime_error("getPerHostNumIdleCores() should return {1,2} or {2,1} for compute service #1");
      }

      if (this->test->compute_service1->getTotalNumIdleCores() != 3) {
        throw std::runtime_error("getTotalNumIdleCores() should return 3 for compute service #1");
      }

      this->test->compute_service1->getPerHostAvailableMemoryCapacity(); // coverage

      // Wait for the workflow execution event
      auto event = this->getWorkflow()->waitForNextExecutionEvent();
      if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
        throw std::runtime_error("Unexpected workflow execution event!");
      }


      this->getWorkflow()->removeTask(t1);
      this->getWorkflow()->removeTask(t2);

      return 0;
    }
};


TEST_F(BareMetalComputeServiceTestResourceInformation, ResourceInformation) {
  DO_TEST_WITH_FORK(do_ResourceInformation_test);
}

void BareMetalComputeServiceTestResourceInformation::do_ResourceInformation_test() {

  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("unit_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create 1 Compute Service that manages Host1 and Host2
  ASSERT_NO_THROW(compute_service1 = simulation->add(
          new wrench::BareMetalComputeService("Host1",
                                              {{std::make_pair("Host1",
                                                               std::make_tuple(4, wrench::ComputeService::ALL_RAM))},
                                               {std::make_pair("Host2",
                                                               std::make_tuple(4, wrench::ComputeService::ALL_RAM))}}, ""
          )));

  // Create 1 Compute Service that manages Host3 and Host4
  ASSERT_NO_THROW(compute_service2 = simulation->add(
          new wrench::BareMetalComputeService("Host1",
                                              {{std::make_pair("Host3",
                                                               std::make_tuple(8, wrench::ComputeService::ALL_RAM))},
                                               {std::make_pair("Host4",
                                                               std::make_tuple(8, wrench::ComputeService::ALL_RAM))}}, ""
          )));
  std::set<std::shared_ptr<wrench::ComputeService>> compute_services;
  compute_services.insert(compute_service1);
  compute_services.insert(compute_service2);

  // Create the WMS
  std::shared_ptr<wrench::WMS> wms = nullptr;;
  ASSERT_NO_THROW(wms = simulation->add(
          new ResourceInformationTestWMS(
                  this, compute_services, {}, "Host1")));

  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  for (int i=0; i < argc; i++)
     free(argv[i]);
  free(argv);
}
