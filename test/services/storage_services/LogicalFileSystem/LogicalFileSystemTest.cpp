#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(logical_file_system_test, "Log category for LogicalFileSystemTest");


class LogicalFileSystemTest : public ::testing::Test {

public:
    void do_BasicTests();
    void do_DevNullTests();
    void do_LRUTests();

protected:
    LogicalFileSystemTest() {

        // Create a 2-host platform file
        // [WMSHost]-----[StorageHost]
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host\" speed=\"1f\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100MB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"100bytedisk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/tmp\"/>"
                          "          </disk>"
                          "       </host>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


TEST_F(LogicalFileSystemTest, BasicTests) {
    DO_TEST_WITH_FORK(do_BasicTests);
}

void LogicalFileSystemTest::do_BasicTests() {
    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //        argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));
    auto workflow = wrench::Workflow::createWorkflow();

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create two Storage Services
    std::shared_ptr<wrench::SimpleStorageService> storage_service1, storage_service2;
    ASSERT_NO_THROW(storage_service1 = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("Host", {"/"})));

    ASSERT_THROW(wrench::LogicalFileSystem::createLogicalFileSystem("Host", nullptr, "/tmp", "NONE"), std::invalid_argument);
    ASSERT_THROW(wrench::LogicalFileSystem::createLogicalFileSystem("Host", storage_service1.get(), "/bogus"), std::invalid_argument);

    // Create a Logical File System
    try {
        wrench::LogicalFileSystem::createLogicalFileSystem("Host", storage_service1.get(), "/tmp", "BOGUS");
        throw std::runtime_error("Should not be able to create a logical file system with a bogus caching policy");
    } catch (std::invalid_argument &ignore) {}

    auto fs1 = wrench::LogicalFileSystem::createLogicalFileSystem("Host", storage_service1.get(), "/tmp", "NONE");

    // Attempt to create a redundant Logical File System
    try {
        wrench::LogicalFileSystem::createLogicalFileSystem("Host", storage_service1.get(), "/tmp");
        throw std::runtime_error("Initializing a redundant file system should have thrown");
    } catch (std::invalid_argument &ignore) {
    }

    fs1->createDirectory(("/foo"));
    fs1->removeAllFilesInDirectory("/foo");
    fs1->listFilesInDirectory("/foo");
    fs1->removeEmptyDirectory("/foo");

    ASSERT_DOUBLE_EQ(100, fs1->getTotalCapacity());
    auto file_80 = wrench::Simulation::addFile("file_80", 80);
    ASSERT_TRUE(fs1->reserveSpace(file_80, "/files/"));
    fs1->unreserveSpace(file_80, "/files/");
    ASSERT_DOUBLE_EQ(100, fs1->getFreeSpace());
    ASSERT_TRUE(fs1->reserveSpace(file_80, "/files/"));
    fs1->storeFileInDirectory(file_80, "/files/");
    ASSERT_DOUBLE_EQ(20, fs1->getFreeSpace());
    fs1->incrementNumRunningTransactionsForFileInDirectory(file_80, "/files");// coverage
    fs1->decrementNumRunningTransactionsForFileInDirectory(file_80, "/files");// coverage

    auto file_50 = wrench::Simulation::addFile("file_50", 50);
    ASSERT_FALSE(fs1->reserveSpace(file_50, "/files/"));
    ASSERT_DOUBLE_EQ(20, fs1->getFreeSpace());
    fs1->removeFileFromDirectory(file_80, "/files/");
    ASSERT_DOUBLE_EQ(100, fs1->getFreeSpace());

    fs1->storeFileInDirectory(file_50, "/faa");
    fs1->removeAllFilesInDirectory("/faa/");// coverage


    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


TEST_F(LogicalFileSystemTest, DevNullTests) {
    DO_TEST_WITH_FORK(do_DevNullTests);
}

void LogicalFileSystemTest::do_DevNullTests() {
    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));
    auto workflow = wrench::Workflow::createWorkflow();

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a  Storage Services
    std::shared_ptr<wrench::SimpleStorageService> storage_service;
    ASSERT_NO_THROW(storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("Host", {"/"})));

    // Create a Logical File System
    auto fs1 = wrench::LogicalFileSystem::createLogicalFileSystem("Host", storage_service.get(), "/dev/null");

    auto file = wrench::Simulation::addFile("file", 1.0);

    fs1->createDirectory(("/foo"));
    fs1->stageFile(file, "/foo");
    ASSERT_FALSE(fs1->doesDirectoryExist(("/foo")));
    ASSERT_TRUE(fs1->isDirectoryEmpty(("/foo")));
    ASSERT_FALSE(fs1->isFileInDirectory(file, "/foo"));
    fs1->removeEmptyDirectory("/foo");
    fs1->storeFileInDirectory(file, "/foo");
    fs1->removeFileFromDirectory(file, "/foo");
    fs1->removeAllFilesInDirectory("/foo");
    ASSERT_TRUE(fs1->listFilesInDirectory("/foo").empty());
    fs1->reserveSpace(file, "/foo");
    fs1->unreserveSpace(file, "/foo");
    fs1->getFileLastWriteDate(file, "/foo");

    // Create a Logical File System
    auto fs2 = wrench::LogicalFileSystem::createLogicalFileSystem("Host", storage_service.get(), "/dev/null", "LRU");

    fs2->createDirectory(("/foo"));
    ASSERT_FALSE(fs2->doesDirectoryExist(("/foo")));
    ASSERT_TRUE(fs2->isDirectoryEmpty(("/foo")));
    ASSERT_FALSE(fs2->isFileInDirectory(file, "/foo"));
    fs2->removeEmptyDirectory("/foo");
    fs2->storeFileInDirectory(file, "/foo");
    fs2->removeFileFromDirectory(file, "/foo");
    fs2->removeAllFilesInDirectory("/foo");
    ASSERT_TRUE(fs2->listFilesInDirectory("/foo").empty());
    fs2->reserveSpace(file, "/foo");
    fs2->unreserveSpace(file, "/foo");
    fs2->getFileLastWriteDate(file, "/foo");

    workflow->clear();
    wrench::Simulation::removeAllFiles();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


