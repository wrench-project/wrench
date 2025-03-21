
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"


WRENCH_LOG_CATEGORY(simulation_timestamp_disk_read_write_test, "Log category for SimulationTimestampDiskReadWriteTest");


class SimulationTimestampDiskReadWriteTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service_1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service_2 = nullptr;

    std::shared_ptr<wrench::DataFile> file_1;

    void do_SimulationTimestampDiskReadWriteBasic_test();

protected:
    ~SimulationTimestampDiskReadWriteTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    SimulationTimestampDiskReadWriteTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"10000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"10000us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

        workflow = wrench::Workflow::createWorkflow();

        file_1 = wrench::Simulation::addFile("file_1", 100);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;
};

/**********************************************************************/
/**            SimulationTimestampDiskReadWriteTestBasic             **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampDiskReadWrite class.
 * This test ensures that SimulationTimestampDiskReadWriteStart, SimulationTimestampDiskReadWriteFailure,
 * and SimulationTimestampDiskReadWriteCompletion objects are added to their respective simulation
 * traces at the appropriate times.
 */
class SimulationTimestampDiskReadWriteBasicTestWMS : public wrench::ExecutionController {
public:
    SimulationTimestampDiskReadWriteBasicTestWMS(SimulationTimestampDiskReadWriteTest *test,
                                                 std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampDiskReadWriteTest *test;

    int main() override {


        wrench::StorageService::readFileAtLocation(
                wrench::FileLocation::LOCATION(this->test->storage_service_1, this->test->file_1));

        wrench::StorageService::writeFileAtLocation(
                wrench::FileLocation::LOCATION(this->test->storage_service_2, this->test->file_1));


        return 0;
    }
};

TEST_F(SimulationTimestampDiskReadWriteTest, SimulationTimestampDiskReadWriteBasicTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampDiskReadWriteBasic_test);
}

void SimulationTimestampDiskReadWriteTest::do_SimulationTimestampDiskReadWriteBasic_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string host1 = "Host1";
    std::string host2 = "Host2";

    ASSERT_NO_THROW(storage_service_1 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(host1, {"/"},
                                                                                                                 {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10"}})));

    ASSERT_NO_THROW(storage_service_2 = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(host2, {"/"},
                                                                                                                 {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "20"}})));

    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(host1)));


    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampDiskReadWriteBasicTestWMS(
                            this, host1)));

    //stage files
    std::set<std::shared_ptr<wrench::DataFile>> files_to_stage = {file_1};

    for (auto const &f: files_to_stage) {
        ASSERT_NO_THROW(storage_service_1->createFile(f, "/"));
    }

    simulation->getOutput().enableDiskTimestamps(true);

    ASSERT_NO_THROW(simulation->launch());

    auto diskread_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampDiskReadStart>();
    auto diskwrite_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampDiskWriteStart>();

    // check number of SimulationTimestampDiskReadWrite
    ASSERT_EQ(10, diskread_timestamps.size());
    ASSERT_EQ(5, diskwrite_timestamps.size());

    // Coverage
    auto rb = diskread_timestamps.front()->getContent()->getBytes();
    auto rc = diskread_timestamps.front()->getContent()->getCounter();
    auto rd = diskread_timestamps.front()->getContent()->getDate();
    diskread_timestamps.front()->getContent()->getEndpoint();
    diskread_timestamps.front()->getContent()->getHostname();
    diskread_timestamps.front()->getContent()->getMount();

    auto wb = diskwrite_timestamps.front()->getContent()->getBytes();
    auto wc = diskwrite_timestamps.front()->getContent()->getCounter();
    auto wd = diskwrite_timestamps.front()->getContent()->getDate();
    diskwrite_timestamps.front()->getContent()->getEndpoint();
    diskwrite_timestamps.front()->getContent()->getHostname();
    diskwrite_timestamps.front()->getContent()->getMount();

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
