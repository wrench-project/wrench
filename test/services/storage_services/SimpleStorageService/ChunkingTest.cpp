#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"


class SimpleStorageServiceChunkingTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::DataFile> file_size_0;
    std::shared_ptr<wrench::DataFile> file_size_100;

    std::shared_ptr<wrench::StorageService> storage_service_1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_2 = nullptr;

    void do_ChunkingTest(std::string mode);

protected:
    ~SimpleStorageServiceChunkingTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    SimpleStorageServiceChunkingTest() {
        // simple workflow
        workflow = wrench::Workflow::createWorkflow();

        // create the files
        file_size_0 = wrench::Simulation::addFile("file_size_0", 0);
        file_size_100 = wrench::Simulation::addFile("file_size_100", 100);


        // Create a 2-host platform file
        // [WMSHost]-----[StorageHost]
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"StorageHost\" speed=\"1f\"> "
                          "          <disk id=\"disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000B\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"WMSHost\" speed=\"1f\"/> "
                          "       <link id=\"link\" bandwidth=\"100Bps\" latency=\"100us\"/>"
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
    std::shared_ptr<wrench::Workflow> workflow;
};

class SimpleStorageServiceChunkingTestWMS : public wrench::ExecutionController {
public:
    SimpleStorageServiceChunkingTestWMS(SimpleStorageServiceChunkingTest *test,
                                        std::string mode,
                                        std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test), mode(mode) {
    }

private:
    SimpleStorageServiceChunkingTest *test;
    std::string mode;

    int main() override {

        auto data_movement_manager = this->createDataMovementManager();

        if (mode == "reading") {
            wrench::StorageService::readFileAtLocation(

                    wrench::FileLocation::LOCATION(this->test->storage_service_1, this->test->file_size_0));
            wrench::StorageService::readFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, this->test->file_size_100));

        } else if (mode == "writing") {
            wrench::StorageService::writeFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_2, this->test->file_size_0));
            wrench::StorageService::writeFileAtLocation(
                    wrench::FileLocation::LOCATION(this->test->storage_service_2, this->test->file_size_100));

        } else if (mode == "copying") {
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, this->test->file_size_0),
                    wrench::FileLocation::LOCATION(this->test->storage_service_2, this->test->file_size_0));
            data_movement_manager->doSynchronousFileCopy(
                    wrench::FileLocation::LOCATION(this->test->storage_service_1, this->test->file_size_100),
                    wrench::FileLocation::LOCATION(this->test->storage_service_2, this->test->file_size_100));
        }

        return 0;
    }
};

TEST_F(SimpleStorageServiceChunkingTest, ReadingFile) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ChunkingTest, "reading");
}

TEST_F(SimpleStorageServiceChunkingTest, WritingFile) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ChunkingTest, "writing");
}

TEST_F(SimpleStorageServiceChunkingTest, CopyingFile) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ChunkingTest, "copying");
}

void SimpleStorageServiceChunkingTest::do_ChunkingTest(std::string mode) {

    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create One Storage Service
    ASSERT_NO_THROW(storage_service_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost", {"/disk1"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "5"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));

    // Create Another Storage Service
    ASSERT_NO_THROW(storage_service_2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost", {"/disk2"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new SimpleStorageServiceChunkingTestWMS(this, mode, "WMSHost")));

    // Stage the file on the StorageHost
    ASSERT_NO_THROW(storage_service_1->createFile(file_size_0));
    ASSERT_NO_THROW(storage_service_1->createFile(file_size_100));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
