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
#include <boost/algorithm/string.hpp>

#include "../include/TestWithFork.h"

class NetworkProximityTest : public ::testing::Test {

public:
    wrench::WorkflowFile *input_file;
    wrench::WorkflowFile *output_file;
    wrench::WorkflowTask *task;
    wrench::StorageService *storage_service1 = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_NetworkProximity_Test();

    void do_CompareNetworkProximity_Test();

    void do_VivaldiConverge_Test();

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
              "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"3\""
              "/> </route>"
              "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"4\""
              "/> </route>"
              "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"5\""
              "/> </route>"
              "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"6\""
              "/> </route>"
              "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\""
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
/**  SIMPLE PROXIMITY TEST                                           **/
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
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        // Create a data movement manager
        std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

        std::pair<std::string, std::string> hosts_to_compute_proximity;
        hosts_to_compute_proximity = std::make_pair(this->simulation->getHostnameList()[2],
                                                    this->simulation->getHostnameList()[1]);
        int count = 0, max_count = 100;
        std::set<wrench::NetworkProximityService *> network_proximity_services = this->simulation->getRunningNetworkProximityServices();
        auto network_proximity_service = network_proximity_services.begin();
        double proximity = (*network_proximity_service)->query(hosts_to_compute_proximity);

        while (proximity < 0 && count < max_count) {
            count++;
            wrench::S4U_Simulation::sleep(10.0);
            proximity = (*network_proximity_service)->query(hosts_to_compute_proximity);
        }

        if (count == max_count) {
            throw std::runtime_error("Never got an answer to proximity query");
        }

