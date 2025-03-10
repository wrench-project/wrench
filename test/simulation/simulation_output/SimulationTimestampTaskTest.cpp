
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#define EPSILON 0.00001

class SimulationTimestampTaskTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service = nullptr;
    std::shared_ptr<wrench::StorageService> backup_storage_service = nullptr;
    std::shared_ptr<wrench::FileRegistryService> file_registry_service = nullptr;

    std::shared_ptr<wrench::WorkflowTask> task1 = nullptr;
    std::shared_ptr<wrench::WorkflowTask> task2 = nullptr;

    std::shared_ptr<wrench::WorkflowTask> failed_task = nullptr;
    std::shared_ptr<wrench::DataFile> small_input_file = nullptr;
    std::shared_ptr<wrench::DataFile> large_input_file = nullptr;

    std::shared_ptr<wrench::Workflow> workflow;


    void do_SimulationTimestampTaskBasic_test();
    void do_SimulationTimestampTaskMultiple_test();
    void do_SimulationTimestampTaskTerminateAndFail_test();


protected:
    ~SimulationTimestampTaskTest() {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    SimulationTimestampTaskTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"WMSHost\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk_backup\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/backup\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"ExecutionHost\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk_backup\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/backup\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"ComputeHost\" speed=\"1f\" core=\"1\" > "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"large_disk_backup\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/backup\"/>"
                          "          </disk>"
                          "          <disk id=\"other_large_disk\" read_bw=\"100MBps\" write_bw=\"100MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"1Gbps\" latency=\"1us\"/>"
                          "       <route src=\"ExecutionHost\" dst=\"WMSHost\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"ComputeHost\" dst=\"WMSHost\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"ComputeHost\" dst=\"ExecutionHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

        workflow = wrench::Workflow::createWorkflow();
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};

/**********************************************************************/
/**            SimulationTimestampTaskTestBasic                      **/
/**********************************************************************/

/*
 * Testing the basic functionality of the SimulationTimestampTask class.
 * This test ensures that SimulationTimestampTaskStart, SimulationTimestampTaskFailure,
 * and SimulationTimestampTaskCompletion objects are added to their respective simulation
 * traces at the appropriate times.
 */
class SimulationTimestampTaskBasicTestWMS : public wrench::ExecutionController {
public:
    SimulationTimestampTaskBasicTestWMS(SimulationTimestampTaskTest *test,
                                        std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampTaskTest *test;

    int main() override {

        auto job_manager = this->createJobManager();

        this->test->task1 = this->test->workflow->addTask("task1", 10.0, 1, 1, 0);
        auto job1 = job_manager->createStandardJob(this->test->task1);
        job_manager->submitJob(job1, this->test->compute_service);
        this->waitForAndProcessNextEvent();

        this->test->task2 = this->test->workflow->addTask("task2", 10.0, 1, 1, 0);
        auto job2 = job_manager->createStandardJob(this->test->task2);
        job_manager->submitJob(job2, this->test->compute_service);
        this->waitForAndProcessNextEvent();

        this->test->failed_task = this->test->workflow->addTask("failed_task", 100000.0, 1, 1, 0);
        this->test->failed_task->addInputFile(this->test->large_input_file);
        this->test->failed_task->addInputFile(this->test->small_input_file);

        auto failed_job = job_manager->createStandardJob(
                this->test->failed_task,
                {{this->test->small_input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->small_input_file)},
                 {this->test->large_input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->large_input_file)}});

        job_manager->submitJob(failed_job, this->test->compute_service);

        wrench::StorageService::deleteFileAtLocation(
                wrench::FileLocation::LOCATION(this->test->storage_service, wrench::Simulation::getFileByID("small_input_file")));


        std::shared_ptr<wrench::ExecutionEvent> workflow_execution_event;
        try {
            workflow_execution_event = this->waitForNextEvent();
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error("Error getting the execution event: " + e.getCause()->toString());
        }

        if (not std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(workflow_execution_event)) {
            throw std::runtime_error("Job should have failed");
        }

        return 0;
    }
};

