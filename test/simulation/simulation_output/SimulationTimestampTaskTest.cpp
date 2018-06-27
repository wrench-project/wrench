
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include "../../include/TestWithFork.h"

class SimulationTimestampTaskTest : public ::testing::Test {

public:
    wrench::ComputeService *compute_service = nullptr;
    wrench::StorageService *storage_service = nullptr;

    wrench::WorkflowTask *task1 = nullptr;
    wrench::WorkflowTask *task2 = nullptr;
    wrench::WorkflowTask *failed_task = nullptr;

    void do_SimulationTimestampTaskBasic_test();
    void do_SimulationTimestampTaskMultiple_test();

    void tester();

protected:
    SimulationTimestampTaskTest() {

        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"WMSHost\" speed=\"1f\" core=\"1\"/> "
                          "       <host id=\"ExecutionHost\" speed=\"1f\" core=\"1\"/> "
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <route src=\"ExecutionHost\" dst=\"WMSHost\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());
    }

    std::string platform_file_path = "/tmp/platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

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
class SimulationTimestampTaskBasicTestWMS : public wrench::WMS {
public:
    SimulationTimestampTaskBasicTestWMS(SimulationTimestampTaskTest *test,
        const std::set<wrench::ComputeService *> &compute_services,
        const std::set<wrench::StorageService *> &storage_services,
        std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampTaskTest *test;

    int main() {

        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        this->test->task1 = this->getWorkflow()->addTask("task1", 10.0, 1, 1, 1.0, 0);
        wrench::StandardJob *job1 = job_manager->createStandardJob(this->test->task1, {});
        job_manager->submitJob(job1, this->test->compute_service);
        this->waitForAndProcessNextEvent();

        this->test->task2 = this->getWorkflow()->addTask("task2", 10.0, 1, 1, 1.0, 0);
        wrench::StandardJob *job2 = job_manager->createStandardJob(this->test->task2, {});
        job_manager->submitJob(job2, this->test->compute_service);
        this->waitForAndProcessNextEvent();

        this->test->failed_task = this->getWorkflow()->addTask("failed_task", 1000.0, 1, 1, 1.0, 0);
        wrench::StandardJob *failed_job = job_manager->createStandardJob(this->test->failed_task, {});
        job_manager->submitJob(failed_job, this->test->compute_service);
        this->test->compute_service->stop();

        std::unique_ptr<wrench::WorkflowExecutionEvent> workflow_execution_event;
        try {
           workflow_execution_event = this->getWorkflow()->waitForNextExecutionEvent();
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Error getting the excution event: " + e.getCause()->toString());
        }

        if (workflow_execution_event->type != wrench::WorkflowExecutionEvent::STANDARD_JOB_FAILURE) {
            throw std::runtime_error("Job should have failed");
        }

        return 0;
    }
};

TEST_F(SimulationTimestampTaskTest, SimulationTimestampTaskBasicTest) {
   DO_TEST_WITH_FORK(do_SimulationTimestampTaskBasic_test);
}

void SimulationTimestampTaskTest::do_SimulationTimestampTaskBasic_test(){
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("simulation_timestamp_task_basic_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = simulation->getHostnameList()[1];
    std::string execution_host = simulation->getHostnameList()[0];

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::MultihostMulticoreComputeService(wms_host,
                                                                                                   {std::make_tuple(
                                                                                                           execution_host,
                                                                                                           wrench::ComputeService::ALL_CORES,
                                                                                                           wrench::ComputeService::ALL_RAM)},
                                                                                                   {})));

    ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(wms_host, 100000000000000.0)));

    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampTaskBasicTestWMS(
            this, {compute_service}, {storage_service}, wms_host
    )));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    /*
     * expected timeline: task1_start...task1_end...task2_start...task2_end
     * WorkflowTask member variables start_date and end_date should equal SimulationTimestamp<SimulationTimestampTaskStart>::getContent()->getDate()
     * and SimulationTimestamp<SimulationTimestampTaskCompletion>::getContent()->getDate() respectively
     */
    auto timestamp_start_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskStart>();
    double task1_start_date = this->task1->getStartDate();
    double task1_start_timestamp = timestamp_start_trace[0]->getContent()->getDate();

    ASSERT_DOUBLE_EQ(task1_start_date, task1_start_timestamp);

    auto timestamp_completion_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    double task1_completion_date = this->task1->getEndDate();
    double task1_completion_timestamp = timestamp_completion_trace[0]->getContent()->getDate();

    ASSERT_DOUBLE_EQ(task1_completion_date, task1_completion_timestamp);

    double task2_start_timestamp = timestamp_start_trace[1]->getContent()->getDate();
    double task2_completion_timestamp = timestamp_completion_trace[1]->getContent()->getDate();

    ASSERT_GT(task2_start_timestamp, task1_start_timestamp);
    ASSERT_GT(task2_completion_timestamp, task1_completion_timestamp);

    /*
     * expected timeline: task2_end...failed_task_start...failed_task_failed
     */

    auto timestamp_failure_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskFailure>();
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
            throw std::runtime_error("timstamp_completion_trace should not have the 'failed_task'");
        }
    }

    delete simulation;
    free(argv[0]);
    free(argv);
}

/**********************************************************************/
/**            SimulationTimestampTaskTestMultiple                   **/
/**********************************************************************/

/*
 * Testing that multiple timestamps of the same task are created for each time
 * that the task is run in a job.
 */

class SimulationTimestampTaskMultipleTestWMS : public wrench::WMS {
public:
    SimulationTimestampTaskMultipleTestWMS(SimulationTimestampTaskTest *test,
                                           const std::set<wrench::ComputeService *> &compute_services,
                                           const std::set<wrench::StorageService *> &storage_services,
                                           std::string &hostname) :
            wrench::WMS(nullptr, nullptr, compute_services, storage_services, {}, nullptr, hostname, "test") {
        this->test = test;
    }

private:
    SimulationTimestampTaskTest *test;

