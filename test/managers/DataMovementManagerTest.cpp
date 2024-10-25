#include <gtest/gtest.h>

#include <utility>

#include <utility>

#include <memory>

#include "wrench-dev.h"

#include "../include/TestWithFork.h"
#include "../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(data_movement_manager_test, "Log category for DataMovementManagerTest");


#define FILE_SIZE 100000.00
#define STORAGE_SIZE (100 * FILE_SIZE)

class DataMovementManagerTest : public ::testing::Test {

public:
    void do_AsyncCopy_test();
    void do_AsyncWrite_test();
    void do_AsyncRead_test();


    std::shared_ptr<wrench::DataFile> src_file_1;
    std::shared_ptr<wrench::DataFile> src_file_2;
    std::shared_ptr<wrench::DataFile> src_file_3;

    std::shared_ptr<wrench::DataFile> src2_file_1;
    std::shared_ptr<wrench::DataFile> src2_file_2;
    std::shared_ptr<wrench::DataFile> src2_file_3;

    std::shared_ptr<wrench::SimpleStorageService> dst_storage_service = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> src_storage_service = nullptr;
    std::shared_ptr<wrench::SimpleStorageService> src2_storage_service = nullptr;

    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;


protected:
    ~DataMovementManagerTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    DataMovementManagerTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create the files
        src_file_1 = wrench::Simulation::addFile("file_1", FILE_SIZE);
        src_file_2 = wrench::Simulation::addFile("file_2", FILE_SIZE);
        src_file_3 = wrench::Simulation::addFile("file_3", FILE_SIZE);

        src2_file_1 = wrench::Simulation::addFile("file_4", FILE_SIZE);
        src2_file_2 = wrench::Simulation::addFile("file_5", FILE_SIZE);
        src2_file_3 = wrench::Simulation::addFile("file_6", FILE_SIZE);

        // Create a 3-host platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"SrcHost\" speed=\"1f\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"" +
                          std::to_string(STORAGE_SIZE) + "B\"/>"
                                                         "             <prop id=\"mount\" value=\"/\"/>"
                                                         "          </disk>"
                                                         "       </host>"
                                                         "       <host id=\"DstHost\" speed=\"1f\"> "
                                                         "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                         "             <prop id=\"size\" value=\"" +
                          std::to_string(STORAGE_SIZE) + "B\"/>"
                                                         "             <prop id=\"mount\" value=\"/\"/>"
                                                         "          </disk>"
                                                         "       </host>"
                                                         "       <host id=\"WMSHost\" speed=\"1f\"> "
                                                         "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                                                         "             <prop id=\"size\" value=\"" +
                          std::to_string(STORAGE_SIZE) + "B\"/>"
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
    std::shared_ptr<wrench::Workflow> workflow;
};

/**********************************************************************/
/**  ASYNC COPY  TEST                                                **/
/**********************************************************************/

class DataMovementManagerAsyncCopyTestWMS : public wrench::ExecutionController {

public:
    DataMovementManagerAsyncCopyTestWMS(DataMovementManagerTest *test,
                                        std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                        std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test), file_registry_service(std::move(std::move(file_registry_service))) {
    }

private:
    DataMovementManagerTest *test;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service;

