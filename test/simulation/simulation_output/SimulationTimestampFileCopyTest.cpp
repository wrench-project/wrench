#include <iomanip>
#include <algorithm>
#include <map>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(simulation_timestamp_file_copy_test, "Log category for SimulationTimestampFileCopyTest");


class SimulationTimestampFileCopyTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> source_storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> destination_storage_service = nullptr;

    std::shared_ptr<wrench::DataFile> file_1;
    std::shared_ptr<wrench::DataFile> file_2;
    std::shared_ptr<wrench::DataFile> file_3;
    std::shared_ptr<wrench::DataFile> xl_file;
    std::shared_ptr<wrench::DataFile> too_large_file;

    void do_SimulationTimestampFileCopyBasic_test();

protected:

    ~SimulationTimestampFileCopyTest() {
        workflow->clear();
    }

    SimulationTimestampFileCopyTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000000000B\"/>"
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

        file_1 = workflow->addFile("file_1", 100.0);
        file_2 = workflow->addFile("file_2", 100.0);
        file_3 = workflow->addFile("file_3", 100.0);

        xl_file = workflow->addFile("xl_file", 1000000000.0);
        too_large_file = workflow->addFile("too_large_file", 10000000000000000000.0);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;
};

/**********************************************************************/
/**            SimulationTimestampFileCopyTestBasic                  **/
/**********************************************************************/

class SimulationTimestampFileCopyBasicTestWMS : public wrench::ExecutionController {
public:
    SimulationTimestampFileCopyBasicTestWMS(SimulationTimestampFileCopyTest *test,
                                            std::string &hostname):
                                            wrench::ExecutionController(hostname, "test"), test(test) {
    }
protected:
    SimulationTimestampFileCopyTest *test;

