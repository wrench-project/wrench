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

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(file_registry_service_test, "Log category for File Registry Service test");


class FileRegistryTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service3 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

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
              "       <host id=\"Host1\" speed=\"1f\" core=\"10\" > "
              "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"10000000000000B\"/>"
              "             <prop id=\"mount\" value=\"/\"/>"
              "          </disk>"
              "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"10000000000000B\"/>"
              "             <prop id=\"mount\" value=\"/otherdisk\"/>"
              "          </disk>"
              "          <disk id=\"scratch_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"100B\"/>"
              "             <prop id=\"mount\" value=\"/scratch\"/>"
              "          </disk>"
              "       </host>"
              "       <host id=\"Host2\" speed=\"1f\" core=\"10\" > "
              "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"10000000000000B\"/>"
              "             <prop id=\"mount\" value=\"/\"/>"
              "          </disk>"
              "          <disk id=\"scratch_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"100B\"/>"
              "             <prop id=\"mount\" value=\"/scratch\"/>"
              "          </disk>"
              "       </host>"
              "       <host id=\"Host3\" speed=\"1f\" core=\"10\" > "
              "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"10000000000000B\"/>"
              "             <prop id=\"mount\" value=\"/\"/>"
              "          </disk>"
              "          <disk id=\"scratch_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"100B\"/>"
              "             <prop id=\"mount\" value=\"/scratch\"/>"
              "          </disk>"
              "       </host>"
              "       <host id=\"Host4\" speed=\"1f\" core=\"10\" > "
              "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"100B\"/>"
              "             <prop id=\"mount\" value=\"/\"/>"
              "          </disk>"
              "          <disk id=\"scratch_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
              "             <prop id=\"size\" value=\"100B\"/>"
              "             <prop id=\"mount\" value=\"/scratch\"/>"
              "          </disk>"
              "       </host>"
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
                        const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                        std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                        std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, file_registry_service, hostname, "test") {
      this->test = test;
    }

