
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"


WRENCH_LOG_NEW_DEFAULT_CATEGORY(simulation_timestamp_file_read_test, "Log category for SimulationTimestampFileReadTest");


class SimulationTimestampFileReadTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    

    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_2;
    wrench::WorkflowFile *file_3;
    wrench::WorkflowFile *xl_file;
    wrench::WorkflowFile *too_large_file;


    void do_SimulationTimestampFileReadBasic_test();

protected:
    SimulationTimestampFileReadTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"WMSHost\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk_backup\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/backup\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"ExecutionHost\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk_backup\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/backup\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                          "       <route src=\"ExecutionHost\" dst=\"WMSHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        file_1 = workflow->addFile("file_1", 100.0);
        file_2 = workflow->addFile("file_2", 100.0);
        file_3 = workflow->addFile("file_3", 100.0);

        xl_file = workflow->addFile("xl_file", 1000000000.0);
        too_large_file = workflow->addFile("too_large_file", 10000000000000000000.0);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**            SimulationTimestampFileReadTestBasic                      **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampFileRead class.
 * This test ensures that SimulationTimestampFileReadStart, SimulationTimestampFileReadFailure,
 * and SimulationTimestampFileReadCompletion objects are added to their respective simulation
 * traces at the appropriate times.
 */
class SimulationTimestampFileReadBasicTestWMS : public wrench::WMS {
public:
    SimulationTimestampFileReadBasicTestWMS(SimulationTimestampFileReadTest *test,
                                        const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                        std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                        std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, file_registry_service, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampFileReadTest *test;

    int main() {


        ///StorageService::readFile(file*, location)

        wrench::StorageService::readFile(this->test->file_1, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->xl_file, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->file_2, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->file_3, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->file_3, wrench::FileLocation::LOCATION(this->test->storage_service));

        try {
            wrench::StorageService::readFile(this->test->too_large_file, wrench::FileLocation::LOCATION(this->test->storage_service));
            throw std::runtime_error("file read should have failed");
        } catch(wrench::WorkflowExecutionException &e) {
        }


        wrench::Simulation::sleep(100); ///to allow for xl_file to finish reading.
        /*
         *  This is where it should have tests for reading several files.
         */
        /*
         * expected outcome:
         * file_1 start
         * file_1 end
         * xl_file start
         * file_2 start
         * file_2 end
         * file_3 start
         * file_3 end
         * file_3 start
         * file_3_end
         * too_large_file start
         * too_large_file failure
         * xl_file end
         */

        return 0;

    }
};

TEST_F(SimulationTimestampFileReadTest, SimulationTimestampFileReadBasicTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampFileReadBasic_test);
}

