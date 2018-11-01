#include <gtest/gtest.h>

#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#define FILE_SIZE 100000.00
#define STORAGE_SIZE (100 * FILE_SIZE)

class DataMovementManagerCopyRegisterTest : public ::testing::Test {

public:
    wrench::WorkflowFile *src_file_1;
    wrench::WorkflowFile *src_file_2;
    wrench::WorkflowFile *src_file_3;

    wrench::WorkflowFile *src2_file_1;
    wrench::WorkflowFile *src2_file_2;
    wrench::WorkflowFile *src2_file_3;

    wrench::StorageService *dst_storage_service = nullptr;
    wrench::StorageService *src_storage_service = nullptr;
    wrench::StorageService *src2_storage_service = nullptr;

    wrench::ComputeService *compute_service = nullptr;

    void do_CopyRegister_test();

protected:
    DataMovementManagerCopyRegisterTest() {

      // Create the simplest workflow
      workflow = new wrench::Workflow();

      // Create the files
      src_file_1 = workflow->addFile("file_1", FILE_SIZE);
      src_file_2 = workflow->addFile("file_2", FILE_SIZE);
      src_file_3 = workflow->addFile("file_3", FILE_SIZE);

      src2_file_1 = workflow->addFile("file_4", FILE_SIZE);
      src2_file_2 = workflow->addFile("file_5", FILE_SIZE);
      src2_file_3 = workflow->addFile("file_6", FILE_SIZE);

      // Create a 3-host platform file
      std::string xml = "<?xml version='1.0'?>"
              "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
              "<platform version=\"4.1\"> "
              "   <zone id=\"AS0\" routing=\"Full\"> "
              "       <host id=\"SrcHost\" speed=\"1f\"/> "
              "       <host id=\"DstHost\" speed=\"1f\"/> "
              "       <host id=\"WMSHost\" speed=\"1f\"/> "
              "       <link id=\"link\" bandwidth=\"10MBps\" latency=\"100us\"/>"
              "       <route src=\"SrcHost\" dst=\"DstHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"SrcHost\">"
              "         <link_ctn id=\"link\"/>"
              "       </route>"
              "       <route src=\"WMSHost\" dst=\"DstHost\">"
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

/**********************************************************************/
/**  FILE COPY AND REGISTER TEST                                     **/
/**********************************************************************/

class DataMovementManagerCopyRegisterTestWMS : public wrench::WMS {

public:
    DataMovementManagerCopyRegisterTestWMS(DataMovementManagerCopyRegisterTest *test,
                                           const std::set<wrench::ComputeService *> compute_services,
                                           const std::set<wrench::StorageService *> storage_services,
                                           wrench::FileRegistryService *file_registry_service,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, file_registry_service,
                        hostname, "test") {
      this->test = test;
    }

private:
    DataMovementManagerCopyRegisterTest *test;

    int main() {


      std::shared_ptr<wrench::DataMovementManager> data_movement_manager = this->createDataMovementManager();
      wrench::FileRegistryService *file_registry_service = this->getAvailableFileRegistryService();

      // try synchronous copy and register
      bool success = true;
      bool is_registered_at_dst = true;

      try {
        data_movement_manager->doSynchronousFileCopy(this->test->src_file_1, this->test->src_storage_service,
                                                     this->test->dst_storage_service, file_registry_service);

      } catch (wrench::WorkflowExecutionEvent) {
        success = false;
      }

      std::set<wrench::StorageService *> src_file_1_locations = file_registry_service->lookupEntry(this->test->src_file_1);
      auto dst_search = src_file_1_locations.find(this->test->dst_storage_service);

      if (dst_search == src_file_1_locations.end()) {
        is_registered_at_dst = false;
      }

      if (!success) {
        throw std::runtime_error("Synchronous file copy failed");
      }

      if (!is_registered_at_dst) {
        throw std::runtime_error("Synchronous file copy succeeded but file was not registered at DstHost");
      }


      // try asynchronous copy and register
      std::unique_ptr<wrench::WorkflowExecutionEvent> async_copy_event;

      try {
        data_movement_manager->initiateAsynchronousFileCopy(this->test->src2_file_1,
                                                            this->test->src2_storage_service, this->test->dst_storage_service,
                                                            file_registry_service);
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Got an exception while trying to instantiate a file copy: " + std::string(e.what()));
      }

      try {
        async_copy_event = this->getWorkflow()->waitForNextExecutionEvent();
      } catch (wrench::WorkflowExecutionException &e) {
        throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
      }

      if (async_copy_event->type == wrench::WorkflowExecutionEvent::EventType::FILE_COPY_FAILURE) {
        throw std::runtime_error("Asynchronous file copy failed.");
      }

      std::set<wrench::StorageService *> src2_file_1_locations = file_registry_service->lookupEntry(this->test->src2_file_1);
      dst_search = src2_file_1_locations.find(this->test->dst_storage_service);

      if (dst_search == src2_file_1_locations.end()) {
        throw std::runtime_error("Asynchronous file copy succeeded but file was not registered at DstHost.");
      }

      // try 2 asynchronous copies of the same file
      bool double_copy_failed = false;
      std::unique_ptr<wrench::WorkflowExecutionEvent> async_dual_copy_event;

      data_movement_manager->initiateAsynchronousFileCopy(this->test->src_file_2,
                                                          this->test->src_storage_service,
                                                          this->test->dst_storage_service,
                                                          file_registry_service);

      try {
        data_movement_manager->doSynchronousFileCopy(this->test->src_file_2,
                                                     this->test->src_storage_service,
                                                     this->test->dst_storage_service,
                                                     file_registry_service);
      } catch (wrench::WorkflowExecutionException &e) {
        double_copy_failed = true;
      }

      async_dual_copy_event = this->getWorkflow()->waitForNextExecutionEvent();

      std::set<wrench::StorageService *> src_file_2_locations = file_registry_service->lookupEntry(this->test->src_file_2);
      dst_search = src_file_2_locations.find(this->test->dst_storage_service);

      if (dst_search == src_file_2_locations.end()) {
        throw std::runtime_error("Asynchronous file copy succeeded but file was not registered at DstHost");
      }

      if (!double_copy_failed) {
        throw std::runtime_error("The second asynchronous file copy should have failed.");
      }

      if (async_copy_event->type != wrench::WorkflowExecutionEvent::EventType::FILE_COPY_COMPLETION) {
        throw std::runtime_error("Asynchronous copy event should be of type WorkflowExecutionEvent::EventType::FILE_COPY_COMPLETION.");
      }

      // try 1 asynchronous and 1 synchronous copy of the same file
      double_copy_failed = false;

      std::unique_ptr<wrench::WorkflowExecutionEvent> async_dual_copy_event2;


      data_movement_manager->initiateAsynchronousFileCopy(this->test->src_file_3,
                                                          this->test->src_storage_service,
                                                          this->test->dst_storage_service, file_registry_service);

      try {
        data_movement_manager->doSynchronousFileCopy(this->test->src_file_3,
                                                     this->test->src_storage_service,
                                                     this->test->dst_storage_service, file_registry_service);
      } catch (wrench::WorkflowExecutionException &e) {
        if (e.getCause()->getCauseType() == wrench::FailureCause::FILE_ALREADY_BEING_COPIED) {
          double_copy_failed = true;
        }
      }

      async_dual_copy_event2 = this->getWorkflow()->waitForNextExecutionEvent();

      if (async_dual_copy_event2->type != wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION)  {
        throw std::runtime_error(std::string("Unexpected workflow execution even (type=") + std::to_string(async_dual_copy_event2->type) + ")");
      }

      auto async_dual_copy_event2_real = dynamic_cast<wrench::FileCopyCompletedEvent *>(async_dual_copy_event2.get());

      if (!async_dual_copy_event2_real->file_registry_service_updated) {
        throw std::runtime_error("Asynchronous file copy should have set the event's file_registry_service_updated variable to true");
      }

      if (!double_copy_failed) {
        throw std::runtime_error("Synchronous file copy should have failed.");
      }

      std::set<wrench::StorageService *> src_file_3_locations = file_registry_service->lookupEntry(this->test->src_file_3);
      dst_search = src_file_3_locations.find(this->test->dst_storage_service);

      if (dst_search == src_file_3_locations.end()) {
        throw std::runtime_error("File was not registered after Asynchronous copy completed.");
      }

      // try 1 asynchronous copy and then kill the file registry service right after the copy is instantiated
      std::unique_ptr<wrench::WorkflowExecutionEvent> async_copy_event2;

      data_movement_manager->initiateAsynchronousFileCopy(this->test->src2_file_2,
                                                          this->test->src2_storage_service,
                                                          this->test->dst_storage_service, file_registry_service);

      file_registry_service->stop();

      async_copy_event2 = this->getWorkflow()->waitForNextExecutionEvent();

      if (async_copy_event2->type != wrench::WorkflowExecutionEvent::FILE_COPY_COMPLETION) {
        throw std::runtime_error("Asynchronous file copy should have completed");
      }

      auto async_copy_event2_real = dynamic_cast<wrench::FileCopyCompletedEvent *>(async_copy_event2.get());


      if (async_copy_event2_real->file_registry_service_updated) {
        throw std::runtime_error("File registry service should not have been updated");
      }

      if (!this->test->dst_storage_service->lookupFile(this->test->src2_file_2, nullptr)) {
        throw std::runtime_error("Asynchronous file copy should have completed even though the FileRegistryService was down.");
      }

      return 0;
    }
};

TEST_F(DataMovementManagerCopyRegisterTest, CopyAndRegister) {
  DO_TEST_WITH_FORK(do_CopyRegister_test);
}

void DataMovementManagerCopyRegisterTest::do_CopyRegister_test() {
  // Create and initialize the simulation
  wrench::Simulation *simulation = new wrench::Simulation();

  int argc = 1;
  char **argv = (char **) calloc(1, sizeof(char *));
  argv[0] = strdup("copy_register_test");

  ASSERT_NO_THROW(simulation->init(&argc, argv));

  // set up the platform
  ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

  // Create a (unused) Compute Service
  ASSERT_NO_THROW(compute_service = simulation->add(
          new wrench::BareMetalComputeService("WMSHost",
                                                       {std::make_pair("WMSHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                        wrench::ComputeService::ALL_RAM))}, {})));

  // Create src and dst storage services
  ASSERT_NO_THROW(src_storage_service = simulation->add(
          new wrench::SimpleStorageService("SrcHost", STORAGE_SIZE)));

  ASSERT_NO_THROW(src2_storage_service = simulation->add(
          new wrench::SimpleStorageService("WMSHost", STORAGE_SIZE)
  ));

  ASSERT_NO_THROW(dst_storage_service = simulation->add(
          new wrench::SimpleStorageService("DstHost", STORAGE_SIZE)));

  // Create a file registry
  wrench::FileRegistryService *file_registry_service = nullptr;
  ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

  // Create a WMS
  wrench::WMS *wms = nullptr;
  ASSERT_NO_THROW(wms = simulation->add(new DataMovementManagerCopyRegisterTestWMS(
          this, {compute_service}, {src_storage_service, src2_storage_service, dst_storage_service}, file_registry_service, "WMSHost")));

  wms->addWorkflow(this->workflow);

  // Stage the 2 files on the StorageHost
  ASSERT_NO_THROW(simulation->stageFiles({{src_file_1->getID(), src_file_1},
                                          {src_file_2->getID(), src_file_2},
                                          {src_file_3->getID(), src_file_3}}, src_storage_service));

  ASSERT_NO_THROW(simulation->stageFiles({{src2_file_1->getID(), src2_file_1},
                                          {src2_file_2->getID(), src2_file_2},
                                          {src2_file_3->getID(), src2_file_3}}, src2_storage_service));

  ASSERT_NO_THROW(simulation->launch());

  delete simulation;
  free(argv[0]);
  free(argv);
}