TEST_F(LogicalFileSystemTest, LRUTests) {
    DO_TEST_WITH_FORK(do_LRUTests);
}

void LogicalFileSystemTest::do_LRUTests() {
    // Create and initialize the simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    //    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a  Storage Services
    std::shared_ptr<wrench::SimpleStorageService> storage_service;
    ASSERT_NO_THROW(storage_service = simulation->add(
                            wrench::SimpleStorageService::createSimpleStorageService("Host", {"/"})));

    // Create a Logical File System with LRU eviction
    auto fs1 = wrench::LogicalFileSystem::createLogicalFileSystem("Host", storage_service.get(), "/tmp", "LRU");

    auto file_60 = wrench::Simulation::addFile("file_60", 60);
    auto file_50 = wrench::Simulation::addFile("file_50", 50);
    auto file_30 = wrench::Simulation::addFile("file_30", 30);
    auto file_20 = wrench::Simulation::addFile("file_20", 20);
    auto file_10 = wrench::Simulation::addFile("file_10", 10);


    fs1->createDirectory(("/foo"));
    ASSERT_TRUE(fs1->reserveSpace(file_60, "/foo"));
    ASSERT_FALSE(fs1->reserveSpace(file_50, "/foo"));
    fs1->storeFileInDirectory(file_60, "/foo");
    ASSERT_DOUBLE_EQ(40, fs1->getFreeSpace());
    fs1->storeFileInDirectory(file_10, "/foo");
    ASSERT_DOUBLE_EQ(30, fs1->getFreeSpace());

    ASSERT_TRUE(fs1->reserveSpace(file_50, "/foo"));
    // Check that file_60 has been evicted
    ASSERT_FALSE(fs1->isFileInDirectory(file_60, "/foo"));
    // Check that file_10 is still there evicted
    ASSERT_TRUE(fs1->isFileInDirectory(file_10, "/foo"));
    fs1->storeFileInDirectory(file_50, "/foo");
    ASSERT_DOUBLE_EQ(40, fs1->getFreeSpace());

    // At this point the content is:
    //  If I store another file that requires 50 bytes, but make file_10 unevictable, file_50 should be evicted
    auto other_file_50 = wrench::Simulation::addFile("other_file_50", 50);
    fs1->incrementNumRunningTransactionsForFileInDirectory(file_10, "/foo");
    ASSERT_TRUE(fs1->reserveSpace(other_file_50, "/foo"));
    ASSERT_TRUE(fs1->isFileInDirectory(file_10, "/foo"));
    ASSERT_FALSE(fs1->isFileInDirectory(file_50, "/foo"));
    fs1->storeFileInDirectory(other_file_50, "/foo");
    fs1->updateReadDate(other_file_50, "/foo");
    fs1->updateReadDate(other_file_50, "/faa");// coverage

    // At this point the content is;
    //   LRU: file_10 (UNEVICTABLE), other_file_50 (EVICTABLE)
    fs1->incrementNumRunningTransactionsForFileInDirectory(other_file_50, "/foo");
    // At this point the content is;
    //   LRU: file_10 (UNEVICTABLE), other_file_50 (UNEVICTABLE)
    // I should not be able to store/reserve space for file_50
    ASSERT_FALSE(fs1->reserveSpace(file_50, "/foo"));
    ASSERT_DOUBLE_EQ(fs1->getFreeSpace(), 40);
    // Make other_file_50 EVICTABLE again
    fs1->decrementNumRunningTransactionsForFileInDirectory(other_file_50, "/foo");
    ASSERT_TRUE(fs1->reserveSpace(file_50, "/foo"));
    ASSERT_FALSE(fs1->isFileInDirectory(file_50, "/foo"));
    ASSERT_DOUBLE_EQ(fs1->getFreeSpace(), 40);


    fs1->removeFileFromDirectory(file_10, "/foo");// coverage
    fs1->storeFileInDirectory(file_10, "/foo");   // coverage
    fs1->removeAllFilesInDirectory("/foo");       // coverage

    fs1->storeFileInDirectory(file_10, "/faa");// coverage

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
