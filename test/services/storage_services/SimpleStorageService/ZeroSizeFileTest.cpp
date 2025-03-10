#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"


class SimpleStorageServiceZeroSizeFileTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::DataFile> file;

    std::shared_ptr<wrench::StorageService> storage_service = nullptr;

    void do_ReadZeroSizeFileTest(double buffer_size);

protected:
    ~SimpleStorageServiceZeroSizeFileTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    SimpleStorageServiceZeroSizeFileTest() {
        // simple workflow
        workflow = wrench::Workflow::createWorkflow();

        // create the files
        file = wrench::Simulation::addFile("file_1", 0);


        // Create a 2-host platform file
        // [WMSHost]-----[StorageHost]
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"StorageHost\" speed=\"1f\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10B\"/>"
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
    std::shared_ptr<wrench::Workflow> workflow;
};

class SimpleStorageServiceZeroSizeFileTestWMS : public wrench::ExecutionController {
public:
    SimpleStorageServiceZeroSizeFileTestWMS(SimpleStorageServiceZeroSizeFileTest *test,
                                            std::shared_ptr<wrench::StorageService> &storage_service,
                                            std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                            std::string hostname) : wrench::ExecutionController(hostname, "test"),
                                                                    storage_service(storage_service), file_registry_service(file_registry_service) {
        this->test = test;
    }

private:
    SimpleStorageServiceZeroSizeFileTest *test;
    std::shared_ptr<wrench::StorageService> storage_service;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service;

    int main() override {

        // read the file
        wrench::StorageService::readFileAtLocation(
                wrench::FileLocation::LOCATION(storage_service, this->test->file));


        return 0;
    }
};

TEST_F(SimpleStorageServiceZeroSizeFileTest, ReadZeroSizeFile) {
    DO_TEST_WITH_FORK_ONE_ARG(do_ReadZeroSizeFileTest, 1000000);
    DO_TEST_WITH_FORK_ONE_ARG(do_ReadZeroSizeFileTest, 0);
}

void SimpleStorageServiceZeroSizeFileTest::do_ReadZeroSizeFileTest(double buffer_size) {
    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create One Storage Service
    ASSERT_NO_THROW(storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost", {"/"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, std::to_string(buffer_size)}}, {})));

    // Create a file registry
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService("WMSHost")));

    // Create a WMS
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new SimpleStorageServiceZeroSizeFileTestWMS(
                            this, storage_service, file_registry_service, "WMSHost")));

    // Stage the file on the StorageHost
    ASSERT_NO_THROW(storage_service->createFile(file));

    ASSERT_NO_THROW(simulation->launch());


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