        if (proximity < 0.0) {
            throw std::runtime_error("Got a negative proximity value");
        }

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
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr,
                                                               {})));
  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new ProxTestWMS(
                  this,
                  {compute_service}, {storage_service1}, hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

  // Create a file registry service
  simulation->setFileRegistryService( new wrench::FileRegistryService(hostname));

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

  wrench::NetworkProximityService *network_proximity_service = nullptr;

  // Create a network proximity service
  EXPECT_THROW(network_proximity_service =
          new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                              {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "BOGUS"}}),
               std::invalid_argument);

    EXPECT_THROW(network_proximity_service =
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "0.5"}}),
                 std::invalid_argument);

    std::vector<std::string> too_few_hosts = {network_daemon1};
    EXPECT_THROW(network_proximity_service =
            new wrench::NetworkProximityService(network_proximity_db_hostname, too_few_hosts,
                                                {}),
            std::invalid_argument);

  EXPECT_NO_THROW(network_proximity_service =
          new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                              {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}}));

  EXPECT_NO_THROW(simulation->add(network_proximity_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  COMPARE PROXIMITY TEST                                          **/
/**********************************************************************/

class CompareProxTestWMS : public wrench::WMS {

public:
    CompareProxTestWMS(NetworkProximityTest *test,
                       std::set<wrench::ComputeService *> compute_services,
                       std::set<wrench::StorageService *> storage_services,
                       std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, hostname, "test") {
      this->test = test;
    }

private:

    NetworkProximityTest *test;

    int main() {
      // Create a job manager
      std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

      // Create a data movement manager
      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();


      std::pair<std::string, std::string> first_pair_to_compute_proximity;
      first_pair_to_compute_proximity = std::make_pair(this->simulation->getHostnameList()[0],
                                                       this->simulation->getHostnameList()[1]);
      int count = 0, max_count = 100;
      double first_pair_proximity = (*(this->simulation->getRunningNetworkProximityServices().begin()))->query(
              first_pair_to_compute_proximity);

      while (first_pair_proximity < 0 && count < max_count) {
        count++;
        wrench::Simulation::sleep(10.0);
        first_pair_proximity = (*(this->simulation->getRunningNetworkProximityServices().begin()))->query(first_pair_to_compute_proximity);
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
      double second_pair_proximity = (*(this->simulation->getRunningNetworkProximityServices().begin()))->query(
              second_pair_to_compute_proximity);

        while (second_pair_proximity < 0 && count < max_count) {
            count++;
            wrench::Simulation::sleep(10.0);
            second_pair_proximity = (*(this->simulation->getRunningNetworkProximityServices().begin()))->query(second_pair_to_compute_proximity);
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

      return 0;
    }
};

TEST_F(NetworkProximityTest, CompareNetworkProximity) {
  DO_TEST_WITH_FORK(do_CompareNetworkProximity_Test);
}

void NetworkProximityTest::do_CompareNetworkProximity_Test() {

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
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr,
                                                               {})));

  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new CompareProxTestWMS(this, (std::set<wrench::ComputeService *>){compute_service},
                                                              (std::set<wrench::StorageService *>){storage_service1},
                                                              hostname)));

  wms->addWorkflow(this->workflow, 0.0);


  simulation->setFileRegistryService(new wrench::FileRegistryService(hostname));

  // Staging the input_file on the storage service
  EXPECT_NO_THROW(simulation->stageFiles({std::make_pair(input_file->getId(), input_file)}, storage_service1));

  // Get a host for network proximity host
  std::string network_proximity_db_hostname = simulation->getHostnameList()[1];

  //Get two hosts to communicate with each other for proximity value
  std::string network_daemon1 = simulation->getHostnameList()[0];
  std::string network_daemon2 = simulation->getHostnameList()[1];
  std::string network_daemon3 = simulation->getHostnameList()[2];
  std::string network_daemon4 = simulation->getHostnameList()[3];
  std::vector<std::string> hosts_in_network = {network_daemon1, network_daemon2, network_daemon3, network_daemon4};

  wrench::NetworkProximityService *network_proximity_service(
          new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network, {})
  );

  EXPECT_NO_THROW(simulation->add(network_proximity_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}

/**********************************************************************/
/**  VIVALDI CONVERGE TEST                                           **/
/**********************************************************************/

class VivaldiConvergeWMS : public wrench::WMS {
public:
    VivaldiConvergeWMS(NetworkProximityTest *test,
        std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, {}, hostname, "test") {
        this->test = test;
    }

private:
    NetworkProximityTest *test;

    int main() {
        // Create a job manager
        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        // Create a data movement manager
        std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();

        std::pair<std::string, std::string> hosts_to_compute_proximity;
        hosts_to_compute_proximity = std::make_pair(this->simulation->getHostnameList()[2],
                                                    this->simulation->getHostnameList()[3]);
        int count = 0, max_count = 100;
        std::set<wrench::NetworkProximityService *> network_proximity_services = this->simulation->getRunningNetworkProximityServices();

        wrench::NetworkProximityService* alltoall_service;
        wrench::NetworkProximityService* vivaldi_service;

        // TODO: discuss using map so that users can access a desired service easier. since the key in set will be a raw pointer
        for (auto  nps : network_proximity_services) {
            std::string type = nps->getPropertyValueAsString(wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE);

            if (boost::iequals(type, "alltoall")) {
                alltoall_service = nps;
            }

            if (boost::iequals(type, "vivaldi")) {
                vivaldi_service = nps;
            }
        }

        double alltoall_proximity = alltoall_service->query(hosts_to_compute_proximity);

        while (alltoall_proximity < 0 && count < max_count) {
            count++;
            wrench::S4U_Simulation::sleep(10.0);
            alltoall_proximity = alltoall_service->query(hosts_to_compute_proximity);
        }

        if (count == max_count) {
            throw std::runtime_error("Never got an answer to proximity query");
        }

        if (alltoall_proximity < 0.0) {
            throw std::runtime_error("Got a negative proximity value");
        }

        wrench::S4U_Simulation::sleep(100);
        double vivaldi_proximity = vivaldi_service->query(hosts_to_compute_proximity);

        if (vivaldi_proximity > 1000) {
            throw std::runtime_error("Vivaldi proximity is larger than it should be");
        }


        // Check values
        double epsilon = 0.1 * 1000;

      if (fabs(1000 - alltoall_proximity) > epsilon) {
        throw std::runtime_error("All-to-all algorithm goe a strange value: " + std::to_string(alltoall_proximity));
      }

      if (fabs(vivaldi_proximity - alltoall_proximity) > epsilon) {
            throw std::runtime_error("Vivaldi algorithm did not converge");
        }

        hosts_to_compute_proximity = std::make_pair(this->simulation->getHostnameList()[1],
                                                    this->simulation->getHostnameList()[2]);

        vivaldi_proximity = vivaldi_service->query(hosts_to_compute_proximity);

        if (vivaldi_proximity < 0 || vivaldi_proximity > 0.000001) {
            throw std::runtime_error("Vivaldi algorithm computed the wrong proximity between hosts connected by a 0 latency link");
        }

        return 0;
    }
};

TEST_F(NetworkProximityTest, VivaldiConvergeTest) {
    DO_TEST_WITH_FORK(do_VivaldiConverge_Test);
}

void NetworkProximityTest::do_VivaldiConverge_Test() {
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
                  new wrench::MultihostMulticoreComputeService(hostname, true, true,
                                                               {std::make_tuple(hostname, wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM)},
                                                               nullptr,
                                                               {})));
  // Create a Storage Service
  EXPECT_NO_THROW(storage_service1 = simulation->add(
                  new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  EXPECT_NO_THROW(wms = simulation->add(
          new CompareProxTestWMS(
                  this,
                  (std::set<wrench::ComputeService *>){compute_service},
                  (std::set<wrench::StorageService *>){storage_service1},
                  hostname)));

  EXPECT_NO_THROW(wms->addWorkflow(workflow));

    // Create a file registry service
  simulation->setFileRegistryService(new wrench::FileRegistryService(hostname));

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

    wrench::NetworkProximityService* alltoall_network_service = nullptr;
    wrench::NetworkProximityService* vivaldi_network_service = nullptr;

    // Add vivaldi and alltoall network proximity services
    EXPECT_NO_THROW(alltoall_network_service =
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "ALLTOALL"}}));

    EXPECT_NO_THROW(vivaldi_network_service =
            new wrench::NetworkProximityService(network_proximity_db_hostname, hosts_in_network,
                                                {{wrench::NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE, "VIVALDI"},
                                                 {wrench::NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE, "1.0"}}));

    EXPECT_NO_THROW(simulation->add(alltoall_network_service));
    EXPECT_NO_THROW(simulation->add(vivaldi_network_service));

  // Running a "run a single task" simulation
  EXPECT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