TEST_F(SimulationTimestampTaskTest, SimulationTimestampTaskBasicTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampTaskBasic_test);
}

void SimulationTimestampTaskTest::do_SimulationTimestampTaskBasic_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-default-control-message-size=1024");

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

    ASSERT_NO_THROW(storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(wms_host, {"/"})));

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampTaskBasicTestWMS(
                            this, wms_host)));

    file_registry_service = simulation->add(new wrench::FileRegistryService(wms_host));

    small_input_file = wrench::Simulation::addFile("small_input_file", 10);
    large_input_file = wrench::Simulation::addFile("large_input_file", 1000000);

    ASSERT_NO_THROW(storage_service->createFile(large_input_file));
    ASSERT_NO_THROW(storage_service->createFile(small_input_file));

    simulation->getOutput().enableWorkflowTaskTimestamps(true);

    ASSERT_NO_THROW(simulation->launch());

    /*
     * expected timeline: task1_start...task1_end...task2_start...task2_end
     * WorkflowTask member variables start_date and end_date should equal SimulationTimestamp<SimulationTimestampTaskStart>::getContent()->getDate()
     * and SimulationTimestamp<SimulationTimestampTaskCompletion>::getContent()->getDate() respectively
     */

    auto timestamp_start_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskStart>();
    auto timestamp_completion_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    double task1_start_date = this->task1->getStartDate();
    double task1_start_timestamp = timestamp_start_trace[0]->getContent()->getDate();

    ASSERT_DOUBLE_EQ(task1_start_date, task1_start_timestamp);

    double task1_completion_date = this->task1->getEndDate();
    double task1_completion_timestamp = timestamp_completion_trace[0]->getContent()->getDate();

    ASSERT_LT(std::abs(task1_completion_date - task1_completion_timestamp), 0.001);

    double task2_start_timestamp = timestamp_start_trace[1]->getContent()->getDate();
    double task2_completion_timestamp = timestamp_completion_trace[1]->getContent()->getDate();

    ASSERT_GT(task2_start_timestamp, task1_start_timestamp);
    ASSERT_GT(task2_completion_timestamp, task1_completion_timestamp);

    // expected timeline: task2_end...failed_task_start...failed_task_failed
    auto timestamp_failure_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskFailure>();
    //    for (auto const &ts : timestamp_start_trace) {
    //        std::cerr << "  - " << ts->getContent()->getTask()->getID() << ": " << ts->getDate() << "\n";
    //    }
    double failed_task_start_timestamp = timestamp_start_trace[2]->getContent()->getDate();
    double failed_task_end_date = this->failed_task->getEndDate();
    double failed_task_failure_timestamp = timestamp_failure_trace[0]->getContent()->getDate();

    //    ASSERT_GT(failed_task_start_timestamp, task2_completion_timestamp);
    ASSERT_GT(failed_task_failure_timestamp, failed_task_start_timestamp);

    /*
     * the failed_task should not have its 'end_date' updated
     */
    ASSERT_DOUBLE_EQ(failed_task_end_date, -1.0);

    /*
     * check that there is no completion timestamp for the failed task1
     */
    for (auto &ts: timestamp_completion_trace) {
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

#ifdef WRENCH_INTERNAL_EXCEPIONS
    ASSERT_THROW(simulation->getOutput().addTimestampTaskStart(0.0, nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->getOutput().addTimestampTaskFailure(0.0, nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->getOutput().addTimestampTaskCompletion(0.0, nullptr), std::invalid_argument);
    ASSERT_THROW(simulation->getOutput().addTimestampTaskTermination(0.0, nullptr), std::invalid_argument);
#endif

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**            SimulationTimestampTaskTestMultiple                   **/
/**********************************************************************/

/*
 * Testing that multiple timestamps of the same task1 are created for each time
 * that the task1 is run in a job.
 */

class SimulationTimestampTaskMultipleTestWMS : public wrench::ExecutionController {
public:
    SimulationTimestampTaskMultipleTestWMS(SimulationTimestampTaskTest *test,
                                           std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampTaskTest *test;

    int main() override {

        auto job_manager = this->createJobManager();

        this->test->task1 = this->test->workflow->addTask("task1", 10.0, 1, 1, 0);

        int num_task1_runs = 2;

#if 1
        for (int i = 0; i < num_task1_runs; ++i) {
            this->test->task1->setInternalState(wrench::WorkflowTask::InternalState::TASK_READY);
            this->test->task1->setState(wrench::WorkflowTask::State::READY);

            auto job1 = job_manager->createStandardJob(this->test->task1);
            job_manager->submitJob(job1, this->test->compute_service);
            this->waitForAndProcessNextEvent();
        }

#endif
        this->test->failed_task = this->test->workflow->addTask("failed_task", 10, 1, 1, 0);
        this->test->failed_task->addInputFile(this->test->large_input_file);
        this->test->failed_task->addInputFile(this->test->small_input_file);

#if 1
        auto failed_job = job_manager->createStandardJob(
                this->test->failed_task,
                {{this->test->small_input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->small_input_file)},
                 {this->test->large_input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->large_input_file)}});

        job_manager->submitJob(failed_job, this->test->compute_service);
        wrench::StorageService::deleteFileAtLocation(
                wrench::FileLocation::LOCATION(this->test->storage_service, wrench::Simulation::getFileByID("small_input_file")));

        this->waitForAndProcessNextEvent();
#endif

        auto passing_job = job_manager->createStandardJob(
                this->test->failed_task,
                {{this->test->small_input_file, wrench::FileLocation::LOCATION(this->test->backup_storage_service, this->test->small_input_file)},
                 {this->test->large_input_file, wrench::FileLocation::LOCATION(this->test->storage_service, this->test->large_input_file)}});
        job_manager->submitJob(passing_job, this->test->compute_service);
        this->waitForAndProcessNextEvent();

        return 0;
    }
};

TEST_F(SimulationTimestampTaskTest, SimulationTimestampTaskMultipleTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampTaskMultiple_test);
}

void SimulationTimestampTaskTest::do_SimulationTimestampTaskMultiple_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-default-control-message-size=1024");
//        argv[2] = strdup("--wrench-full-log");


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

    ASSERT_NO_THROW(storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(wms_host, {"/"})));
    ASSERT_NO_THROW(backup_storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(wms_host, {"/backup"})));


    std::shared_ptr<wrench::ExecutionController> wms;

    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampTaskMultipleTestWMS(
                            this, wms_host)));

    file_registry_service = simulation->add(new wrench::FileRegistryService(wms_host));

    small_input_file = wrench::Simulation::addFile("small_input_file", 10);
    large_input_file = wrench::Simulation::addFile("large_input_file", 1000000);

    ASSERT_NO_THROW(storage_service->createFile(large_input_file));
    ASSERT_NO_THROW(storage_service->createFile(small_input_file));
    ASSERT_NO_THROW(backup_storage_service->createFile(small_input_file));

    simulation->getOutput().enableWorkflowTaskTimestamps(true);

    ASSERT_NO_THROW(simulation->launch());

    auto starts_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskStart>();
    auto completions_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    auto failures_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskFailure>();

    /*
     * Check that we have the right number of SimulationTimestampTaskXXXX
     *
     * Ran 4 single task1 jobs:
     *  - 4 started: task1 x 2, failed_task x 2
     *  - 3 completed: task1 x 2, failed_task x 1
     *  - 1 failed : failed_task x 1
     */
    ASSERT_EQ(starts_trace.size(), 4);
    ASSERT_EQ(completions_trace.size(), 3);
    ASSERT_EQ(failures_trace.size(), 1);

    std::string task1_id = "task1";
    std::string failed_task_id = "failed_task";

    int num_task1_starts = std::count_if(starts_trace.begin(), starts_trace.end(),
                                         [&task1_id](wrench::SimulationTimestamp<wrench::SimulationTimestampTaskStart> *ts) {
                                             return ((*ts).getContent()->getTask()->getID() == task1_id);
                                         });

    int num_failed_task_starts = std::count_if(starts_trace.begin(), starts_trace.end(),
                                               [&failed_task_id](wrench::SimulationTimestamp<wrench::SimulationTimestampTaskStart> *ts) {
                                                   return ((*ts).getContent()->getTask()->getID() == failed_task_id);
                                               });

    int num_task1_completions = std::count_if(completions_trace.begin(), completions_trace.end(),
                                              [&task1_id](wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *ts) {
                                                  return ((*ts).getContent()->getTask()->getID() == task1_id);
                                              });

    int num_failed_task_failures = std::count_if(failures_trace.begin(), failures_trace.end(),
                                                 [&failed_task_id](wrench::SimulationTimestamp<wrench::SimulationTimestampTaskFailure> *ts) {
                                                     return ((*ts).getContent()->getTask()->getID() == failed_task_id);
                                                 });

    int num_failed_task_completions = std::count_if(completions_trace.begin(), completions_trace.end(),
                                                    [&failed_task_id](wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion> *ts) {
                                                        return ((*ts).getContent()->getTask()->getID() == failed_task_id);
                                                    });

    ASSERT_EQ(num_task1_starts, 2);
    ASSERT_EQ(num_failed_task_starts, 2);
    ASSERT_EQ(num_task1_completions, 2);
    ASSERT_EQ(num_failed_task_failures, 1);
    ASSERT_EQ(num_failed_task_completions, 1);

    /*
     * EXPECTED TIMELINE:
     *  task1 start
     *      about 10 seconds passes
     *  task1 end
     *  task1 start again
     *      about 10 seconds passes
     *  task1 end again
     *  failed_task start
     *      large file being read from storage service
     *          small file gets deleted in the mean time
     *      large file read completes
     *      attempt to read small file fails
     *  failed_task fails
     *  failed_task start again
     *      large successfully read from storage service
     *      small file successfully read from backup storage service
     *      task1 computation runs
     *  failed_task completes
     */

    double task1_start_date_1 = starts_trace[0]->getDate();
    double task1_completion_date_1 = completions_trace[0]->getDate();

    //    ASSERT_DOUBLE_EQ(std::floor(task1_completion_date_1 - task1_start_date_1), 10.0);
    ASSERT_TRUE(std::abs<double>((task1_completion_date_1 - task1_start_date_1) - 10.0) < EPSILON);

    double task1_start_date_2 = starts_trace[1]->getDate();
    double task1_completion_date_2 = completions_trace[1]->getDate();

    //    std::cerr << task1_completion_date_2 << "\n";
    //    std::cerr << task1_start_date_2 << "\n";
    //    std::cerr << std::floor(task1_completion_date_2 - task1_start_date_2) << "\n";
    //    ASSERT_DOUBLE_EQ(std::floor(task1_completion_date_2 - task1_start_date_2), 10.0);
    ASSERT_TRUE(std::abs<double>((task1_completion_date_2 - task1_start_date_2) - 10.0) < EPSILON);

    double failed_task_start_date_1 = starts_trace[2]->getDate();
    double failed_task_failure_date_1 = failures_trace[0]->getDate();

    ASSERT_GT(failed_task_failure_date_1, failed_task_start_date_1);

    double failed_task_start_date_2 = starts_trace[3]->getDate();
    double failed_task_completion_date_1 = completions_trace[2]->getDate();

    ASSERT_GT(failed_task_completion_date_1, failed_task_start_date_2);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**            SimulationTimestampTaskTestTerminateAndFail           **/
