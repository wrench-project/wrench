/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include "helper_services/standard_job_executor/StandardJobExecutorMessage.h"
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include "wrench/workflow/job/PilotJob.h"

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

#define EPSILON 0.05

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_conservative_bf_test, "Log category for BatchServiceCONSERVATIVEBFTest");

class BatchServiceCONSERVATIVE_BFTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    wrench::Simulation *simulation;

    void do_SimpleCONSERVATIVE_BF_test();
    void do_LargeCONSERVATIVE_BF_test();
    void do_SimpleCONSERVATIVE_BFQueueWaitTimePrediction_test();
    void do_BrokenQueueWaitTimePrediction_test();

protected:
    BatchServiceCONSERVATIVE_BFTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"3\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host4\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\""
                          "/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);

    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
    std::unique_ptr<wrench::Workflow> workflow;

};

/**********************************************************************/
/**  SIMPLE CONSERVATIVE_BF TEST                                     **/
/**********************************************************************/

class SimpleCONSERVATIVE_BFTestWMS : public wrench::WMS {

public:
    SimpleCONSERVATIVE_BFTestWMS(BatchServiceCONSERVATIVE_BFTest *test,
                      const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                      std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    BatchServiceCONSERVATIVE_BFTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create 4 1-min tasks and submit them as various shaped jobs

        wrench::WorkflowTask *tasks[4];
        wrench::StandardJob *jobs[4];
        for (int i=0; i < 4; i++) {
            tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60, 1, 1, 1.0, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i], {});
        }

        std::map<std::string, std::string>
                job1,
                job2,
                job3,
                job4;

        job1["-N"] = "2";
        job1["-t"] = "10";
        job1["-c"] = "10";

        job2["-N"] = "2";
        job2["-t"] = "2";
        job2["-c"] = "10";

        job3["-N"] = "4";
        job3["-t"] = "2";
        job3["-c"] = "10";

        job4["-N"] = "2";
        job4["-t"] = "5";
        job4["-c"] = "10";



        std::map<std::string, std::string> job_args[4] = {
                job1,
                job2,
                job3,
                job4,
        };

        double expected_completion_times[4] = {
                60,
                60,
                120,
                180,
        };

        // Submit jobs
        try {
            for (int i=0; i < 4; i++) {
                job_manager->submitJob(jobs[i], this->test->compute_service, job_args[i]);
            }
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting job"
            );
        }

        double actual_completion_times[4];
        for (int i=0; i < 4; i++) {
            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                actual_completion_times[i] =  wrench::Simulation::getCurrentSimulatedDate();
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        // Check
        for (int i=0; i < 4; i++) {
            double delta = std::abs(actual_completion_times[i] - expected_completion_times[i]);
            if (delta > EPSILON) {
                throw std::runtime_error("Unexpected job completion time for the job containing task " +
                                         tasks[i]->getID() +
                                         ": " +
                                         std::to_string(actual_completion_times[i]) +
                                         "(expected: " +
                                         std::to_string(expected_completion_times[i]) +
                                         ")");
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
//TEST_F(BatchServiceCONSERVATIVE_BFTest, DISABLED_SimpleCONSERVATIVE_BFTest)
TEST_F(BatchServiceCONSERVATIVE_BFTest, SimpleCONSERVATIVE_BFTest)
#else
TEST_F(BatchServiceCONSERVATIVE_BFTest, SimpleCONSERVATIVE_BFTest)
#endif
{
    DO_TEST_WITH_FORK(do_SimpleCONSERVATIVE_BF_test);
}


void BatchServiceCONSERVATIVE_BFTest::do_SimpleCONSERVATIVE_BF_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new SimpleCONSERVATIVE_BFTestWMS(
                    this,  {compute_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  LARGE CONSERVATIVE_BF TEST                                     **/
/**********************************************************************/

#define NUM_JOBS 70

#define SEED 37

class LargeCONSERVATIVE_BFTestWMS : public wrench::WMS {

public:
    LargeCONSERVATIVE_BFTestWMS(BatchServiceCONSERVATIVE_BFTest *test,
                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    BatchServiceCONSERVATIVE_BFTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        srand(SEED);

        // Create 4 1-min tasks and submit them as various shaped jobs

        wrench::WorkflowTask *tasks[NUM_JOBS];
        wrench::StandardJob *jobs[NUM_JOBS];
        for (int i=0; i < NUM_JOBS; i++) {
            tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60, 1, 1, 1.0, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i], {});
        }

        std::map<std::string, std::string> job_specific_args;
        job_specific_args["-N"] = "2";
        job_specific_args["-t"] = "10";
        job_specific_args["-c"] = "10";


        // Submit jobs
        try {
            for (int i=0; i < NUM_JOBS; i++) {
                std::map<std::string, std::string> job_specific_args;
                job_specific_args["-N"] = std::to_string(1 + rand() % 4);
                job_specific_args["-t"] = std::to_string(2 + rand() % 100);  // TODO: CHANGE the 2 to a 1, NUM_JOBS=200, SEED 37
                job_specific_args["-c"] = "10";
                job_manager->submitJob(jobs[i], this->test->compute_service, job_specific_args);
            }
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting job"
            );
        }

        std::map<std::string, double>  actual_completion_times;
        for (int i=0; i < NUM_JOBS; i++) {
            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                actual_completion_times[real_event->standard_job->getName()] =  wrench::Simulation::getCurrentSimulatedDate();

            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        // Print Completion times:
        for (auto const &i : actual_completion_times) {
            std::cerr << "COMPLETION TIME #" << i.first << ": " << i.second << "\n";
        }

//        // Check
//        for (int i=0; i < NUM_JOBS; i++) {
//            double delta = std::abs(actual_completion_times[i] - expected_completion_times[i]);
//            if (delta > EPSILON) {
//                throw std::runtime_error("Unexpected job completion time for the job containing task " +
//                                         tasks[i]->getID() +
//                                         ": " +
//                                         std::to_string(actual_completion_times[i]) +
//                                         "(expected: " +
//                                         std::to_string(expected_completion_times[i]) +
//                                         ")");
//            }
//        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
//TEST_F(BatchServiceCONSERVATIVE_BFTest, DISABLED_LargeCONSERVATIVE_BFTest)
TEST_F(BatchServiceCONSERVATIVE_BFTest, LargeCONSERVATIVE_BFTest)
#else
TEST_F(BatchServiceCONSERVATIVE_BFTest, LargeCONSERVATIVE_BFTest)
#endif
{
    DO_TEST_WITH_FORK(do_LargeCONSERVATIVE_BF_test);
}


void BatchServiceCONSERVATIVE_BFTest::do_LargeCONSERVATIVE_BF_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new LargeCONSERVATIVE_BFTestWMS(
                    this,  {compute_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}

#if 0


/**********************************************************************/
/**  SIMPLE CONSERVATIVE_BF TEST WITH QUEUE PREDICTION                          **/
/**********************************************************************/

class SimpleCONSERVATIVE_BFQueueWaitTimePredictionWMS : public wrench::WMS {

public:
    SimpleCONSERVATIVE_BFQueueWaitTimePredictionWMS(BatchServiceCONSERVATIVE_BFTest *test,
                                         const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                         std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    BatchServiceCONSERVATIVE_BFTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create 4 tasks and submit them as three various shaped jobs

        wrench::WorkflowTask *tasks[9];
        wrench::StandardJob *jobs[9];
        for (int i=0; i < 9; i++) {
            tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60, 1, 1, 1.0, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i], {});
        }

        std::map<std::string, std::string>
                two_hosts_ten_cores,
                two_hosts_five_cores,
                one_hosts_five_cores,
                three_hosts_five_cores,
                one_host_four_cores_short;

        two_hosts_ten_cores["-N"] = "2";
        two_hosts_ten_cores["-t"] = "2";
        two_hosts_ten_cores["-c"] = "10";

        two_hosts_five_cores["-N"] = "2";
        two_hosts_five_cores["-t"] = "2";
        two_hosts_five_cores["-c"] = "5";

        one_hosts_five_cores["-N"] = "1";
        one_hosts_five_cores["-t"] = "2";
        one_hosts_five_cores["-c"] = "5";

        three_hosts_five_cores["-N"] = "3";
        three_hosts_five_cores["-t"] = "2";
        three_hosts_five_cores["-c"] = "5";

        one_host_four_cores_short["-N"] = "1";
        one_host_four_cores_short["-t"] = "1";
        one_host_four_cores_short["-c"] = "4";

        std::map<std::string, std::string> job_args[9] = {
                two_hosts_ten_cores,
                three_hosts_five_cores,
                two_hosts_ten_cores,
                two_hosts_ten_cores,
                three_hosts_five_cores,
                two_hosts_five_cores,
                one_hosts_five_cores,
                two_hosts_five_cores,
                one_host_four_cores_short
        };


        // Submit jobs
        try {
            for (int i=0; i < 9; i++) {
                job_manager->submitJob(jobs[i], this->test->compute_service, job_args[i]);
            }
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting job"
            );
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Get Predictions
        std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job1", 1, 1, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job2", 5, 1, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job3", 4, 10, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job4", 1, 6, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job5", 2, 6, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job6", 2, 7, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job7", 3, 7, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job8", 4, 4, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job9", 1, 1, 400},
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job10", 1, 2, 400},
        };

        // Expectations
        std::map<std::string, double> expectations;
        expectations.insert(std::make_pair("job1", 480));
        expectations.insert(std::make_pair("job2", -1));
        expectations.insert(std::make_pair("job3", 600));
        expectations.insert(std::make_pair("job4", 480));
        expectations.insert(std::make_pair("job5", 480));
        expectations.insert(std::make_pair("job6", 480));
        expectations.insert(std::make_pair("job7", 600));
        expectations.insert(std::make_pair("job8", 540));
        expectations.insert(std::make_pair("job9", 480));
        expectations.insert(std::make_pair("job10", 480));

        std::map<std::string,double> jobs_estimated_start_times =
                (*(this->getAvailableComputeServices<wrench::BatchComputeService>().begin()))->getStartTimeEstimates(set_of_jobs);

        for (auto job : set_of_jobs) {
            std::string id = std::get<0>(job);
            double estimated = jobs_estimated_start_times[id];
            double expected = expectations[id];
            if (std::abs(estimated - expected) > 1.0) {
                throw std::runtime_error("invalid prediction for job '" + id + "': got " +
                                         std::to_string(estimated) + " but expected is " + std::to_string(expected));
            }
        }

        wrench::Simulation::sleep(10);

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceCONSERVATIVE_BFTest, DISABLED_SimpleCONSERVATIVE_BFQueueWaitTimePrediction)
#else
TEST_F(BatchServiceCONSERVATIVE_BFTest, SimpleCONSERVATIVE_BFQueueWaitTimePrediction)
#endif
{
    DO_TEST_WITH_FORK(do_SimpleCONSERVATIVE_BFQueueWaitTimePrediction_test);
}


void BatchServiceCONSERVATIVE_BFTest::do_SimpleCONSERVATIVE_BFQueueWaitTimePrediction_test() {


    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new SimpleCONSERVATIVE_BFQueueWaitTimePredictionWMS(
                    this,  {compute_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  BROKEN TEST WITH QUEUE PREDICTION                               **/
/**********************************************************************/

class BrokenQueueWaitTimePredictionWMS : public wrench::WMS {

public:
    BrokenQueueWaitTimePredictionWMS(BatchServiceCONSERVATIVE_BFTest *test,
                                     const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                     std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, {}, {}, nullptr, hostname,
                        "test") {
        this->test = test;
    }

private:

    BatchServiceCONSERVATIVE_BFTest *test;

    int main() {

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Get Predictions
        std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {
                (std::tuple<std::string,unsigned int,unsigned int, double>){"job1", 1, 1, 400}
        };


        try {
            std::map<std::string,double> jobs_estimated_start_times =
                    (*(this->getAvailableComputeServices<wrench::BatchComputeService>().begin()))->getStartTimeEstimates(set_of_jobs);
            throw std::runtime_error("Should not have been able to get prediction for BESTFIT algorithm");
        } catch (wrench::WorkflowExecutionException &e) {
            auto cause = std::dynamic_pointer_cast<wrench::FunctionalityNotAvailable>(e.getCause());
            if (not cause) {
                throw std::runtime_error("Got expected exception, but unexpected failure cause: " +
                                         e.getCause()->toString() + "(Expected: FunctionalityNotAvailable)");
            }
            if (cause->getService() != this->test->compute_service) {
                throw std::runtime_error("Got expected exception and cause type, but compute service is wrong");
            }
            if (cause->getFunctionalityName() != "start time estimates") {
                throw std::runtime_error("Got expected exception and cause type, but functionality name is wrong (" +
                                         cause->getFunctionalityName() + ")");
            }
            WRENCH_INFO("toString: %s", cause->toString().c_str());  // for coverage

        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceCONSERVATIVE_BFTest, DISABLED_BrokenQueueWaitTimePrediction)
#else
TEST_F(BatchServiceCONSERVATIVE_BFTest, BrokenQueueWaitTimePrediction)
#endif
{
    DO_TEST_WITH_FORK(do_BrokenQueueWaitTimePrediction_test);
}


void BatchServiceCONSERVATIVE_BFTest::do_BrokenQueueWaitTimePrediction_test() {


    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "fcfs"},
                                             {wrench::BatchComputeServiceProperty::HOST_SELECTION_ALGORITHM, "BESTFIT"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BrokenQueueWaitTimePredictionWMS(
                    this,  {compute_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


#endif
