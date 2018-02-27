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

#include "TestWithFork.h"

class NetworkProximityTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service1 = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_NetworkProximity_Test();

    void do_CompareNetworkProximity_Test();

protected:
    NetworkProximityTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create two files
      input_file = workflow->addFile("input_file", 10000.0);
      output_file = workflow->addFile("output_file", 20000.0);

      // Create one task
      task = workflow->addTask("task", 3600);
      task->addInputFile(input_file);
      task->addOutputFile(output_file);

      // Create a one-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
              "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
              "       <link id=\"2\" bandwidth=\"1000GBps\" latency=\"1000us\"/>"
              "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"0us\"/>"
              "       <link id=\"4\" bandwidth=\"3000GBps\" latency=\"0us\"/>"
              "       <link id=\"5\" bandwidth=\"8000GBps\" latency=\"0us\"/>"
              "       <link id=\"6\" bandwidth=\"2900GBps\" latency=\"0us\"/>"
              "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
              "/> </route>"
              "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\""
              "/> </route>"
              "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"3\""
              "/> </route>"
              "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"4\""
              "/> </route>"
              "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"5\""
              "/> </route>"
              "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"6\""
              "/> </route>"
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
/**  SIMPLE PROXIMITY TEST                                            **/
/**********************************************************************/

class ProxTestWMS : public wrench::WMS {

public:
    ProxTestWMS(NetworkProximityTest *test,
                const std::set<wrench::ComputeService *> &compute_services,
                const std::set<wrench::StorageService *> &storage_services,
                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    NetworkProximityTest *test;

    int main() {
      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      std::pair<std::string, std::string> hosts_to_compute_proximity;
      hosts_to_compute_proximity = std::make_pair(this->simulation->getHostnameList()[2],
                                                  this->simulation->getHostnameList()[1]);
      int count = 0, max_count = 100;
      double proximity = this->simulation->getNetworkProximityService()->query(hosts_to_compute_proximity);

      while (proximity < 0 && count < max_count) {
        count++;
        wrench::Simulation::sleep(10.0);
        proximity = this->simulation->getNetworkProximityService()->query(hosts_to_compute_proximity);
      }

      if (count == max_count) {
        throw std::runtime_error("Never got an answer to proximity query");
      }

      if (proximity < 0.0) {
        throw std::runtime_error("Got a negative proximity value");
      }

      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};

TEST_F(NetworkProximityTest, NetworkProximity) {
  DO_TEST_WITH_FORK(do_NetworkProximity_Test);
}

void NetworkProximityTest::do_NetworkProximity_Test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr,
                                                               {}))));
  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new ProxTestWMS(
                  this,
                  {compute_service}, {storage_service1}, hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry service
  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Get a host for network proximity host
  std::string network_proximity_db_hostname = simulation->getHostnameList()[1];

  //Get two hosts to communicate with each other for proximity value
  std::string network_daemon1 = simulation->getHostnameList()[0];
  std::string network_daemon2 = simulation->getHostnameList()[1];
  std::string network_daemon3 = simulation->getHostnameList()[2];
  std::string network_daemon4 = simulation->getHostnameList()[3];
  std::vector<std::string> hosts_in_network = {network_daemon1, network_daemon2, network_daemon3, network_daemon4};

  // Create a network proximity service
  std::unique_ptr<wrench::NetworkProximityService> network_proximity_service(
          new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, 1, 2, 1)
  );

  EXPECT_NO_THROW(simulation->setNetworkProximityService(std::move(network_proximity_service)));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}


/**********************************************************************/
/**  COMPARE PROXIMITY TEST                                           **/
/**********************************************************************/

class CompareProxTestWMS : public wrench::WMS {

public:
    CompareProxTestWMS(NetworkProximityTest *test,
                       const std::set<wrench::ComputeService *> &compute_services,
                       const std::set<wrench::StorageService *> &storage_services,
                       std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    NetworkProximityTest *test;

    int main() {
      // Create a job manager
      std::unique_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a data movement manager
      std::unique_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

      std::pair<std::string, std::string> first_pair_to_compute_proximity;
      first_pair_to_compute_proximity = std::make_pair(this->simulation->getHostnameList()[0],
                                                       this->simulation->getHostnameList()[1]);
      int count = 0, max_count = 100;
      double first_pair_proximity = this->simulation->getNetworkProximityService()->query(
              first_pair_to_compute_proximity);

      while (first_pair_proximity < 0 && count < max_count) {
        count++;
        wrench::Simulation::sleep(10.0);
        first_pair_proximity = this->simulation->getNetworkProximityService()->query(first_pair_to_compute_proximity);
      }

      if (count == max_count) {
        throw std::runtime_error("Never got an answer to proximity query");
      }

      if (first_pair_proximity < 0.0) {
        throw std::runtime_error("Got a negative proximity value");
      }


      std::pair<std::string, std::string> second_pair_to_compute_proximity;
      second_pair_to_compute_proximity = std::make_pair(this->simulation->getHostnameList()[2],
                                                        this->simulation->getHostnameList()[3]);
      count = 0, max_count = 100;
      double second_pair_proximity = this->simulation->getNetworkProximityService()->query(
              second_pair_to_compute_proximity);

        while (second_pair_proximity < 0 && count < max_count) {
            count++;
            wrench::Simulation::sleep(10.0);
            second_pair_proximity = this->simulation->getNetworkProximityService()->query(second_pair_to_compute_proximity);
        }

      if (count == max_count) {
        throw std::runtime_error("Never got an answer to proximity query");
      }

      if (second_pair_proximity < 0.0) {
        throw std::runtime_error("Got a negative proximity value");
      }

      if (first_pair_proximity > second_pair_proximity) {
        throw std::runtime_error(
                "CompareProxTestWMS::main():: Expected proximity between a pair to be less than the other pair of hosts"
        );
      }

      // Terminate
      this->shutdownAllServices();
      return 0;
    }
};

TEST_F(NetworkProximityTest, CompareNetworkProximity) {
  DO_TEST_WITH_FORK(do_CompareNetworkProximity_Test);
}

void NetworkProximityTest::do_CompareNetworkProximity_Test() {

  // Create and initialize a simulation
  auto *simulation = new wrench::Simulation();
  int argc = 1;
  auto **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("one_task_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  EXPECT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  EXPECT_NO_THROW(compute_service = simulation->add(
          std::unique_ptr<wrench::MultihostMulticoreComputeService>(
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr,
                                                               {}))));
  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
          std::unique_ptr<wrench::SimpleStorageService>(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0))));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          std::unique_ptr<wrench::WMS>(new CompareProxTestWMS(
                  this,
                  {compute_service}, {storage_service1}, hostname))));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
          new wrench::FileRegistryService(hostname));

  simulation->setFileRegistryService(std::move(file_registry_service));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFile(input_file, storage_service1));

  // Get a host for network proximity host
  std::string network_proximity_db_hostname = simulation->getHostnameList()[1];

  //Get two hosts to communicate with each other for proximity value
  std::string network_daemon1 = simulation->getHostnameList()[0];
  std::string network_daemon2 = simulation->getHostnameList()[1];
  std::string network_daemon3 = simulation->getHostnameList()[2];
  std::string network_daemon4 = simulation->getHostnameList()[3];
  std::vector<std::string> hosts_in_network = {network_daemon1, network_daemon2, network_daemon3, network_daemon4};

  std::unique_ptr<wrench::NetworkProximityService> network_proximity_service(
          new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, 1, 2, 1)
  );

  EXPECT_NO_THROW(simulation->setNetworkProximityService(std::move(network_proximity_service)));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