void SimulationTimestampFileReadTest::do_SimulationTimestampFileReadBasic_test(){
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = wrench::Simulation::getHostnameList()[1];
    std::string execution_host = wrench::Simulation::getHostnameList()[0];

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(wms_host,
                                                                                          {std::make_pair(
                                                                                                  execution_host,
                                                                                                  std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                                  wrench::ComputeService::ALL_RAM))},
                                                                                          {})));

    ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(wms_host, {"/"},
            {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "infinity"}})));

    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(execution_host)));


    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampFileReadBasicTestWMS(
            this, {compute_service}, {storage_service}, file_registry_service, wms_host
    )));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));


    //stage files
    std::set<wrench::WorkflowFile *> files_to_stage = {file_1, file_2, file_3, xl_file, too_large_file};

    for (auto const &f  : files_to_stage) {
        ASSERT_NO_THROW(simulation->stageFile(f, storage_service));
    }

    ASSERT_NO_THROW(simulation->launch());

    <--- done to here --->

    int expected_start_timestamps = 6;
    int expected_failure_timestamps = 1;
    int expected_completion_timestamps = 5;

    auto start_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadStart>();
    auto failure_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadFailure>();
    auto completion_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadCompletion>();

    // check number of SimulationTimestampFileRead
    ASSERT_EQ(expected_start_timestamps, start_timestamps.size());
    ASSERT_EQ(expected_failure_timestamps, failure_timestamps.size());
    ASSERT_EQ(expected_completion_timestamps, completion_timestamps.size());

    wrench::SimulationTimestampFileRead *file_1_start = start_timestamps[0]->getContent();
    wrench::SimulationTimestampFileRead *file_1_end = completion_timestamps[0]->getContent();

    wrench::SimulationTimestampFileRead *xl_file_start = start_timestamps[1]->getContent();
    wrench::SimulationTimestampFileRead *xl_file_end = completion_timestamps.back()->getContent();

    wrench::SimulationTimestampFileRead *file_2_start = start_timestamps[2]->getContent();
    wrench::SimulationTimestampFileRead *file_2_end = completion_timestamps[1]->getContent();

    wrench::SimulationTimestampFileRead *file_3_1_start = start_timestamps[3]->getContent();
    wrench::SimulationTimestampFileRead *file_3_1_end = completion_timestamps[2]->getContent();

    wrench::SimulationTimestampFileRead *file_3_2_start = start_timestamps[4]->getContent();
    wrench::SimulationTimestampFileRead *file_3_2_end = completion_timestamps[3]->getContent();

    wrench::SimulationTimestampFileRead *too_large_file_start = start_timestamps[5]->getContent();
    wrench::SimulationTimestampFileRead *too_large_file_end = failure_timestamps.front()->getContent();

    // list of expected matching start and end timestamps
    std::vector<std::pair<wrench::SimulationTimestampFileRead *, wrench::SimulationTimestampFileRead *>> file_read_timestamps = {
            std::make_pair(file_1_start, file_1_end),
            std::make_pair(xl_file_start, xl_file_end),
            std::make_pair(file_2_start, file_2_end),
            std::make_pair(file_3_1_start, file_3_1_end),
            std::make_pair(file_3_2_start, file_3_2_end),
            std::make_pair(too_large_file_start, too_large_file_end)
    };

    wrench::StorageService *service = storage_service.get();


    for (auto &fc : file_read_timestamps) {

        // endpoints should be set correctly
        ASSERT_EQ(fc.first->getEndpoint(), fc.second);
        ASSERT_EQ(fc.second->getEndpoint(), fc.first);

        // completion/failure timestamp times should be greater than start timestamp times
        ASSERT_GT(fc.second->getDate(), fc.first->getDate());

        // source should be set
        ASSERT_EQ(this->storage_service, fc.first->getSource()->getStorageService());
        ASSERT_EQ("/", fc.first->getSource()->getAbsolutePathAtMountPoint());

        ASSERT_EQ(this->storage_service, fc.second->getSource()->getStorageService());
        ASSERT_EQ("/", fc.second->getSource()->getAbsolutePathAtMountPoint());

        //service should be set
        ASSERT_EQ(fc.first->getService(), fc.second->getService());

        // file should be set
        ASSERT_EQ(fc.first->getFile(), fc.second->getFile());
    }



    // test constructors for invalid arguments
    ASSERT_THROW(wrench::SimulationTimestampFileReadStart(
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 service), std::invalid_argument);

    ASSERT_THROW(wrench::SimulationTimestampFileReadStart(
                         this->file_1,
                                 nullptr,
                                 service), std::invalid_argument);

    ASSERT_THROW(wrench::SimulationTimestampFileReadStart(
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 nullptr), std::invalid_argument);


    ASSERT_THROW(wrench::SimulationTimestampFileReadFailure(
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 service), std::invalid_argument);

    ASSERT_THROW(wrench::SimulationTimestampFileReadFailure(
                         this->file_1,
                                 nullptr,
                                 service), std::invalid_argument);

    ASSERT_THROW(wrench::SimulationTimestampFileReadFailure(
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 nullptr), std::invalid_argument);


    ASSERT_THROW(wrench::SimulationTimestampFileReadCompletion(
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 service), std::invalid_argument);

    ASSERT_THROW(wrench::SimulationTimestampFileReadCompletion(
                         this->file_1,
                                 nullptr,
                                 service), std::invalid_argument);

    ASSERT_THROW(wrench::SimulationTimestampFileReadCompletion(
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 nullptr), std::invalid_argument);


    delete simulation;
    free(argv[0]);
    free(argv);
}