    int main() {

        std::shared_ptr<wrench::JobManager> job_manager = this->createJobManager();

        this->test->task1 = this->getWorkflow()->addTask("task1", 10.0, 1, 1, 1.0, 0);

        int num_task1_runs = 5;

        for (int i = 0; i < num_task1_runs; ++i) {
            this->test->task1->setInternalState(wrench::WorkflowTask::InternalState::TASK_READY);
            this->test->task1->setState(wrench::WorkflowTask::State::READY);

            wrench::StandardJob *job1 = job_manager->createStandardJob(this->test->task1, {});
            job_manager->submitJob(job1, this->test->compute_service);
            this->waitForAndProcessNextEvent();
        }

        this->test->failed_task = this->getWorkflow()->addTask("failed_task", 10, 1, 1, 1.0 ,0);
        int num_failed_task_runs = 5;

        for (int j = 0; j < num_failed_task_runs; ++j) {
            /*
             * Changing the internal state to TASK_FAILURE to emulate failure. WorkflowTask::setInternalState()
             * should create SimulationTimestampTaskFailures
             */
            this->test->failed_task->setInternalState(wrench::WorkflowTask::InternalState::TASK_READY);
            this->test->failed_task->setInternalState(wrench::WorkflowTask::InternalState::TASK_RUNNING);

            /*
             * SimulationTimestampTaskFailure::getContent()->getDate() should be 10s after the start time of the task.
             */
            wrench::S4U_Simulation::sleep(10);
            this->test->failed_task->setInternalState(wrench::WorkflowTask::InternalState::TASK_FAILED);
        }

        return 0;
    }
};

TEST_F(SimulationTimestampTaskTest, SimulationTimestampTaskMultipleTest) {
    DO_TEST_WITH_FORK(do_SimulationTimestampTaskMultiple_test);
}

void SimulationTimestampTaskTest::do_SimulationTimestampTaskMultiple_test() {
    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("simulation_timestamp_task_multiple_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    std::string wms_host = simulation->getHostnameList()[1];
    std::string execution_host = simulation->getHostnameList()[0];

    ASSERT_NO_THROW(compute_service = simulation->add(new wrench::MultihostMulticoreComputeService(wms_host,
                                                                                                   {std::make_tuple(
                                                                                                           execution_host,
                                                                                                           wrench::ComputeService::ALL_CORES,
                                                                                                           wrench::ComputeService::ALL_RAM)},
                                                                                                   {})));

    ASSERT_NO_THROW(storage_service = simulation->add(new wrench::SimpleStorageService(wms_host, 100000000000000.0)));

    wrench::WMS *wms = nullptr;
    ASSERT_NO_THROW(wms = simulation->add(new SimulationTimestampTaskMultipleTestWMS(
            this, {compute_service}, {storage_service}, wms_host
    )));

    ASSERT_NO_THROW(wms->addWorkflow(workflow.get()));

    ASSERT_NO_THROW(simulation->launch());

    auto starts_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskStart>();
    auto completions_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    auto failures_trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskFailure>();

/*    for (auto &s : starts) {
        std::cerr << std::setw(15) << s->getContent()->getTask()->getID() << std::setw(15) << s->getContent()->getDate() << std::endl;
    }

    std::cerr << " " << std::endl;

    for (auto &c : completions) {
        std::cerr << std::setw(15) << c->getContent()->getTask()->getID() << std::setw(15) << c->getContent()->getDate() << std::endl;
    }

    std::cerr << " " << std::endl;

    for (auto &f: failures) {
        std::cerr << std::setw(15) << f->getContent()->getTask()->getID() << std::setw(15) << f->getContent()->getDate() << std::endl;
    }*/


    /*
     * Check that we have the right number of SimulationTimestampTaskXXXX
     *
     * Ran 10 tasks:
     *  - 10 started: task1 x 5, failed_task x 5
     *  - 5 completed: task1 x 5
     *  - 5 failed : failed_task x 5
     */
    ASSERT_EQ(starts_trace.size(), 10);
    ASSERT_EQ(completions_trace.size(), 5);
    ASSERT_EQ(failures_trace.size(), 5);

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

    ASSERT_EQ(num_task1_starts, 5);
    ASSERT_EQ(num_failed_task_starts, 5);
    ASSERT_EQ(num_task1_completions, 5);
    ASSERT_EQ(num_failed_task_failures, 5);

    /*
     * Check that the elapsed time between a task start and completion/failure is about 10 seconds.
     * There should be about a 10s difference in each of the dates in each trace.
     */
    for (size_t i = 1; i < starts_trace.size(); ++i) {
        double previous_start = std::floor(starts_trace.at(i-1)->getContent()->getDate());
        double current_start = std::floor(starts_trace.at(i)->getContent()->getDate());

        ASSERT_DOUBLE_EQ(current_start - previous_start, 10.0);
    }

    for (size_t i = 1; i < completions_trace.size(); ++i) {
        double previous_completion = std::floor(completions_trace.at(i-1)->getContent()->getDate());
        double current_completion = std::floor(completions_trace.at(i)->getContent()->getDate());

        ASSERT_DOUBLE_EQ(current_completion - previous_completion, 10.0);
    }

    for (size_t i = 1; i < failures_trace.size(); ++i) {
        double previous_failure = std::floor(failures_trace.at(i-1)->getContent()->getDate());
        double current_failure = std::floor(failures_trace.at(i)->getContent()->getDate());

        ASSERT_DOUBLE_EQ(current_failure - previous_failure, 10.0);
    }

    delete simulation;
    free(argv[0]);
    free(argv);
}

