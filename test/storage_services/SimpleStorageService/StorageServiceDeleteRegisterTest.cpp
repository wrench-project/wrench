#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#define FILE_SIZE 1000
#define STORAGE_SIZE (100 * FILE_SIZE)

class SimpleStorageServiceDeleteRegisterTest : public ::testing::Test {

public:
    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_2;

    std::shared_ptr<wrench::StorageService> storage_service = nullptr;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_DeleteRegisterTest();

protected:
    SimpleStorageServiceDeleteRegisterTest() {
      // simple workflow
      workflow = new wrench::Workflow();

      // create the files
      file_1 = workflow->addFile("file_1", FILE_SIZE);
      file_2 = workflow->addFile("file_2", FILE_SIZE);


      // Create a 2-host platform file
      // [WMSHost]-----[StorageHost]
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"StorageHost\" speed=\"1f\"> "
              "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
              "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "\"/>"
              "             <prop id=\"mount\" value=\"/\"/>"
              "          </disk>"
              "       </host>"
              "       <host id=\"WMSHost\" speed=\"1f\"/> "
              "       <link id=\"link\" bandwidth=\"10MBps\" latency=\"100us\"/>"
              "       <route src=\"WMSHost\" dst=\"StorageHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "   </zone> "
              "</platform>";
      FILE *platform_file = fopen(platform_file_path.c_str(), "w");
      fprintf(platform_file, "%s", xml.c_str());
      fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    wrench::Workflow *workflow;
};

class SimpleStorageServiceDeleteRegisterTestWMS : public wrench::WMS {
public:
    SimpleStorageServiceDeleteRegisterTestWMS(SimpleStorageServiceDeleteRegisterTest *test,
                                              const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                              const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                              std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                              std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, file_registry_service,
                        hostname, "test") {
      this->test = test;
    }

private:
    SimpleStorageServiceDeleteRegisterTest *test;

    int main() {

      // get the file registry service
      auto file_registry_service = this->getAvailableFileRegistryService();

      // get the single storage service
      auto storage_service = *(this->getAvailableStorageServices().begin());

      // register both files
      wrench::WorkflowFile *file_1 = this->test->file_1;
      wrench::WorkflowFile *file_2 = this->test->file_2;

      file_registry_service->addEntry(file_1, wrench::FileLocation::LOCATION(storage_service));
      file_registry_service->addEntry(file_2, wrench::FileLocation::LOCATION(storage_service));

      // delete file and don't unregister
      wrench::StorageService::deleteFile(file_1, wrench::FileLocation::LOCATION(storage_service));
      if (storage_service->lookupFile(file_1, nullptr)) {
        throw std::runtime_error("StorageService should have deleted file_1");
      }

      if (file_registry_service->lookupEntry(file_1).size() != 1) {
        throw std::runtime_error("The file_1 should still be registered with the FileRegistryService");
      }

      // delete file and unregister
      wrench::StorageService::deleteFile(file_2,
              wrench::FileLocation::LOCATION(storage_service), file_registry_service);
      if (storage_service->lookupFile(file_2, nullptr)) {
        throw std::runtime_error("StorageService should have deleted file_2");
      }

      if (file_registry_service->lookupEntry(file_2).size() != 0) {
        throw std::runtime_error("The file_2 should not be registered with the FileRegistryService");
      }



      return 0;
    }
};

TEST_F(SimpleStorageServiceDeleteRegisterTest, DeleteAndRegister) {
  DO_TEST_WITH_FORK(do_DeleteRegisterTest);
}

void SimpleStorageServiceDeleteRegisterTest::do_DeleteRegisterTest() {
  // Create and initialize the simulation
  wrench::Simulation *simulation = new wrench::Simulation();

  // ??
  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("unit_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // set up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a (unused) Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BareMetalComputeService("WMSHost",
                                                       {std::make_pair("WMSHost",
                                                                       std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                       wrench::ComputeService::ALL_RAM))}, {})));

  // Create One Storage Service
  ASSERT_NO_THROW(storage_service = simulation->add(
          new wrench::SimpleStorageService("StorageHost", {"/"})));

  // Create a file registry
  std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
  ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

  // Create a WMS
  std::shared_ptr<wrench::WMS> wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(new SimpleStorageServiceDeleteRegisterTestWMS(
          this, {compute_service}, {storage_service}, file_registry_service, "WMSHost")));

  wms->addWorkflow(this->workflow);



  // Stage the 2 files on the StorageHost
  ASSERT_NO_THROW(simulation->stageFile(file_1, wrench::FileLocation::LOCATION(storage_service)));
  ASSERT_NO_THROW(simulation->stageFile(file_2, wrench::FileLocation::LOCATION(storage_service)));


  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}