    int main() override {


        auto data_movement_manager = this->createDataMovementManager();

        // try synchronous copy and register

        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_1),
                    wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src_file_1),
                    file_registry_service);

        } catch (wrench::ExecutionEvent &e) {
            throw std::runtime_error("Synchronous file copy failed");
        }

        auto src_file_1_locations = file_registry_service->lookupEntry(this->test->src_file_1);
        bool found = false;
        for (auto const &l: src_file_1_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service, l->getFile()))) {
                found = true;
            }
        }

        if (not found) {
            throw std::runtime_error("Synchronous file copy succeeded but file was not registered at DstHost");
        }

        // Do the same thing but kill the FileRegistryService first

        wrench::StorageService::deleteFileAtLocation(
                wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src_file_1));
        file_registry_service->stop();
        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_1),
                    wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src_file_1),
                    file_registry_service);

            throw std::runtime_error("Synchronous file copy failed");
        } catch (wrench::ExecutionException &e) {
        }

        // Create a new file registry service to resume normal testing
        file_registry_service = std::shared_ptr<wrench::FileRegistryService>(new wrench::FileRegistryService(
                this->hostname, {}, {}));
        file_registry_service->setSimulation(this->getSimulation());
        file_registry_service->start(file_registry_service, true, false);

        // try asynchronous copy and register
        std::shared_ptr<wrench::ExecutionEvent> async_copy_event;

        try {
            data_movement_manager->initiateAsynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->src2_storage_service, this->test->src2_file_1),
                    wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src2_file_1),
                    file_registry_service);
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Got an exception while trying to instantiate a file copy: " + std::string(e.what()));
        }

        try {
            async_copy_event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
        }

        if (std::dynamic_pointer_cast<wrench::FileCopyFailedEvent>(async_copy_event)) {
            throw std::runtime_error("Asynchronous file copy failed.");
        }

        auto src2_file_1_locations = file_registry_service->lookupEntry(this->test->src2_file_1);

        found = false;
        for (auto const &l: src2_file_1_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service, l->getFile()))) {
                found = true;
            }
        }

        if (not found) {
            throw std::runtime_error("Asynchronous file copy succeeded but file was not registered at DstHost.");
        }


        // try 2 asynchronous copies of the same file
        bool double_copy_failed = false;
        std::shared_ptr<wrench::ExecutionEvent> async_dual_copy_event;

        data_movement_manager->initiateAsynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_2),
                wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src_file_2),
                file_registry_service);

        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_2),
                    wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src_file_2),
                    file_registry_service);
        } catch (wrench::ExecutionException &e) {
            double_copy_failed = true;
        }

        async_dual_copy_event = this->waitForNextEvent();

        auto src_file_2_locations = file_registry_service->lookupEntry(this->test->src_file_2);

        found = false;
        for (auto const &l: src_file_2_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service, l->getFile()))) {
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


        data_movement_manager->initiateAsynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_3),
                wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src_file_3),
                file_registry_service);

        try {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_3),
                    wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src_file_3),
                    file_registry_service);
        } catch (wrench::ExecutionException &e) {
            if (std::dynamic_pointer_cast<wrench::FileAlreadyBeingCopied>(e.getCause())) {
                double_copy_failed = true;
            }
        }

        async_dual_copy_event2 = this->waitForNextEvent();

        auto async_dual_copy_event2_real = std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(async_dual_copy_event2);

        if (not async_dual_copy_event2_real) {
            throw std::runtime_error(std::string("Unexpected workflow execution event: " + async_dual_copy_event2->toString()));
        }


        //
        if (!double_copy_failed) {
            throw std::runtime_error("Synchronous file copy should have failed.");
        }

        auto src_file_3_locations = file_registry_service->lookupEntry(this->test->src_file_3);

        found = false;
        for (auto const &l: src_file_3_locations) {
            if (wrench::FileLocation::equal(l, wrench::FileLocation::LOCATION(this->test->dst_storage_service, l->getFile()))) {
                found = true;
            }
        }

        if (not found) {
            throw std::runtime_error("File was not registered after Asynchronous copy completed.");
        }

        // try 1 asynchronous copy and then kill the file registry service right after the copy is instantiated
        std::shared_ptr<wrench::ExecutionEvent> async_copy_event2;

        data_movement_manager->initiateAsynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->src2_storage_service, this->test->src2_file_2),
                wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src2_file_2),
                file_registry_service);

        file_registry_service->stop();

        async_copy_event2 = this->waitForNextEvent();

        auto async_copy_event2_real = std::dynamic_pointer_cast<wrench::FileCopyCompletedEvent>(async_copy_event2);

        if (not async_copy_event2_real) {
            throw std::runtime_error("Asynchronous file copy should have completed");
        }

        //

        if (not wrench::StorageService::lookupFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src2_file_2))) {
            throw std::runtime_error("Asynchronous file copy should have completed even though the FileRegistryService was down.");
        }

        // Stop the data movement manager
        data_movement_manager->stop();

        return 0;
    }
};

TEST_F(DataMovementManagerTest, AsyncCopy) {
    DO_TEST_WITH_FORK(do_AsyncCopy_test);
}

void DataMovementManagerTest::do_AsyncCopy_test() {
    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a (unused) Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("WMSHost",
                                                                {std::make_pair("WMSHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                           wrench::ComputeService::ALL_RAM))},
                                                                {})));

    // Create src and dst storage services
    ASSERT_NO_THROW(src_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SrcHost", {"/"})));

    ASSERT_NO_THROW(src2_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("WMSHost", {"/"})));

    ASSERT_NO_THROW(dst_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("DstHost", {"/"})));

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new DataMovementManagerAsyncCopyTestWMS(
                            this, file_registry_service, "WMSHost")));

    // Stage the 2 files on the StorageHost
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_1));
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_2));
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_3));

    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_1));
    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_2));
    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_3));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ASYNC WRITE  TEST                                               **/
/**********************************************************************/

class DataMovementManagerAsyncWriteTestWMS : public wrench::ExecutionController {

public:
    DataMovementManagerAsyncWriteTestWMS(DataMovementManagerTest *test,
                                         std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                         std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test), file_registry_service(std::move(file_registry_service)) {
    }

private:
    DataMovementManagerTest *test;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service;

