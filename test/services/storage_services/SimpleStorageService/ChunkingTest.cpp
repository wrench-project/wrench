#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"


class SimpleStorageServiceChunkingTest : public ::testing::Test {

public:
    wrench::WorkflowFile *file_size_0;
    wrench::WorkflowFile *file_size_100;

    std::shared_ptr<wrench::StorageService> storage_service_1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_2 = nullptr;

    void do_ChunkingTest(std::string mode);

protected:
    SimpleStorageServiceChunkingTest() {
        // simple workflow
        workflow = new wrench::Workflow();

        // create the files
        file_size_0 = workflow->addFile("file_size_0", 0);
        file_size_100 = workflow->addFile("file_size_100", 100);


        // Create a 2-host platform file
        // [WMSHost]-----[StorageHost]
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
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
    wrench::Workflow *workflow;
};

class SimpleStorageServiceChunkingTestWMS : public wrench::WMS {
public:
    SimpleStorageServiceChunkingTestWMS(SimpleStorageServiceChunkingTest *test,
                                        std::string mode,
                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                        std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr, {}, storage_services, {}, file_registry_service,
                        hostname, "test") {
        this->test = test;
        this->mode = mode;
    }

private:
    SimpleStorageServiceChunkingTest *test;
    std::string mode;

    int main() {

        auto data_movement_manager = this->createDataMovementManager();

        if (mode == "reading") {
            wrench::StorageService::readFile(
                    this->test->file_size_0,
                    wrench::FileLocation::LOCATION(this->test->storage_service_1));
            wrench::StorageService::readFile(
                    this->test->file_size_100,
                    wrench::FileLocation::LOCATION(this->test->storage_service_1));

        } else if (mode == "writing") {
            wrench::StorageService::writeFile(
                    this->test->file_size_0,
                    wrench::FileLocation::LOCATION(this->test->storage_service_2));
            wrench::StorageService::writeFile(
                    this->test->file_size_100,
                    wrench::FileLocation::LOCATION(this->test->storage_service_2));

        } else if (mode == "copying") {
            data_movement_manager->doSynchronousFileCopy(this->test->file_size_0,
                                                         wrench::FileLocation::LOCATION(this->test->storage_service_1),
                                                         wrench::FileLocation::LOCATION(this->test->storage_service_2));
            data_movement_manager->doSynchronousFileCopy(this->test->file_size_100,
                                                         wrench::FileLocation::LOCATION(this->test->storage_service_1),
                                                         wrench::FileLocation::LOCATION(this->test->storage_service_2));

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
    auto simulation = new wrench::Simulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create One Storage Service
    ASSERT_NO_THROW(storage_service_1 = simulation->add(
            new wrench::SimpleStorageService("StorageHost", {"/disk1"},
                                             {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "5"},
                                              {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}}
            )));

    // Create Another Storage Service
    ASSERT_NO_THROW(storage_service_2 = simulation->add(
            new wrench::SimpleStorageService("StorageHost", {"/disk2"},
                                             {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10"},
                                              {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}}
            )));

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new SimpleStorageServiceChunkingTestWMS(
            this, mode, {storage_service_1, storage_service_2}, file_registry_service, "WMSHost")));

    wms->addWorkflow(this->workflow);

    // Stage the file on the StorageHost
    ASSERT_NO_THROW(simulation->stageFile(file_size_0, storage_service_1));
    ASSERT_NO_THROW(simulation->stageFile(file_size_100, storage_service_1));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;
    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
