
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

    wrench::WorkflowTask *task1 = nullptr;
    wrench::WorkflowTask *task2 = nullptr;

    wrench::WorkflowTask *failed_task = nullptr;
    wrench::WorkflowFile *small_input_file = nullptr;
    wrench::WorkflowFile *large_input_file = nullptr;

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

        auto job_manager = this->createJobManager();

        ///StorageService::readFile(file*, location)

        wrench::StorageService::readFile(this->test->file_1, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->file_2, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->file_3, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->xl_file, wrench::FileLocation::LOCATION(this->test->storage_service));

        wrench::StorageService::readFile(this->test->too_large_file, wrench::FileLocation::LOCATION(this->test->storage_service));
/*
 *  This is where it should have tests for reading several files.
 */








        this->test->task1 = this->getWorkflow()->addTask("task1", 10.0, 1, 1, 1.0, 0);
        wrench::StandardJob *job1 = job_manager->createStandardJob(this->test->task1, {});
        job_manager->submitJob(job1, this->test->compute_service);
        this->waitForAndProcessNextEvent();

        this->test->task2 = this->getWorkflow()->addTask("task2", 10.0, 1, 1, 1.0, 0);
        wrench::StandardJob *job2 = job_manager->createStandardJob(this->test->task2, {});
        job_manager->submitJob(job2, this->test->compute_service);
        this->waitForAndProcessNextEvent();

        this->test->failed_task = this->getWorkflow()->addTask("failed_task", 100000.0, 1, 1, 1.0, 0);
        this->test->failed_task->addInputFile(this->test->large_input_file);
        this->test->failed_task->addInputFile(this->test->small_input_file);

        wrench::StandardJob *failed_job = job_manager->createStandardJob(
                this->test->failed_task,
                {{this->test->small_input_file, wrench::FileLocation::LOCATION(this->test->storage_service)},
                 {this->test->large_input_file, wrench::FileLocation::LOCATION(this->test->storage_service)}});
        job_manager->submitJob(failed_job, this->test->compute_service);

        wrench::StorageService::deleteFile(this->getWorkflow()->getFileByID("small_input_file"),
                                           wrench::FileLocation::LOCATION(this->test->storage_service),
                                           this->test->file_registry_service);

        std::shared_ptr<wrench::WorkflowExecutionEvent> workflow_execution_event;
        try {
            workflow_execution_event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error getting the execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(workflow_execution_event)) {
            throw std::runtime_error("Job should have failed");
        }

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

    ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(wms_host, {"/"})));

    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampFileReadBasicTestWMS(
            this, {compute_service}, {storage_service}, wms_host
    )));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    file_registry_service = simulation->add(new wrench::FileRegistryService(wms_host));

    small_input_file = this->workflow->addFile("small_input_file", 10);
    large_input_file = this->workflow->addFile("large_input_file", 1000000);

    ASSERT_NO_THROW(simulation->stageFile(large_input_file, storage_service));
    ASSERT_NO_THROW(simulation->stageFile(small_input_file, storage_service));

    ASSERT_NO_THROW(simulation->launch());

    /*
     * expected timeline: task1_start...task1_end...task2_start...task2_end
     * WorkflowTask member variables start_date and end_date should equal SimulationTimestamp<SimulationTimestampFileReadStart>::getContent()->getDate()
     * and SimulationTimestamp<SimulationTimestampFileReadCompletion>::getContent()->getDate() respectively
     */
    auto timestamp_start_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadStart>();
    double task1_start_date = this->task1->getStartDate();
    double task1_start_timestamp = timestamp_start_trace[0]->getContent()->getDate();

    ASSERT_DOUBLE_EQ(task1_start_date, task1_start_timestamp);

    auto timestamp_completion_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadCompletion>();
    double task1_completion_date = this->task1->getEndDate();
    double task1_completion_timestamp = timestamp_completion_trace[0]->getContent()->getDate();

    ASSERT_DOUBLE_EQ(task1_completion_date, task1_completion_timestamp);

    double task2_start_timestamp = timestamp_start_trace[1]->getContent()->getDate();
    double task2_completion_timestamp = timestamp_completion_trace[1]->getContent()->getDate();

    ASSERT_GT(task2_start_timestamp, task1_start_timestamp);
    ASSERT_GT(task2_completion_timestamp, task1_completion_timestamp);

    // expected timeline: task2_end...failed_task_start...failed_task_failed
    auto timestamp_failure_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampFileReadFailure>();
    double failed_task_start_timestamp = timestamp_start_trace[2]->getContent()->getDate();
    double failed_task_end_date = this->failed_task->getEndDate();
    double failed_task_failure_timestamp = timestamp_failure_trace[0]->getContent()->getDate();

    ASSERT_GT(failed_task_start_timestamp, task2_completion_timestamp);
    ASSERT_GT(failed_task_failure_timestamp, failed_task_start_timestamp);

    /*
     * the failed_task should not have its 'end_date' updated
     */
    ASSERT_DOUBLE_EQ(failed_task_end_date, -1.0);

    /*
     * check that there is no completion timestamp for the failed task
     */
    for (auto &ts : timestamp_completion_trace) {
        if (ts->getContent()->getTask()->getID() == "failed_task") {
            throw std::runtime_error("timestamp_completion_trace should not have the 'failed_task'");
        }
    }

    // check that endpoints match up correctly
    wrench::SimulationTimestampTask *task_1_start = timestamp_start_trace[0]->getContent();
    wrench::SimulationTimestampTask *task_1_end = timestamp_completion_trace[0]->getContent();
    ASSERT_EQ(task_1_start->getEndpoint(), task_1_end);
    ASSERT_EQ(task_1_end->getEndpoint(), task_1_start);
    ASSERT_EQ(task_1_start->getTask(), task_1_end->getTask());

    wrench::SimulationTimestampTask *task_2_start = timestamp_start_trace[1]->getContent();
    wrench::SimulationTimestampTask *task_2_end = timestamp_completion_trace[1]->getContent();
    ASSERT_EQ(task_2_start->getEndpoint(), task_2_end);
    ASSERT_EQ(task_2_end->getEndpoint(), task_2_start);
    ASSERT_EQ(task_2_start->getTask(), task_2_end->getTask());

    wrench::SimulationTimestampTask *failed_task_start = timestamp_start_trace[2]->getContent();
    wrench::SimulationTimestampTask *failed_task_failed = timestamp_failure_trace[0]->getContent();
    ASSERT_EQ(failed_task_start->getEndpoint(), failed_task_failed);
    ASSERT_EQ(failed_task_failed->getEndpoint(), failed_task_start);
    ASSERT_EQ(failed_task_start->getTask(), failed_task_failed->getTask());

    // test constructors
    ASSERT_THROW(wrench::SimulationTimestampFileReadStart(nullptr), std::invalid_argument);
    ASSERT_THROW(wrench::SimulationTimestampFileReadFailure(nullptr), std::invalid_argument);
    ASSERT_THROW(wrench::SimulationTimestampFileReadCompletion(nullptr), std::invalid_argument);

    delete simulation;
    free(argv[0]);
    free(argv);
}
