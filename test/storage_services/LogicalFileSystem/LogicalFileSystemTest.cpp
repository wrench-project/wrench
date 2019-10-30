#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(logical_file_system_test, "Log category for LogicalFileSystemTest");


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
                          "          <disk id=\"large_disk1\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100MB\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk2\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100MB\"/>"
                          "             <prop id=\"mount\" value=\"/tmp\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk4\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
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

    int argc = 1;
    char **argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // set up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Logical File System
    auto fs1 = new wrench::LogicalFileSystem("Host", "ss1", "/");
    fs1->init();


    auto fs2 = new wrench::LogicalFileSystem("Host", "ss2", "/");
    ASSERT_THROW(fs2->init(), std::invalid_argument);

    auto fs3 = new wrench::LogicalFileSystem("Host", "ss1", "/tmp"); // coverage
    fs3->init();

    fs1->createDirectory(("/foo"));
    fs1->removeAllFilesInDirectory("/foo");
    fs1->listFilesInDirectory("/foo");
    fs1->removeEmptyDirectory("/foo");



    delete simulation;
    free(argv[0]);
    free(argv);
}