/**********************************************************************/

/*
 * Testing that SimulationTimestampTaskFailure timestamps get created when tasks fail
 * due to a compute service being shut down while containing standard jobs with
 * running tasks.
 *
 * Also testing that SimulationTimestampTaskTerminated timestamps get created when
 * tasks are deliberately cleanly_terminated by the WMS.
 */

class SimulationTimestampTaskTerminateAndFailTestWMS : public wrench::ExecutionController {
public:
    SimulationTimestampTaskTerminateAndFailTestWMS(SimulationTimestampTaskTest *test,
                                                   std::string &hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampTaskTest *test;

    int main() override {

        auto job_manager = this->createJobManager();

        this->test->task1 = this->test->workflow->addTask("terminated_task", 1000.0, 1, 1, 0);
        auto job_that_will_be_terminated = job_manager->createStandardJob(this->test->task1);
        job_manager->submitJob(job_that_will_be_terminated, this->test->compute_service);
        wrench::S4U_Simulation::sleep(10.0);
        job_manager->terminateJob(job_that_will_be_terminated);
        // should a StandardJobTerminated event be sent? (if terminateJob is called then waitForAndProcessNextEvent() we get stuck)

        this->test->task2 = this->test->workflow->addTask("failed_task", 1000.0, 1, 1, 0);
        auto job_that_will_fail = job_manager->createStandardJob(this->test->task2);
        job_manager->submitJob(job_that_will_fail, this->test->compute_service);
        wrench::S4U_Simulation::sleep(10.0);
        wrench::Simulation::turnOffHost("ComputeHost");

        this->waitForAndProcessNextEvent();

        return 0;
    }
};

TEST_F(SimulationTimestampTaskTest, SimulationTimestampTaskTerminateAndFailTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampTaskTerminateAndFail_test);
}