private:

    FileRegistryTest *test;

    int main() {

      auto file1 = this->getWorkflow()->addFile("file1", 100.0);
      auto file2 = this->getWorkflow()->addFile("file2", 100.0);
      auto frs = this->getAvailableFileRegistryService();

      std::set<std::shared_ptr<wrench::FileLocation>> locations;

      try {
        frs->addEntry(nullptr, wrench::FileLocation::LOCATION(this->test->storage_service1));
        throw std::runtime_error("Should not be able to add an entry with a nullptr file");
      } catch (std::invalid_argument &e) {
      }

      try {
        frs->addEntry(file1, nullptr);
        throw std::runtime_error("Should not be able to add an entry with a nullptr storage service");
      } catch (std::invalid_argument &e) {
      }

      try {
        frs->lookupEntry(nullptr);
        throw std::runtime_error("Should not be able to lookup an entry with a nullptr file");
      } catch (std::invalid_argument &e) {
      }

      // Adding twice file1
      frs->addEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service1));
      frs->addEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service1));

      // Getting file1
      try {
        locations = frs->lookupEntry(file1);
      } catch (std::exception &e) {
        throw std::runtime_error("Should not have gotten an exception when looking up entry");
      }

      if (locations.size() != 1) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }


      if ((*locations.begin())->getStorageService() != this->test->storage_service1) {
        throw std::runtime_error("Got the wrong location for file 1");
      }

      // Adding another location for file1
      frs->addEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service2));

      // Getting file1
      locations = frs->lookupEntry(file1);

      if (locations.size() != 2) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }

      bool found_ss1 = false;
      bool found_ss2 = false;

      for (auto const &l : locations) {
        if (l->getStorageService() == this->test->storage_service1) {
          found_ss1 = true;
        }
        if (l->getStorageService() == this->test->storage_service2) {
          found_ss2 = true;
        }
      }
      if ((not found_ss1) || (not found_ss2)) {
        throw std::runtime_error("Got the wrong locations for file 1");
      }

      try {
        frs->removeEntry(nullptr, wrench::FileLocation::LOCATION(this->test->storage_service1));
        throw std::runtime_error("Should not be able to remove an entry with a nullptr file");
      } catch (std::invalid_argument &e) {
      }

      try {
        frs->removeEntry(file1, nullptr);
        throw std::runtime_error("Should not be able to remove an entry with a nullptr storage service");
      } catch (std::invalid_argument &e) {
      }

      frs->removeEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service1));

      // Doing it again, which does nothing (coverage)
      frs->removeEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service1));

      // Getting file1
      locations = frs->lookupEntry(file1);

      if (locations.size() != 1) {
        throw std::runtime_error("Got a wrong number of locations for file1");
      }
      if ((*(locations.begin()))->getStorageService() != this->test->storage_service2) {
        throw std::runtime_error("Got the wrong locations for file 1");
      }

      // Remove an already removed entry
      frs->removeEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service1));

      // Shutting down the service
      frs->stop();

      // Trying a removeEntry
      try {
        frs->removeEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service1));
        throw std::runtime_error("Should not be able to remove an entry from a service that is down");
      } catch (wrench::WorkflowExecutionException &e) {
        // Check Exception
        auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
        if (not cause) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                   e.getCause()->toString() + " (expected: ServiceIsDown)");
        }
        // Check Exception details
        if (cause->getService() != frs) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
        }
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
  char **argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("unit_test");

  simulation->init(&argc, argv);

  // Setting up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Get a hostname
  std::string hostname = wrench::Simulation::getHostnameList()[0];

  // Create a Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BareMetalComputeService(hostname,
                                              {std::make_pair(hostname, std::make_tuple(wrench::ComputeService::ALL_CORES, wrench::ComputeService::ALL_RAM))},
                                              {})));
  // Create a Storage Service
  ASSERT_NO_THROW(storage_service1 = simulation->add(
          new wrench::SimpleStorageService(hostname, {"/"})));

  // Create a Storage Service
  ASSERT_NO_THROW(storage_service2 = simulation->add(
          new wrench::SimpleStorageService(hostname, {"/otherdisk"})));

  // Create a file registry service
  std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
  ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(hostname)));

  // Create a WMS
  std::shared_ptr<wrench::WMS> wms = nullptr;;
  ASSERT_NO_THROW(wms = simulation->add(
          new FileRegistryTestWMS(
                  this,
                  {compute_service}, {storage_service1, storage_service2}, file_registry_service, hostname)));



  ASSERT_NO_THROW(wms->addWorkflow(workflow));

  // Running a "run a single task" simulation
  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  for (int i=0; i < argc; i++)
     free(argv[i]);
  free(argv);
}
/**********************************************************************/
/**   LOOKUP ENTRY BY PROXIMITY TEST                                 **/
/**********************************************************************/

class FileRegistryLookupEntryTestWMS : public wrench::WMS {

public:
    FileRegistryLookupEntryTestWMS(FileRegistryTest *test,
                                   const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                   const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                   const std::set<std::shared_ptr<wrench::NetworkProximityService>> &network_proximity_services,
                                   std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                   std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, network_proximity_services, file_registry_service, hostname, "test") {
      this->test = test;
    }

private:

    FileRegistryTest *test;

