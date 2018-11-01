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
#include <algorithm>

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

class FileRegistryTest : public ::testing::Test {

public:
    wrench::StorageService *storage_service1 = nullptr;
    wrench::StorageService *storage_service2 = nullptr;
    wrench::StorageService *storage_service3 = nullptr;
    wrench::ComputeService *compute_service = nullptr;

    void do_FileRegistry_Test();
    void do_lookupEntry_Test();

protected:
    FileRegistryTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

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
              "       <link id=\"3\" bandwidth=\"2000GBps\" latency=\"1500us\"/>"
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

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;

};

/**********************************************************************/
/**  SIMPLE FILE REGISTRY TEST                                       **/
/**********************************************************************/

class FileRegistryTestWMS : public wrench::WMS {

public:
    FileRegistryTestWMS(FileRegistryTest *test,
                        const std::set<wrench::ComputeService *> &compute_services,
                        const std::set<wrench::StorageService *> &storage_services,
                        wrench::FileRegistryService *file_registry_service,
                        std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, file_registry_service, hostname, "test") {
      this->test = test;
    }

private:

    FileRegistryTest *test;

    int main() {


      wrench::WorkflowFile *file1 = this->getWorkflow()->addFile("file1", 100.0);
      wrench::WorkflowFile *file2 = this->getWorkflow()->addFile("file2", 100.0);
      wrench::FileRegistryService *frs = this->getAvailableFileRegistryService();

      bool success;
      std::set<wrench::StorageService *> locations;

      success = true;
      try {
        frs->addEntry(nullptr, this->test->storage_service1);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to add an entry with a nullptr file");
      }

      success = true;
      try {
        frs->addEntry(file1, nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to add an entry with a nullptr storage service");
      }

      success = true;
      try {
        frs->lookupEntry(nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup an entry with a nullptr file");
      }

      // Adding twice file1
      frs->addEntry(file1, this->test->storage_service1);
      frs->addEntry(file1, this->test->storage_service1);

      // Getting file1
      try {
        locations = frs->lookupEntry(file1);
      } catch (std::exception &e) {
        throw std::runtime_error("Should not have gotten an exception when looking up entry");
      }

      if (locations.size() != 1) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }
      if ((*locations.begin()) != this->test->storage_service1) {
        throw std::runtime_error("Got the wrong location for file 1");
      }

      // Adding another location for file1
      frs->addEntry(file1, this->test->storage_service2);

      // Getting file1
      locations = frs->lookupEntry(file1);

      if (locations.size() != 2) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }
      if ((locations.find(this->test->storage_service1) == locations.end()) ||
          (locations.find(this->test->storage_service2) == locations.end())) {
        throw std::runtime_error("Got the wrong locations for file 1");
      }

      success = true;
      try {
        frs->removeEntry(nullptr, this->test->storage_service1);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to remove an entry with a nullptr file");
      }

      success = true;
      try {
        frs->removeEntry(file1, nullptr);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to remove an entry with a nullptr storage service");
      }

      frs->removeEntry(file1, this->test->storage_service1);

      // Doing it again, which does nothing (coverage)
      frs->removeEntry(file1, this->test->storage_service1);

      // Getting file1
      locations = frs->lookupEntry(file1);

      if (locations.size() != 1) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }
      if (locations.find(this->test->storage_service2) == locations.end()) {
        throw std::runtime_error("Got the wrong locations for file 1");
      }

      // Remove an already removed entry
      frs->removeEntry(file1, this->test->storage_service1);

      // Shutting down the service
      frs->stop();

      // Trying a removeEntry
      success = true;
      try {
        frs->removeEntry(file1, this->test->storage_service1);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != frs) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to remove an entry from a service that is down");
      }

      return 0;
    }
};

TEST_F(FileRegistryTest, SimpleFunctionality) {
  DO_TEST_WITH_FORK(do_FileRegistry_Test);
}

void FileRegistryTest::do_FileRegistry_Test() {

  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("file_registry_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = simulation->getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BareMetalComputeService(hostname,
                                                       {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                                       {})));
  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, 10000000000000.0)));

  // Create a file registry service
  wrench::FileRegistryService *file_registry_service = nullptr;
  ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(hostname)));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(
          new FileRegistryTestWMS(
                  this,
                  {compute_service}, {storage_service1, storage_service2}, file_registry_service, hostname)));



  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
/**********************************************************************/
/**   LOOKUP ENTRY BY PROXIMITY TEST                                 **/
/**********************************************************************/

class FileRegistryLookupEntryTestWMS : public wrench::WMS {

public:
    FileRegistryLookupEntryTestWMS(FileRegistryTest *test,
                                   const std::set<wrench::ComputeService *> &compute_services,
                                   const std::set<wrench::StorageService *> &storage_services,
                                   const std::set<wrench::NetworkProximityService *> &network_proximity_services,
                                   wrench::FileRegistryService *file_registry_service,
                                   std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, network_proximity_services, file_registry_service, hostname, "test") {
      this->test = test;
    }

private:

