#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"


class CopyBufferizedNonBufferizedTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::DataFile> file_1_size_100;
    std::shared_ptr<wrench::DataFile> file_2_size_200;

    std::shared_ptr<wrench::StorageService> storage_service_bufferized_1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_bufferized_2 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_non_bufferized_1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_non_bufferized_2 = nullptr;

    void do_CopyBufferizedNonBufferizedTest_test();
    void do_SelfCopyBufferizedNonBufferizedTest_test();

protected:
    ~CopyBufferizedNonBufferizedTest() {
        wrench::Simulation::removeFile(file_1_size_100);
        wrench::Simulation::removeFile(file_2_size_200);
    }

    CopyBufferizedNonBufferizedTest() {

        // create the files
        file_1_size_100 = wrench::Simulation::addFile("file_1_size_100", 100 * 1000 * 1000);
        file_2_size_200 = wrench::Simulation::addFile("file_2_size_200", 200 * 1000 * 1000);

        // Create a 2-host platform file
        // [WMSHost]-----[StorageHost]
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"StorageHost1\" speed=\"1f\"> "
                          "          <disk id=\"disk1\" read_bw=\"100MBps\" write_bw=\"200MBps\">"
                          "             <prop id=\"size\" value=\"1000MB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"disk2\" read_bw=\"100MBps\" write_bw=\"200MBps\">"
                          "             <prop id=\"size\" value=\"1000MB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"StorageHost2\" speed=\"1f\"> "
                          "          <disk id=\"disk1\" read_bw=\"200MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000MB\"/>"
                          "             <prop id=\"mount\" value=\"/disk1\"/>"
                          "          </disk>"
                          "          <disk id=\"disk2\" read_bw=\"200MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"1000MB\"/>"
                          "             <prop id=\"mount\" value=\"/disk2\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"WMSHost\" speed=\"1f\"/> "
                          "       <link id=\"link\" bandwidth=\"100Bps\" latency=\"100us\"/>"
                          "       <route src=\"WMSHost\" dst=\"StorageHost1\">"
                          "         <link_ctn id=\"link\"/>"
                          "       </route>"
                          "       <route src=\"WMSHost\" dst=\"StorageHost2\">"
                          "         <link_ctn id=\"link\"/>"
                          "       </route>"
                          "       <route src=\"StorageHost1\" dst=\"StorageHost2\">"
                          "         <link_ctn id=\"link\"/>"
                          "       </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/***********************************************************************************/
/** COPY BUFFERIZED / NON-BUFFERIZED                                              **/
/***********************************************************************************/

class CopyBufferizedNonBufferizedTestWMS : public wrench::ExecutionController {
public:
    CopyBufferizedNonBufferizedTestWMS(CopyBufferizedNonBufferizedTest *test,
                                       std::string hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CopyBufferizedNonBufferizedTest *test;
    std::string mode;

    int main() override {

        // Create the files
        this->test->storage_service_bufferized_1->createFile(this->test->file_1_size_100);
        this->test->storage_service_non_bufferized_1->createFile(this->test->file_2_size_200);

        auto data_movement_manager = this->createDataMovementManager();

        //        std::cerr << "DOING BUFFERIZED TO BUFFERIZED\n";
        data_movement_manager->doSynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service_bufferized_1, this->test->file_1_size_100),
                wrench::FileLocation::LOCATION(this->test->storage_service_bufferized_2, this->test->file_1_size_100));

        if (not this->test->storage_service_bufferized_2->lookupFile(this->test->file_1_size_100)) {
            throw std::runtime_error("Cannot find copied file after bufferized to bufferized");
        }

        //        std::cerr << "DOING BUFFERIZED TO NON-BUFFERIZED\n";
        data_movement_manager->doSynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service_bufferized_1, this->test->file_1_size_100),
                wrench::FileLocation::LOCATION(this->test->storage_service_non_bufferized_2, this->test->file_1_size_100));
        if (not this->test->storage_service_non_bufferized_2->lookupFile(this->test->file_1_size_100)) {
            throw std::runtime_error("Cannot find copied file after bufferized to non-bufferized");
        }

        //        std::cerr << "DOING NON-BUFFERIZED TO BUFFERIZED\n";
        data_movement_manager->doSynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service_non_bufferized_1, this->test->file_2_size_200),
                wrench::FileLocation::LOCATION(this->test->storage_service_bufferized_2, this->test->file_2_size_200));
        if (not this->test->storage_service_bufferized_2->lookupFile(this->test->file_2_size_200)) {
            throw std::runtime_error("Cannot find copied file after non-bufferized to bufferized");
        }

        //        std::cerr << "DOING NON-BUFFERIZED TO NON-BUFFERIZED\n";
        data_movement_manager->doSynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service_non_bufferized_1, this->test->file_2_size_200),
                wrench::FileLocation::LOCATION(this->test->storage_service_non_bufferized_2, this->test->file_2_size_200));
        if (not this->test->storage_service_non_bufferized_2->lookupFile(this->test->file_2_size_200)) {
            throw std::runtime_error("Cannot find copied file after non-bufferized to non-bufferized");
        }

        return 0;
    }
};

