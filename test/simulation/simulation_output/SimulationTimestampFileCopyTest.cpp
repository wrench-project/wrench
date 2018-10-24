#include <iomanip>
#include <algorithm>
#include <map>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

class SimulationTimestampFileCopyTest : public ::testing::Test {

public:
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *source_storage_service = nullptr;
    wrench::StorageService *destination_storage_service = nullptr;

    wrench::WorkflowFile *file_1;
    wrench::WorkflowFile *file_2;
    wrench::WorkflowFile *file_3;
    wrench::WorkflowFile *xl_file;
    wrench::WorkflowFile *too_large_file;

    void do_SimulationTimestampFileCopyBasic_test();

protected:
    SimulationTimestampFileCopyTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"10000us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
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
/**            SimulationTimestampFileCopyTestBasic                  **/
/**********************************************************************/

class SimulationTimestampFileCopyBasicTestWMS : public wrench::WMS {
public:
    SimulationTimestampFileCopyBasicTestWMS(SimulationTimestampFileCopyTest *test,
    const std::set<wrench::ComputeService *> &compute_services,
    const std::set<wrench::StorageService *> &storage_services,
    wrench::FileRegistryService *file_registry_service,
    std::string &hostname) : wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, file_registry_service, hostname, "test") {
        this->test = test;
    }
protected:
    SimulationTimestampFileCopyTest *test;

    int main() {

        auto dmm = this->createDataMovementManager();

        // regular copy with successful completion
        this->test->destination_storage_service->copyFile(this->test->file_1, this->test->source_storage_service);

        dmm->initiateAsynchronousFileCopy(this->test->xl_file, this->test->source_storage_service, this->test->destination_storage_service);
        dmm->doSynchronousFileCopy(this->test->file_2, this->test->source_storage_service, this->test->destination_storage_service);

        this->test->destination_storage_service->copyFile(this->test->file_3, this->test->source_storage_service);
        this->test->destination_storage_service->copyFile(this->test->file_3, this->test->source_storage_service);

        bool failed = false;

        // this should fail and a SimulationTimestampFileCopyFailure should be created
        try {
            this->test->destination_storage_service->copyFile(this->test->too_large_file, this->test->source_storage_service);
        } catch(wrench::WorkflowExecutionException &e) {
            failed = true;
        }
        if (not failed) {
            throw std::runtime_error("file copy should have failed");
        }

        // wait for xl_file file copy to complete
        simulation->sleep(100);

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
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("simulation_timestamp_file_copy_basic_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string host1 = simulation->getHostnameList()[0];
    std::string host2 = simulation->getHostnameList()[1];

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::MultihostMulticoreComputeService(host1,
                                                                                                   {std::make_tuple(
                                                                                                           host1,
                                                                                                           wrench::ComputeService::ALL_CORES,
                                                                                                           wrench::ComputeService::ALL_RAM)},
                                                                                                   {})));

    ASSERT_NO_THROW(source_storage_service = simulation->add(new wrench::SimpleStorageService(host1, 100000000000000000000.0)));
    ASSERT_NO_THROW(destination_storage_service = simulation->add(new wrench::SimpleStorageService(host2, 10000000000)));

    wrench::FileRegistryService *file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(host1)));

    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampFileCopyBasicTestWMS(
            this, {compute_service}, {source_storage_service, destination_storage_service, }, file_registry_service, host1
    )));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    //stage files
    std::map<std::string, wrench::WorkflowFile *> files_to_stage = {std::make_pair(file_1->getID(), file_1),
                                                                  std::make_pair(file_2->getID(), file_2),
                                                                  std::make_pair(file_3->getID(), file_3),
                                                                  std::make_pair(xl_file->getID(), xl_file),
                                                                  std::make_pair(too_large_file->getID(), too_large_file)};

    ASSERT_NO_THROW(simulation->stageFiles(files_to_stage, source_storage_service));

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
        ASSERT_EQ(this->source_storage_service, fc.first->getSource().storage_service);
        ASSERT_EQ("/", fc.first->getSource().partition);
        ASSERT_EQ(this->destination_storage_service, fc.first->getDestination().storage_service);
        ASSERT_EQ("/", fc.first->getDestination().partition);

        ASSERT_EQ(this->source_storage_service, fc.second->getSource().storage_service);
        ASSERT_EQ("/", fc.second->getSource().partition);
        ASSERT_EQ(this->destination_storage_service, fc.second->getDestination().storage_service);
        ASSERT_EQ("/", fc.second->getDestination().partition);

        // file should be set
        ASSERT_EQ(fc.first->getFile(), fc.second->getFile());
    }

    // test constructors for invalid arguments
    ASSERT_THROW(wrench::SimulationTimestampFileCopyStart(nullptr, this->source_storage_service, "/", this->destination_storage_service, "/"), std::invalid_argument);
    ASSERT_THROW(wrench::SimulationTimestampFileCopyStart(this->file_1, nullptr, "/", this->destination_storage_service, "/"), std::invalid_argument);
    ASSERT_THROW(wrench::SimulationTimestampFileCopyStart(this->file_1, this->source_storage_service, "", this->destination_storage_service, "/"), std::invalid_argument);
    ASSERT_THROW(wrench::SimulationTimestampFileCopyStart(this->file_1, this->source_storage_service, "", nullptr, "/"), std::invalid_argument);
    ASSERT_THROW(wrench::SimulationTimestampFileCopyStart(this->file_1, this->source_storage_service, "", this->destination_storage_service, ""), std::invalid_argument);

    ASSERT_THROW(wrench::SimulationTimestampFileCopyFailure(nullptr), std::invalid_argument);
    ASSERT_THROW(wrench::SimulationTimestampFileCopyCompletion(nullptr), std::invalid_argument);

    delete simulation;
    free(argv[0]);
    free(argv);
}
