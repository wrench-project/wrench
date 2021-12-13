
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"


WRENCH_LOG_CATEGORY(simulation_timestamp_file_write_test, "Log category for SimulationTimestampFileWriteTest");


class SimulationTimestampFileWriteTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;


    std::shared_ptr<wrench::DataFile> file_1;
    std::shared_ptr<wrench::DataFile> file_2;
    std::shared_ptr<wrench::DataFile> file_3;
    std::shared_ptr<wrench::DataFile> xl_file;

    std::shared_ptr<wrench::WorkflowTask> task = nullptr;


    void do_SimulationTimestampFileWriteBasic_test();

protected:
    ~SimulationTimestampFileWriteTest() {
        workflow->clear();
    }

    SimulationTimestampFileWriteTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
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

        file_1 = workflow->addFile("file_1", 100.0);
        file_2 = workflow->addFile("file_2", 100.0);
        file_3 = workflow->addFile("file_3", 100.0);

        xl_file = workflow->addFile("xl_file", 1000000000.0);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::shared_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**            SimulationTimestampFileWriteTestBasic                      **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampFileWrite class.
 * This test ensures that SimulationTimestampFileWriteStart, SimulationTimestampFileWriteFailure,
 * and SimulationTimestampFileWriteCompletion objects are added to their respective simulation
 * traces at the appropriate times.
 */
