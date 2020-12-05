#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(logical_file_system_test, "Log category for LogicalFileSystemTest");


class LogicalFileSystemTest : public ::testing::Test {

public:

    void do_BasicTests();

protected:
    LogicalFileSystemTest() {

        // Create a 2-host platform file
        // [WMSHost]-----[StorageHost]
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host\" speed=\"1f\"> "
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100MB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100MB\"/>"
                          "             <prop id=\"mount\" value=\"/tmp\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk4\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100MB\"/>"
                          "             <prop id=\"mount\" value=\"/home/users\"/>"
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
    auto simulation = new wrench::Simulation();
    auto workflow = new wrench::Workflow();

    int argc = 2;
    char **argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    ASSERT_THROW(new  wrench::LogicalFileSystem("Host", nullptr, "/"), std::invalid_argument);

    // Create two Storage Services
    std::shared_ptr<wrench::SimpleStorageService> storage_service1, storage_service2;
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService("Host", {"/"})));
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService("Host", {"/"})));

    // Create a Logical File System
    auto fs1 = new wrench::LogicalFileSystem("Host", storage_service1.get(), "/");
    fs1->init();

    // Attempt to create a redundant Logical File System
    auto fs1_bogus = new wrench::LogicalFileSystem("Host", storage_service2.get(), "/");
    try {
        fs1_bogus->init();
        throw std::runtime_error("Initializing a redundant file system should have thrown");
    } catch (std::invalid_argument &e) {
        //  ignored
    }

    ASSERT_THROW(new wrench::LogicalFileSystem("Host", storage_service1.get(), "/bogus"), std::invalid_argument);

    fs1->createDirectory(("/foo"));
    fs1->removeAllFilesInDirectory("/foo");
    fs1->listFilesInDirectory("/foo");
    fs1->removeEmptyDirectory("/foo");

    auto file = workflow->addFile("file", 10000000000);
    auto file1 = workflow->addFile("file1", 10000);
    ASSERT_THROW(fs1->reserveSpace(file, "/files/"), std::invalid_argument);

    delete simulation;
    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