    int main() override {

        auto data_movement_manager = this->createDataMovementManager();

        {

            try {
                data_movement_manager->initiateAsynchronousFileWrite(nullptr, file_registry_service);
                throw std::runtime_error("Should not be able to do a file write with a nullptr location");
            } catch (std::invalid_argument &ignore) {
            }


            // try asynchronous write and register that should work
            std::shared_ptr<wrench::ExecutionEvent> async_write_event;

            auto write_location = wrench::FileLocation::LOCATION(this->test->dst_storage_service, this->test->src2_file_1);

            try {
                data_movement_manager->initiateAsynchronousFileWrite(
                        write_location,
                        file_registry_service);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Got an exception while trying to instantiate a file write: " + e.getCause()->toString());
            }

            wrench::Simulation::sleep(0.01);

            try {
                data_movement_manager->initiateAsynchronousFileWrite(
                        write_location,
                        file_registry_service);
                throw std::runtime_error("Should not be able to concurrently write to the same location");
            } catch (wrench::ExecutionException &e) {
                auto real_cause = std::dynamic_pointer_cast<wrench::FileAlreadyBeingWritten>(e.getCause());
                if (not real_cause) {
                    throw std::runtime_error("Unexpected failure cause: " + e.getCause()->toString());
                }
                real_cause->toString();   // coverage
                real_cause->getLocation();// coverage
            }


            try {
                async_write_event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
            }

            if (std::dynamic_pointer_cast<wrench::FileWriteFailedEvent>(async_write_event)) {
                throw std::runtime_error("Asynchronous file write failed (" + std::dynamic_pointer_cast<wrench::FileWriteFailedEvent>(async_write_event)->toString() + ")");
            }
            auto real_event = std::dynamic_pointer_cast<wrench::FileWriteCompletedEvent>(async_write_event);
            if (not real_event) {
                throw std::runtime_error("Received an unexpected event: " + async_write_event->toString());
            }
            real_event->toString();// coverage

            // Check that the file registry has been updated
            auto lookup = file_registry_service->lookupEntry(this->test->src2_file_1);
            if (lookup.empty()) {
                throw std::runtime_error("File registry service has not been updated");
            }
            if (lookup.size() > 2) {
                throw std::runtime_error("File registry service has been updated too much???");
            }
            bool found_it = false;
            for (auto const &entry: lookup) {
                if (entry->equal(write_location)) {
                    found_it = true;
                    break;
                }
            }
            if (not found_it) {
                throw std::runtime_error("Something's wrong in the file registry entry");
            }
        }

        {
            // try asynchronous write and register that should NOT work
            std::shared_ptr<wrench::ExecutionEvent> async_write_event;

            auto too_big = wrench::Simulation::addFile("too_bit", 1000000000000000ULL);
            auto write_location = wrench::FileLocation::LOCATION(this->test->dst_storage_service, too_big);

            try {
                data_movement_manager->initiateAsynchronousFileWrite(
                        write_location,
                        file_registry_service);
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Got an exception while trying to instantiate a file write: " + std::string(e.what()));
            }

            try {
                async_write_event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
            }

            if (std::dynamic_pointer_cast<wrench::FileWriteCompletedEvent>(async_write_event)) {
                throw std::runtime_error("Asynchronous file write should have failed (" + std::dynamic_pointer_cast<wrench::FileWriteFailedEvent>(async_write_event)->toString() + ")");
            }
            auto real_event = std::dynamic_pointer_cast<wrench::FileWriteFailedEvent>(async_write_event);
            if (not real_event) {
                throw std::runtime_error("Received an unexpected event: " + async_write_event->toString());
            }
            real_event->toString();// coverage

            if (not std::dynamic_pointer_cast<wrench::StorageServiceNotEnoughSpace>(real_event->failure_cause)) {
                throw std::runtime_error("wrong failure cause: " + real_event->failure_cause->toString());
            }
        }

        // Stop the data movement manager
        data_movement_manager->stop();

        return 0;
    }
};

TEST_F(DataMovementManagerTest, AsyncWrite) {
    DO_TEST_WITH_FORK(do_AsyncWrite_test);
}

void DataMovementManagerTest::do_AsyncWrite_test() {
    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a (unused) Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("WMSHost",
                                                                {std::make_pair("WMSHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                           wrench::ComputeService::ALL_RAM))},
                                                                {})));

    // Create src and dst storage services
    ASSERT_NO_THROW(src_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SrcHost", {"/"})));

    ASSERT_NO_THROW(src2_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("WMSHost", {"/"})));

    ASSERT_NO_THROW(dst_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("DstHost", {"/"})));

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new DataMovementManagerAsyncWriteTestWMS(
                            this, file_registry_service, "WMSHost")));

    // Stage the 2 files on the StorageHost
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_1));
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_2));
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_3));

    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_1));
    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_2));
    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_3));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  ASYNC READ  TEST                                                **/