class SimulationTimestampFileWriteBasicTestWMS : public wrench::WMS {
public:
    SimulationTimestampFileWriteBasicTestWMS(SimulationTimestampFileWriteTest *test,
                                             std::shared_ptr<wrench::Workflow> workflow,
                                             const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                             const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                             std::shared_ptr<wrench::FileRegistryService> file_registry_service,
                                             std::string &hostname) :
            wrench::WMS(workflow, nullptr, nullptr, compute_services, storage_services, {}, file_registry_service, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampFileWriteTest *test;

    int main() {


        auto job_manager = this->createJobManager();

        this->test->task = this->getWorkflow()->addTask("task1", 10.0, 1, 1, 0);
        this->test->task->addOutputFile(this->test->file_1);
        this->test->task->addOutputFile(this->test->file_2);
        this->test->task->addOutputFile(this->test->file_3);
        this->test->task->addOutputFile(this->test->xl_file);
        auto job1 = job_manager->createStandardJob(this->test->task,
                                                                   {{this->test->file_1, wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                    {this->test->file_2, wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                    {this->test->file_3, wrench::FileLocation::LOCATION(this->test->storage_service)},
                                                                    {this->test->xl_file, wrench::FileLocation::LOCATION(this->test->storage_service)}});
        job_manager->submitJob(job1, this->test->compute_service);

        this->waitForAndProcessNextEvent();

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
         * xl_file end
         */

        return 0;

    }
};

TEST_F(SimulationTimestampFileWriteTest, SimulationTimestampFileWriteBasicTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampFileWriteBasic_test);
}

void SimulationTimestampFileWriteTest::do_SimulationTimestampFileWriteBasic_test(){
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string host1 = "Host1";


    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(host1,
                                                                                          {std::make_pair(
                                                                                                  host1,
                                                                                                  std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                                  wrench::ComputeService::ALL_RAM))},
                                                                                          {})));

    ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(host1, {"/"},
                                                                                       {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "infinity"}})));

    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;
    ASSERT_NO_THROW(file_registry_service = simulation->add(new wrench::FileRegistryService(host1)));


    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampFileWriteBasicTestWMS(
            this, workflow, {compute_service}, {storage_service}, file_registry_service, host1
    )));

    //stage files
    std::set<std::shared_ptr<wrench::DataFile> > files_to_stage = {file_1, file_2, file_3, xl_file};

    for (auto const &f  : files_to_stage) {
        ASSERT_NO_THROW(simulation->stageFile(f, storage_service));
    }

    ASSERT_NO_THROW(simulation->launch());


    int expected_start_timestamps = 4;
    int expected_failure_timestamps = 0;
    int expected_completion_timestamps = 4;

    auto start_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileWriteStart>();
    auto failure_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileWriteFailure>();
    auto completion_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampFileWriteCompletion>();

    // check number of SimulationTimestampFileWrite
    ASSERT_EQ(expected_start_timestamps, start_timestamps.size());
    ASSERT_EQ(expected_failure_timestamps, failure_timestamps.size());
    ASSERT_EQ(expected_completion_timestamps, completion_timestamps.size());

    wrench::SimulationTimestampFileWrite *file_1_start = start_timestamps[0]->getContent();
    wrench::SimulationTimestampFileWrite *file_1_end = completion_timestamps[0]->getContent();

    wrench::SimulationTimestampFileWrite *file_2_start = start_timestamps[1]->getContent();
    wrench::SimulationTimestampFileWrite *file_2_end = completion_timestamps[1]->getContent();

    wrench::SimulationTimestampFileWrite *file_3_1_start = start_timestamps[2]->getContent();
    wrench::SimulationTimestampFileWrite *file_3_1_end = completion_timestamps[2]->getContent();

    wrench::SimulationTimestampFileWrite *xl_file_start = start_timestamps[3]->getContent();
    wrench::SimulationTimestampFileWrite *xl_file_end = completion_timestamps[3]->getContent();


    // list of expected matching start and end timestamps
    std::vector<std::pair<wrench::SimulationTimestampFileWrite *, wrench::SimulationTimestampFileWrite *>> file_write_timestamps = {
            std::make_pair(file_1_start, file_1_end),
            std::make_pair(file_2_start, file_2_end),
            std::make_pair(file_3_1_start, file_3_1_end),
            std::make_pair(xl_file_start, xl_file_end),
    };

    wrench::StorageService *service = storage_service.get();


    for (auto &fc : file_write_timestamps) {

        // endpoints should be set correctly
        ASSERT_EQ(fc.first->getEndpoint(), fc.second);
        ASSERT_EQ(fc.second->getEndpoint(), fc.first);

        // completion/failure timestamp times should be greater than start timestamp times
        ASSERT_GT(fc.second->getDate(), fc.first->getDate());

        // destination should be set
        ASSERT_EQ(this->storage_service, fc.first->getDestination()->getStorageService());
        ASSERT_EQ("/", fc.first->getDestination()->getAbsolutePathAtMountPoint());

        ASSERT_EQ(this->storage_service, fc.second->getDestination()->getStorageService());
        ASSERT_EQ("/", fc.second->getDestination()->getAbsolutePathAtMountPoint());

        //service should be set
        ASSERT_EQ(fc.first->getService(), fc.second->getService());

        // file should be set
        ASSERT_EQ(fc.first->getFile(), fc.second->getFile());

        //task1 should be set
        ASSERT_EQ(fc.first->getTask(), fc.second->getTask());
    }


    // test constructors for invalid arguments
    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteStart(0.0,
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 service,
                                 task), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteStart(0.0,
                         this->file_1,
                                 nullptr,
                                 service,
                                 task), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteStart(0.0,
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 nullptr,
                                 task), std::invalid_argument);



    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteFailure(0.0,
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 service,
                                 task), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteFailure(0.0,
                         this->file_1,
                                 nullptr,
                                 service,
                                 task), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteFailure(0.0,
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 nullptr,
                                 task), std::invalid_argument);




    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteCompletion(0.0,
                         nullptr,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 service,
                                 task), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteCompletion(0.0,
                         this->file_1,
                                 nullptr,
                                 service,
                                 task), std::invalid_argument);

    ASSERT_THROW(simulation->getOutput().addTimestampFileWriteCompletion(0.0,
                         this->file_1,
                                 wrench::FileLocation::LOCATION(this->storage_service).get(),
                                 nullptr,
                                 task), std::invalid_argument);



    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