TEST_F(CopyBufferizedNonBufferizedTest, BufferizedNonBufferized) {
    DO_TEST_WITH_FORK(do_CopyBufferizedNonBufferizedTest_test);
}


void CopyBufferizedNonBufferizedTest::do_CopyBufferizedNonBufferizedTest_test() {

    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    ASSERT_NO_THROW(storage_service_bufferized_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost1", {"/disk1"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1MB"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));
    ASSERT_NO_THROW(storage_service_non_bufferized_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost1", {"/disk2"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));
    ASSERT_NO_THROW(storage_service_bufferized_2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost2", {"/disk1"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "2MB"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));
    ASSERT_NO_THROW(storage_service_non_bufferized_2 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost2", {"/disk2"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));

    // Create a WMS
    ASSERT_NO_THROW(simulation->add(new CopyBufferizedNonBufferizedTestWMS(this, "WMSHost")));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/***********************************************************************************/
/** SELF-COPY BUFFERIZED / NON-BUFFERIZED                                         **/
/***********************************************************************************/

class SelfCopyBufferizedNonBufferizedTestWMS : public wrench::ExecutionController {
public:
    SelfCopyBufferizedNonBufferizedTestWMS(CopyBufferizedNonBufferizedTest *test,
                                           const std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test) {
    }

private:
    CopyBufferizedNonBufferizedTest *test;
    std::string mode;

    int main() override {

        // Create the files
        this->test->storage_service_bufferized_1->createFile(this->test->file_1_size_100, "/disk1");
        this->test->storage_service_non_bufferized_1->createFile(this->test->file_2_size_200, "/disk1");

        auto data_movement_manager = this->createDataMovementManager();

        data_movement_manager->doSynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service_bufferized_1, "/disk1", this->test->file_1_size_100),
                wrench::FileLocation::LOCATION(this->test->storage_service_bufferized_1, "/disk2", this->test->file_1_size_100));

        if (not this->test->storage_service_bufferized_1->lookupFile(this->test->file_1_size_100, "/disk2")) {
            throw std::runtime_error("Cannot find copied file after self copy bufferized");
        }

        data_movement_manager->doSynchronousFileCopy(
                wrench::FileLocation::LOCATION(this->test->storage_service_non_bufferized_1, "/disk1", this->test->file_2_size_200),
                wrench::FileLocation::LOCATION(this->test->storage_service_non_bufferized_1, "/disk2", this->test->file_2_size_200));

        if (not this->test->storage_service_non_bufferized_1->lookupFile(this->test->file_2_size_200, "/disk2")) {
            throw std::runtime_error("Cannot find copied file after self copy non-bufferized");
        }


        return 0;
    }
};

TEST_F(CopyBufferizedNonBufferizedTest, SelfCopy) {
    DO_TEST_WITH_FORK(do_SelfCopyBufferizedNonBufferizedTest_test);
}


void CopyBufferizedNonBufferizedTest::do_SelfCopyBufferizedNonBufferizedTest_test() {

    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    ASSERT_NO_THROW(storage_service_bufferized_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost1", {"/disk1", "/disk2"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "1MB"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));
    ASSERT_NO_THROW(storage_service_non_bufferized_1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("StorageHost2", {"/disk1", "/disk2"},
                                                                                     {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "0"},
                                                                                      {wrench::SimpleStorageServiceProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "10"}})));
    // Create a WMS
    ASSERT_NO_THROW(simulation->add(new SelfCopyBufferizedNonBufferizedTestWMS(this, "WMSHost")));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}