    int main() {

      wrench::WorkflowFile *file1 = this->getWorkflow()->addFile("file1", 100.0);
      wrench::WorkflowFile * nullptr_file = nullptr;
      auto frs = this->getAvailableFileRegistryService();
      auto nps = *(this->getAvailableNetworkProximityServices().begin());

      frs->addEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service1));
      frs->addEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service2));
      frs->addEntry(file1, wrench::FileLocation::LOCATION(this->test->storage_service3));

      wrench::S4U_Simulation::sleep(600.0);


      std::vector<std::string> file1_expected_locations = {"Host4", "Host1", "Host2"};
      std::vector<std::string> file1_locations(3);
      std::map<double, std::shared_ptr<wrench::FileLocation>> file1_locations_by_proximity;

      try {
        frs->lookupEntry(nullptr_file, "Host3", nps);
        throw std::runtime_error("Should not be able to lookup a nullptr file");
      } catch (std::invalid_argument &e) {
      }

      try {
        file1_locations_by_proximity = frs->lookupEntry(file1, "Host3", nps);
      } catch (std::exception &e) {
        throw std::runtime_error("Should be able to lookup a file");
      }

      int count=0;
      for (auto &storage_service : file1_locations_by_proximity) {
        file1_locations[count++] = storage_service.second->getStorageService()->getHostname();
      }

      bool is_equal = std::equal(file1_expected_locations.begin(), file1_expected_locations.end(),
                                 file1_locations.begin());

      if (!is_equal) {
        throw std::runtime_error("lookupEntry using NetworkProximityService did not return Storage Services in ascending order of Network Proximity");
      }

      auto last_location = file1_locations_by_proximity.rbegin();
      if (last_location->second->getStorageService()->getHostname() != "Host2") {
        throw std::runtime_error(
                "lookupEntry using NetworkProximityService did not return the correct unmonitored Storage Service");
      } else if (last_location->first != DBL_MAX) {
        throw std::runtime_error(
                "lookupEntry using NetworkProximityService did not include the unmonitored Storage Service");
      }


      // Use a bogus host
      try {
        frs->lookupEntry(file1, "BogusHost", nps);
        throw std::runtime_error("Should not be able to lookup a file by proximity with a bogus reference host");
      } catch (std::invalid_argument &e) {
      }

      // shutdown service
      frs->stop();

      try {
        frs->lookupEntry(file1);
        throw std::runtime_error("Should not be able to lookup a file when the service is down");
      } catch (wrench::WorkflowExecutionException &e) {
        // Check Exception
        auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
        if (not cause) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                   e.getCause()->toString() + " (expected: ServiceIsDown)");
        }
        // Check Exception details
        if (cause->getService() != frs) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
        }
      }

      try {
        file1_locations_by_proximity = frs->lookupEntry(file1, "Host3", nps);
        throw std::runtime_error("Should not be able to lookup a file when the service is down");
      } catch (wrench::WorkflowExecutionException &e) {
        // Check Exception
        auto cause = std::dynamic_pointer_cast<wrench::ServiceIsDown>(e.getCause());
        if (not cause) {
          throw std::runtime_error("Got an exception, as expected, but of the unexpected failure cause: " +
                                   e.getCause()->toString() + " (expected: ServiceIsDown)");
        }
        // Check Exception details
        if (cause->getService() != frs) {
          throw std::runtime_error(
                  "Got the expected 'service is down' exception, but the failure cause does not point to the correct service");
        }
      }

      return 0;
    }
};

TEST_F(FileRegistryTest, LookupEntry) {
  DO_TEST_WITH_FORK(do_lookupEntry_Test);
}

void FileRegistryTest::do_lookupEntry_Test() {

  // Create and initialize a simulation
  auto simulation = new wrench::Simulation();
  int argc = 1;
  char **argv = (char **) calloc(argc, sizeof(char *));
  argv[0] = strdup("unit_test");
//  argv[1] = strdup("--wrench-full-log");

  simulation->init(&argc, argv);

  simulation->instantiatePlatform(platform_file_path);

  std::string host1 = "Host1";
  std::string host2 = "Host2";
  std::string host3 = "Host3";
  std::string host4 = "Host4";

  auto network_proximity_service = simulation->add(new wrench::NetworkProximityService(host1, {host1, host3, host4}));

  storage_service1 = simulation->add(
          new wrench::SimpleStorageService(host1, {"/"}));

  storage_service2 = simulation->add(
          new wrench::SimpleStorageService(host2, {"/"}));

  storage_service3 = simulation->add(
          new wrench::SimpleStorageService(host4, {"/"}));


  auto file_registry_service = simulation->add(new wrench::FileRegistryService(host1));

  std::shared_ptr<wrench::WMS> wms = nullptr;;
  wms = simulation->add(
          new FileRegistryLookupEntryTestWMS(
                  this,
                  {}, {storage_service1, storage_service2, storage_service3}, {network_proximity_service}, file_registry_service, host1));

  wms->addWorkflow(workflow);

  ASSERT_NO_THROW(simulation->launch());

  delete simulation;

  for (int i=0; i < argc; i++)
     free(argv[i]);
  free(argv);
}