    int main() {

        auto dmm = this->createDataMovementManager();

        // regular copy with successful completion
        dmm->doSynchronousFileCopy(this->test->file_1,
                                   wrench::FileLocation::LOCATION(this->test->source_storage_service),
                                   wrench::FileLocation::LOCATION(this->test->destination_storage_service));

        dmm->initiateAsynchronousFileCopy(this->test->xl_file,
                                          wrench::FileLocation::LOCATION(this->test->source_storage_service),
                                          wrench::FileLocation::LOCATION(this->test->destination_storage_service));

        dmm->doSynchronousFileCopy(this->test->file_2,
                                   wrench::FileLocation::LOCATION(this->test->source_storage_service),
                                   wrench::FileLocation::LOCATION(this->test->destination_storage_service));

        dmm->doSynchronousFileCopy(this->test->file_3,
                                   wrench::FileLocation::LOCATION(this->test->source_storage_service),
                                   wrench::FileLocation::LOCATION(this->test->destination_storage_service));

        dmm->doSynchronousFileCopy(this->test->file_3,
                                   wrench::FileLocation::LOCATION(this->test->source_storage_service),
                                   wrench::FileLocation::LOCATION(this->test->destination_storage_service));

        // this should fail and a SimulationTimestampFileCopyFailure should be created
        try {
            dmm->doSynchronousFileCopy(this->test->too_large_file,
                                       wrench::FileLocation::LOCATION(this->test->source_storage_service),
                                       wrench::FileLocation::LOCATION(this->test->destination_storage_service));

            throw std::runtime_error("file copy should have failed");
        } catch(wrench::ExecutionException &e) {
        }

        // wait for xl_file file copy to complete
        wrench::Simulation::sleep(100);

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

TEST_F(SimulationTimestampFileCopyTest, SimulationTimestampFileCopyBasicTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampFileCopyBasic_test);
}

void SimulationTimestampFileCopyTest::do_SimulationTimestampFileCopyBasic_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string host1 = "Host1";
    std::string host2 = "Host2";

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(host1,
                                                                                          {std::make_pair(
                                                                                                  host1,
                                                                                                  std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                                  wrench::ComputeService::ALL_RAM))},
                                                                                          {})));

    ASSERT_NO_THROW(source_storage_service = simulation->add(new wrench::SimpleStorageService(host1, {"/"},
                                                                                              {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "infinity"}})));
    ASSERT_NO_THROW(destination_storage_service = simulation->add(new wrench::SimpleStorageService(host2, {"/"},
                                                                                                   {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "infinity"}})));

    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(host1)));

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampFileCopyBasicTestWMS(
            this, host1)));

    //stage files
    std::set<std::shared_ptr<wrench::DataFile> > files_to_stage = {file_1, file_2, file_3, xl_file, too_large_file};

    for (auto const &f  : files_to_stage) {
        ASSERT_NO_THROW(simulation->stageFile(f, source_storage_service));
    }

    ASSERT_NO_THROW(simulation->launch());

    int expected_start_timestamps = 6;
    int expected_failure_timestamps = 1;
    int expected_completion_timestamps = 5;

    auto start_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileCopyStart>();
    auto failure_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileCopyFailure>();
    auto completion_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileCopyCompletion>();

    // check number of SimulationTimestampFileCopy
    ASSERT_EQ(expected_start_timestamps, start_timestamps.size());
    ASSERT_EQ(expected_failure_timestamps, failure_timestamps.size());
    ASSERT_EQ(expected_completion_timestamps, completion_timestamps.size());

    wrench::SimulationTimestampFileCopy *file_1_start = start_timestamps[0]->getContent();
    wrench::SimulationTimestampFileCopy *file_1_end = completion_timestamps[0]->getContent();

    wrench::SimulationTimestampFileCopy *xl_file_start = start_timestamps[1]->getContent();
    wrench::SimulationTimestampFileCopy *xl_file_end = completion_timestamps.back()->getContent();

    wrench::SimulationTimestampFileCopy *file_2_start = start_timestamps[2]->getContent();
    wrench::SimulationTimestampFileCopy *file_2_end = completion_timestamps[1]->getContent();

    wrench::SimulationTimestampFileCopy *file_3_1_start = start_timestamps[3]->getContent();
    wrench::SimulationTimestampFileCopy *file_3_1_end = completion_timestamps[2]->getContent();

    wrench::SimulationTimestampFileCopy *file_3_2_start = start_timestamps[4]->getContent();
    wrench::SimulationTimestampFileCopy *file_3_2_end = completion_timestamps[3]->getContent();

    wrench::SimulationTimestampFileCopy *too_large_file_start = start_timestamps[5]->getContent();
    wrench::SimulationTimestampFileCopy *too_large_file_end = failure_timestamps.front()->getContent();

    // list of expected matching start and end timestamps
    std::vector<std::pair<wrench::SimulationTimestampFileCopy *, wrench::SimulationTimestampFileCopy *>> file_copy_timestamps = {
            std::make_pair(file_1_start, file_1_end),
            std::make_pair(xl_file_start, xl_file_end),
            std::make_pair(file_2_start, file_2_end),
            std::make_pair(file_3_1_start, file_3_1_end),
            std::make_pair(file_3_2_start, file_3_2_end),
            std::make_pair(too_large_file_start, too_large_file_end)
    };

    for (auto &fc : file_copy_timestamps) {

        // endpoints should be set correctly
        ASSERT_EQ(fc.first->getEndpoint(), fc.second);
        ASSERT_EQ(fc.second->getEndpoint(), fc.first);

        // completion/failure timestamp times should be greater than start timestamp times
        ASSERT_GT(fc.second->getDate(), fc.first->getDate());

        // source and destinations should be set
        ASSERT_EQ(this->source_storage_service, fc.first->getSource()->getStorageService());
        ASSERT_EQ("/", fc.first->getSource()->getAbsolutePathAtMountPoint());
        ASSERT_EQ(this->destination_storage_service, fc.first->getDestination()->getStorageService());
        ASSERT_EQ("/", fc.first->getDestination()->getAbsolutePathAtMountPoint());

        ASSERT_EQ(this->source_storage_service, fc.second->getSource()->getStorageService());
        ASSERT_EQ("/", fc.second->getSource()->getAbsolutePathAtMountPoint());
        ASSERT_EQ(this->destination_storage_service, fc.second->getDestination()->getStorageService());
        ASSERT_EQ("/", fc.second->getDestination()->getAbsolutePathAtMountPoint());

        // file should be set
        ASSERT_EQ(fc.first->getFile(), fc.second->getFile());
    }

    // test constructors for invalid arguments
    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyStart(0.0,
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->source_storage_service),
                                 wrench::FileLocation::LOCATION(this->destination_storage_service)), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyStart(0.0,
                         this->file_1,
                                 nullptr,
                                 wrench::FileLocation::LOCATION(this->destination_storage_service, "/")), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyStart(0.0,
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->source_storage_service),
                                 nullptr), std::invalid_argument);



    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyFailure(0.0,
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->source_storage_service),
                                 wrench::FileLocation::LOCATION(this->destination_storage_service)), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyFailure(0.0,
                         this->file_1,
                                 nullptr,
                                 wrench::FileLocation::LOCATION(this->destination_storage_service, "/")), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyFailure(0.0,
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->source_storage_service),
                                 nullptr), std::invalid_argument);



    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyCompletion(0.0,
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->source_storage_service),
                                 wrench::FileLocation::LOCATION(this->destination_storage_service)), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyCompletion(0.0,
                         this->file_1,
                                 nullptr,
                                 wrench::FileLocation::LOCATION(this->destination_storage_service, "/")), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileCopyCompletion(0.0,
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->source_storage_service),
                                 nullptr), std::invalid_argument);

    for (int i=0; i < argc; i++)
     free(argv[i]);
    free(argv);
}