    FileRegistryTest *test;

    int main() {

      wrench::WorkflowFile *file1 = this->getWorkflow()->addFile("file1", 100.0);
      wrench::WorkflowFile * nullptr_file = nullptr;
      wrench::FileRegistryService *frs = this->getAvailableFileRegistryService();
      wrench::NetworkProximityService *nps = *(this->getAvailableNetworkProximityServices().begin());

      frs->addEntry(file1, this->test->storage_service1);
      frs->addEntry(file1, this->test->storage_service2);
      frs->addEntry(file1, this->test->storage_service3);

      wrench::S4U_Simulation::sleep(600.0);


      std::vector<std::string> file1_expected_locations = {"Host4", "Host1", "Host2"};
      std::vector<std::string> file1_locations(3);
      std::map<double, wrench::StorageService *> file1_locations_by_proximity;

      bool success = true;
      try {
        frs->lookupEntry(nullptr_file, "Host3", nps);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a nullptr file");
      }

      try {
        file1_locations_by_proximity = frs->lookupEntry(file1, "Host3", nps);
      } catch (std::exception &e) {
        throw std::runtime_error("Should be able to lookup a file");
      }

      int count=0;
      for (auto &storage_service : file1_locations_by_proximity) {
        file1_locations[count++] = storage_service.second->getHostname();
      }

      bool is_equal = std::equal(file1_expected_locations.begin(), file1_expected_locations.end(),
                                 file1_locations.begin());

      if (!is_equal) {
        throw std::runtime_error("lookupEntry using NetworkProximityService did not return Storage Services in ascending order of Network Proximity");
      }

      auto last_location = file1_locations_by_proximity.rbegin();
      if (last_location->second->getHostname() != "Host2") {
        throw std::runtime_error(
                "lookupEntry using NetworkProximityService did not return the correct unmonitored Storage Service");
      } else if (last_location->first != DBL_MAX) {
        throw std::runtime_error(
                "lookupEntry using NetworkProximityService did not include the unmonitored Storage Service");
      }


      // Use a bogus host
      success = true;
      try {
        frs->lookupEntry(file1, "BogusHost", nps);
      } catch (std::invalid_argument &e) {
        success = false;
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file by proximity with a bogus reference host");
      }

      // shutdown service
      frs->stop();

      success = true;
      try {
        frs->lookupEntry(file1);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != frs) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file when the service is down");
      }

      success = true;
      try {
        file1_locations_by_proximity = frs->lookupEntry(file1, "Host3", nps);
      } catch (wrench::WorkflowExecutionException &e) {
        success = false;
        // Check Exception
        if (e.getCause()->getCauseType() != wrench::FailureCause::SERVICE_DOWN) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected type " +
                                   std::to_string(e.getCause()->getCauseType()) + " (was expecting ServiceIsDown)");
        }
        // Check Exception details
        wrench::ServiceIsDown *real_cause = (wrench::ServiceIsDown *) e.getCause().get();
        if (real_cause->getService() != frs) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
        }
      }
      if (success) {
        throw std::runtime_error("Should not be able to lookup a file when the service is down");
      }


      return 0;
    }
};

TEST_F(FileRegistryTest, LookupEntry) {
  DO_TEST_WITH_FORK(do_lookupEntry_Test);
}

void FileRegistryTest::do_lookupEntry_Test() {

  // Create and initialize a simulation
  wrench::Simulation *simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("file_registry_lookup_entry_test");

  simulation->init(&argc, argv);

  simulation->instantiatePlatform(platform_file_path);

  std::string host1 = simulation->getHostnameList()[0];
  std::string host2 = simulation->getHostnameList()[1];
  std::string host3 = simulation->getHostnameList()[2];
  std::string host4 = simulation->getHostnameList()[3];

  wrench::NetworkProximityService *network_proximity_service = new wrench::NetworkProximityService(host1, {host1, host3, host4});

  simulation->add(network_proximity_service);

  storage_service1 = simulation->add(
          new wrench::SimpleStorageService(host1, 10000000000000.0));

  storage_service2 = simulation->add(
          new wrench::SimpleStorageService(host2, 10000000000000.0));

  storage_service3 = simulation->add(
          new wrench::SimpleStorageService(host4, 10000000000000.0));


  wrench::FileRegistryService *file_registry_service(
          new wrench::FileRegistryService(host1));

  simulation->add(file_registry_service);

  wrench::WMS *wms = nullptr;
  wms = simulation->add(
          new FileRegistryLookupEntryTestWMS(
                  this,
                  {}, {storage_service1, storage_service2, storage_service3}, {network_proximity_service}, file_registry_service, host1));

  wms->addWorkflow(workflow);

  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  free(argv[0]);
  free(argv);
}
