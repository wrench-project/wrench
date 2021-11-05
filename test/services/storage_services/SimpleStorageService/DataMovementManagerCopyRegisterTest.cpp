#include <gtest/gtest.h>

#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(data_movement_manager_copy_register_test, "Log category for DataMovementManagerCopyRegisterTest");


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

    std::shared_ptr<wrench::StorageService> dst_storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> src_storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> src2_storage_service = nullptr;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

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
                          "       <host id=\"SrcHost\" speed=\"1f\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "B\"/>"
                                                                                                     "             <prop id=\"mount\" value=\"/\"/>"
                                                                                                     "          </disk>"
                                                                                                     "       </host>"
                                                                                                     "       <host id=\"DstHost\" speed=\"1f\"> "
                                                                                                     "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                                                     "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "B\"/>"
                                                                                                                                                                                "             <prop id=\"mount\" value=\"/\"/>"
                                                                                                                                                                                "          </disk>"
                                                                                                                                                                                "       </host>"
                                                                                                                                                                                "       <host id=\"WMSHost\" speed=\"1f\"> "
                                                                                                                                                                                "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                                                                                                                                                "             <prop id=\"size\" value=\"" + std::to_string(STORAGE_SIZE) + "B\"/>"
                                                                                                                                                                                                                                                           "             <prop id=\"mount\" value=\"/\"/>"
                                                                                                                                                                                                                                                           "          </disk>"
                                                                                                                                                                                                                                                           "       </host>"
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
                                           const std::set<std::shared_ptr<wrench::ComputeService>> compute_services,
                                           const std::set<std::shared_ptr<wrench::StorageService>> storage_services,
                                           std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                           std::string hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, file_registry_service,
                        hostname, "test") {
        this->test = test;
    }