/**********************************************************************/

class DataMovementManagerAsyncReadTestWMS : public wrench::ExecutionController {

public:
    DataMovementManagerAsyncReadTestWMS(DataMovementManagerTest *test,
                                        std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    DataMovementManagerTest *test;

    int main() override {

        auto data_movement_manager = this->createDataMovementManager();


        {


            try {
                data_movement_manager->initiateAsynchronousFileRead(nullptr, 10);
                throw std::runtime_error("Should not be able to do a file read with a nullptr location");
            } catch (std::invalid_argument &ignore) {
            }


            // try asynchronous read that should work
            std::shared_ptr<wrench::ExecutionEvent> async_read_event;

            try {
                data_movement_manager->initiateAsynchronousFileRead(
                        wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_1), this->test->src_file_1->getSize());
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Got an exception while trying to instantiate a file read: " + std::string(e.what()));
            }

            wrench::Simulation::sleep(0.01);

            try {
                data_movement_manager->initiateAsynchronousFileRead(
                        wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src_file_1));
                throw std::runtime_error("Should not be able to concurrently read from the same location");
            } catch (wrench::ExecutionException &e) {
                auto real_cause = std::dynamic_pointer_cast<wrench::FileAlreadyBeingRead>(e.getCause());
                if (not real_cause) {
                    throw std::runtime_error("Unexpected failure cause: " + real_cause->toString());
                }
                real_cause->toString();   // coverage
                real_cause->getLocation();// coverage
            }


            try {
                async_read_event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
            }

            if (std::dynamic_pointer_cast<wrench::FileReadFailedEvent>(async_read_event)) {
                throw std::runtime_error("Asynchronous file read failed (" + std::dynamic_pointer_cast<wrench::FileReadFailedEvent>(async_read_event)->toString() + ")");
            }
            auto real_event = std::dynamic_pointer_cast<wrench::FileReadCompletedEvent>(async_read_event);
            if (not real_event) {
                throw std::runtime_error("Received an unexpected event: " + async_read_event->toString());
            }
            real_event->toString();// coverage
        }

        {
            // try asynchronous read that should NOT work
            std::shared_ptr<wrench::ExecutionEvent> async_read_event;

            try {
                data_movement_manager->initiateAsynchronousFileRead(
                        wrench::FileLocation::LOCATION(this->test->src_storage_service, this->test->src2_file_1), this->test->src_file_1->getSize());
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Got an exception while trying to instantiate a file read: " + std::string(e.what()));
            }

            try {
                async_read_event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting an execution event: " + e.getCause()->toString());
            }

            if (std::dynamic_pointer_cast<wrench::FileReadCompletedEvent>(async_read_event)) {
                throw std::runtime_error("Asynchronous file read should have failed (" + std::dynamic_pointer_cast<wrench::FileReadCompletedEvent>(async_read_event)->toString() + ")");
            }
            auto real_event = std::dynamic_pointer_cast<wrench::FileReadFailedEvent>(async_read_event);
            if (not real_event) {
                throw std::runtime_error("Received an unexpected event: " + async_read_event->toString());
            }
            real_event->toString();// coverage
            if (not std::dynamic_pointer_cast<wrench::FileNotFound>(real_event->failure_cause)) {
                throw std::runtime_error("Got the expected event, but and unexpected failure cause: " + real_event->failure_cause->toString());
            }
        }

        // Stop the data movement manager
        data_movement_manager->stop();

        return 0;
    }
};

TEST_F(DataMovementManagerTest, AsyncRead) {
    DO_TEST_WITH_FORK(do_AsyncRead_test);
}

void DataMovementManagerTest::do_AsyncRead_test() {
    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //        argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a (unused) Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BareMetalComputeService("WMSHost",
                                                                {std::make_pair("WMSHost", std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                           wrench::ComputeService::ALL_RAM))},
                                                                {})));

    // Create src and dst storage services
    ASSERT_NO_THROW(src_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("SrcHost", {"/"})));

    ASSERT_NO_THROW(src2_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("WMSHost", {"/"})));

    ASSERT_NO_THROW(dst_storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("DstHost", {"/"})));

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new DataMovementManagerAsyncReadTestWMS(
                            this, "WMSHost")));

    // Stage files
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_1));
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_2));
    ASSERT_NO_THROW(src_storage_service->createFile(src_file_3));

    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_1));
    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_2));
    ASSERT_NO_THROW(src2_storage_service->createFile(src2_file_3));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}