void SimulationTimestampTaskTest::do_SimulationTimestampTaskTerminateAndFail_test() {
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-host-shutdown-simulation");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = wrench::Simulation::getHostnameList()[1];
    std::string execution_host = "ExecutionHost";
    std::string compute_host = "ComputeHost";

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::BareMetalComputeService(execution_host,
                                                                                          {std::make_pair(
                                                                                                  compute_host,
                                                                                                  std::make_tuple(wrench::ComputeService::ALL_CORES,
                                                                                                                  wrench::ComputeService::ALL_RAM))},
                                                                                          {})));

    ASSERT_NO_THROW(storage_service = simulation->add(wrench::SimpleStorageService::createSimpleStorageService(wms_host, {"/"},
                                                                                                               {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, "10MB"}})));

    std::shared_ptr<wrench::ExecutionController> wms = nullptr;

    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampTaskTerminateAndFailTestWMS(
                            this, wms_host)));

    simulation->getOutput().enableWorkflowTaskTimestamps(true);

    ASSERT_NO_THROW(simulation->launch());

    /**
     * expected timeline of events:
     *  - terminated_task start (SimulationTimestampTaskStart should be generated)
     *  - some time passes and request sent to terminate the job
     *  - terminated_task is terminated (SimulationTimestampTaskTerminated should be generated)
     *  - failed_task start (SimulatonTimestampTaskStart should be generated)
     *  - some time passes and compute services gets turned off while task1 is running
     *  - failed_task fails (SimulationTimestampTaskFailure should be generated)
     */
    auto start_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskStart>();
    auto terminated_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskTermination>();
    auto failure_timestamps = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskFailure>();

    // check the number of timestamps
    ASSERT_EQ(start_timestamps.size(), 2);
    ASSERT_EQ(terminated_timestamps.size(), 1);
    ASSERT_EQ(failure_timestamps.size(), 1);

    // check that endpoints were set correctly
    wrench::SimulationTimestampTask *terminated_task_start = start_timestamps[0]->getContent();
    wrench::SimulationTimestampTask *terminated_task_termination = terminated_timestamps[0]->getContent();
    ASSERT_EQ(terminated_task_start->getEndpoint(), terminated_task_termination);
    ASSERT_EQ(terminated_task_termination->getEndpoint(), terminated_task_start);

    wrench::SimulationTimestampTask *failed_task_start = start_timestamps[1]->getContent();
    wrench::SimulationTimestampTask *failed_task_failure = failure_timestamps[0]->getContent();
    ASSERT_EQ(failed_task_start->getEndpoint(), failed_task_failure);
    ASSERT_EQ(failed_task_failure->getEndpoint(), failed_task_start);

    // check that dates were set correctly
    ASSERT_EQ(std::floor(terminated_task_start->getDate()), 0);
    ASSERT_EQ(std::floor(terminated_task_termination->getDate()), 10);

    ASSERT_EQ(std::floor(failed_task_start->getDate()), 10);
    ASSERT_EQ(std::floor(failed_task_failure->getDate()), 20);


    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