private:
    DataMovementManagerCopyRegisterTest *test;

    int main() {


        auto data_movement_manager = this->createDataMovementManager();
        auto file_registry_service = this->getAvailableFileRegistryService();

        // try synchronous copy and register

        try {
            data_movement_manager->doSynchronousFileCopy(this->test->src_file_1,
                                                         wrench::FileLocation::LOCATION(this->test->src_storage_service),
                                                         wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                         file_registry_service);

        } catch (wrench::ExecutionEvent &e)  {
            throw std::runtime_error("Synchronous file copy failed");
        }

        auto src_file_1_locations = file_registry_service->lookupEntry(this->test->src_file_1);
        bool found = false;
        for (auto const &l : src_file_1_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service))) {
                found = true;
            }
        }

        if (not found) {
            throw std::runtime_error("Synchronous file copy succeeded but file was not registered at DstHost");
        }

        // Do the same thing but kill the FileRegistryService first

        wrench::StorageService::deleteFile(this->test->src_file_1,
                                           wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                           file_registry_service);
        file_registry_service->stop();
        try {
            data_movement_manager->doSynchronousFileCopy(this->test->src_file_1,
                                                         wrench::FileLocation::LOCATION(this->test->src_storage_service),
                                                         wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                         file_registry_service);

            throw std::runtime_error("Synchronous file copy failed");
        } catch (wrench::ExecutionException &e)  {
        }

        // Create a new file registry service to resume normal testing
        file_registry_service = std::shared_ptr<wrench::FileRegistryService>(
                new wrench::FileRegistryService(this->hostname, {}, {}));
        file_registry_service->setSimulation(this->simulation);
        file_registry_service->start(file_registry_service, true, false);


        // try asynchronous copy and register
        std::shared_ptr<wrench::ExecutionEvent> async_copy_event;

        try {
            data_movement_manager->initiateAsynchronousFileCopy(this->test->src2_file_1,
                                                                wrench::FileLocation::LOCATION(this->test->src2_storage_service),
                                                                wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                                file_registry_service);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an exception while trying to instantiate a file copy: " + std::string(e.what()));
        }

        try {
            async_copy_event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(async_copy_event)) {
            throw std::runtime_error("Asynchronous file copy failed.");
        }

        auto src2_file_1_locations = file_registry_service->lookupEntry(this->test->src2_file_1);

        found = false;
        for (auto const &l : src2_file_1_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service))) {
                found = true;
            }
        }

        if (not found) {
            throw std::runtime_error("Asynchronous file copy succeeded but file was not registered at DstHost.");
        }

        // try 2 asynchronous copies of the same file
        bool double_copy_failed = false;
        std::shared_ptr<wrench::ExecutionEvent> async_dual_copy_event;

        data_movement_manager->initiateAsynchronousFileCopy(this->test->src_file_2,
                                                            wrench::FileLocation::LOCATION(this->test->src_storage_service),
                                                            wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                            file_registry_service);

        try {
            data_movement_manager->doSynchronousFileCopy(this->test->src_file_2,
                                                         wrench::FileLocation::LOCATION(this->test->src_storage_service),
                                                         wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                         file_registry_service);
        } catch (wrench::ExecutionException &e) {
            double_copy_failed = true;
        }

        async_dual_copy_event = this->getWorkflow()->waitForNextExecutionEvent();

        auto src_file_2_locations = file_registry_service->lookupEntry(this->test->src_file_2);

        found = false;
        for (auto const &l : src_file_2_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service))) {
                found = true;
            }
        }

        if (not found) {
            throw std::runtime_error("Asynchronous file copy succeeded but file was not registered at DstHost");
        }

        if (!double_copy_failed) {
            throw std::runtime_error("The second asynchronous file copy should have failed.");
        }

        if (not std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(async_copy_event)) {
            throw std::runtime_error("Asynchronous copy event should be FileCompletedEvent. Instead: " + async_copy_event->toString());
        }

        // try 1 asynchronous and 1 synchronous copy of the same file
        double_copy_failed = false;

        std::shared_ptr<wrench::ExecutionEvent> async_dual_copy_event2;


        data_movement_manager->initiateAsynchronousFileCopy(this->test->src_file_3,
                                                            wrench::FileLocation::LOCATION(this->test->src_storage_service),
                                                            wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                            file_registry_service);

        try {
            data_movement_manager->doSynchronousFileCopy(this->test->src_file_3,
                                                         wrench::FileLocation::LOCATION(this->test->src_storage_service),
                                                         wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                         file_registry_service);
        } catch (wrench::ExecutionException &e) {
            if (std::dynamic_pointer_cast<wrench::FileAlreadyBeingCopied>(e.getCause())) {
                double_copy_failed = true;
            }
        }

        async_dual_copy_event2 = this->getWorkflow()->waitForNextExecutionEvent();

        auto async_dual_copy_event2_real = std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(async_dual_copy_event2);

        if (not async_dual_copy_event2_real)  {
            throw std::runtime_error(std::string("Unexpected workflow execution event: " + async_dual_copy_event2->toString()));
        }


        if (!async_dual_copy_event2_real->file_registry_service_updated) {
            throw std::runtime_error("Asynchronous file copy should have set the event's file_registry_service_updated variable to true");
        }

        if (!double_copy_failed) {
            throw std::runtime_error("Synchronous file copy should have failed.");
        }

        auto src_file_3_locations = file_registry_service->lookupEntry(this->test->src_file_3);

        found = false;
        for (auto const &l : src_file_3_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service))) {
                found = true;
            }
        }

        if (not found) {
            throw std::runtime_error("File was not registered after Asynchronous copy completed.");
        }

        // try 1 asynchronous copy and then kill the file registry service right after the copy is instantiated
        std::shared_ptr<wrench::ExecutionEvent> async_copy_event2;

        data_movement_manager->initiateAsynchronousFileCopy(this->test->src2_file_2,
                                                            wrench::FileLocation::LOCATION(this->test->src2_storage_service),
                                                            wrench::FileLocation::LOCATION(this->test->dst_storage_service),
                                                            file_registry_service);

        file_registry_service->stop();

        async_copy_event2 = this->getWorkflow()->waitForNextExecutionEvent();

        auto async_copy_event2_real = std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(async_copy_event2);

        if (not async_copy_event2_real) {
            throw std::runtime_error("Asynchronous file copy should have completed");
        }

        if (async_copy_event2_real->file_registry_service_updated) {
            throw std::runtime_error("File registry service should not have been updated");
        }

        if (not wrench::StorageService::lookupFile(this->test->src2_file_2,
                                                   wrench::FileLocation::LOCATION(this->test->dst_storage_service))) {
            throw std::runtime_error("Asynchronous file copy should have completed even though the FileRegistryService was down.");
        }

        // Stop the data movement manager
        data_movement_manager->stop();

        return 0;
    }
};

TEST_F(DataMovementManagerCopyRegisterTest, CopyAndRegister) {
    DO_TEST_WITH_FORK(do_CopyRegister_test);
}

void DataMovementManagerCopyRegisterTest::do_CopyRegister_test() {
    // Create and initialize the simulation
    auto simulation = new wrench::Simulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

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
            new wrench::SimpleStorageService("SrcHost", {"/"})));

    ASSERT_NO_THROW(src2_storage_service = simulation->add(
            new wrench::SimpleStorageService("WMSHost", {"/"})
    ));

    ASSERT_NO_THROW(dst_storage_service = simulation->add(
            new wrench::SimpleStorageService("DstHost", {"/"})));

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new DataMovementManagerCopyRegisterTestWMS(
            this, {compute_service}, {src_storage_service, src2_storage_service, dst_storage_service}, file_registry_service, "WMSHost")));

    wms->addWorkflow(this->workflow);

    // Stage the 2 files on the StorageHost
    ASSERT_NO_THROW(simulation->stageFile(src_file_1, src_storage_service));
    ASSERT_NO_THROW(simulation->stageFile(src_file_2, src_storage_service));
    ASSERT_NO_THROW(simulation->stageFile(src_file_3, src_storage_service));

    ASSERT_NO_THROW(simulation->stageFile(src2_file_1, src2_storage_service));
    ASSERT_NO_THROW(simulation->stageFile(src2_file_2, src2_storage_service));
    ASSERT_NO_THROW(simulation->stageFile(src2_file_3, src2_storage_service));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}