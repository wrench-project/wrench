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

WRENCH_LOG_CATEGORY(batch_service_conservative_bf_test, "Log category for BatchServiceCONSERVATIVEBFTest");

class BatchServiceCONSERVATIVE_BFTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
//    wrench::Simulation *simulation;

    void do_SimpleCONSERVATIVE_BF_test();
    void do_LargeCONSERVATIVE_BF_test(int seed);
    void do_SimpleCONSERVATIVE_BFQueueWaitTimePrediction_test();
    void do_BatschedBroken_test();
    int seed;

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
        std::shared_ptr<wrench::StandardJob> jobs[4];
        for (int i=0; i < 4; i++) {
            tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60, 1, 1, 0);
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


#define NUM_JOBS 300

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

        unsigned int random = this->test->seed;

        wrench::WorkflowTask *tasks[NUM_JOBS];
        std::shared_ptr<wrench::StandardJob> jobs[NUM_JOBS];
        // Create 4 1-min tasks and submit them as various shaped jobs
        for (int i=0; i < NUM_JOBS; i++) {
            random = random  * 17 + 4123451;
            tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60 + 60*(random % 30), 1, 1, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i], {});
        }

        // Submit jobs
        try {
            for (int i=0; i < NUM_JOBS; i++) {
                std::map<std::string, std::string> job_specific_args;
                random = random  * 17 + 4123451;
                job_specific_args["-N"] = std::to_string(1 + random % 4);
                random = random  * 17 + 4123451;
                job_specific_args["-t"] = std::to_string(1 + random % 100);
                job_specific_args["-c"] = "10";
                job_manager->submitJob(jobs[i], this->test->compute_service, job_specific_args);
            }
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting job"
            );
        }

        std::map<std::string, std::pair<std::shared_ptr<wrench::StandardJob>, double>>  actual_completion_times;
        for (int i=0; i < NUM_JOBS; i++) {
            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                actual_completion_times[real_event->standard_job->getName()] =  std::make_pair(real_event->standard_job,  wrench::Simulation::getCurrentSimulatedDate());
            } else if (auto real_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(event)) {
                actual_completion_times[real_event->standard_job->getName()] = std::make_pair(real_event->standard_job,
                                                                                              wrench::Simulation::getCurrentSimulatedDate());
            }  else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Get predictions for scalability measures
            std::set<std::tuple<std::string,unsigned long,unsigned long, double>> set_of_jobs = {
                    (std::tuple<std::string, unsigned long, unsigned long, double>) {"testing_job1_" + std::to_string(i), 1, 10, 60},
                    (std::tuple<std::string, unsigned long, unsigned long, double>) {"testing_job2_" + std::to_string(i), 1, 10, 10000},
                    (std::tuple<std::string, unsigned long, unsigned long, double>) {"testing_job3_" + std::to_string(i), 2, 10, 10},
                    (std::tuple<std::string, unsigned long, unsigned long, double>) {"testing_job4_" + std::to_string(i), 2, 10, 80},
                    (std::tuple<std::string, unsigned long, unsigned long, double>) {"testing_job5_" + std::to_string(i), 5, 10, 400},
                    (std::tuple<std::string, unsigned long, unsigned long, double>) {"testing_job6_" + std::to_string(i), 3, 10, 400},
                    (std::tuple<std::string, unsigned long, unsigned long, double>) {"testing_job7_" + std::to_string(i), 4, 10, 400},
            };

            std::map<std::string,double> jobs_estimated_start_times =
                    (*(this->getAvailableComputeServices<wrench::BatchComputeService>().begin()))->getStartTimeEstimates(set_of_jobs);

        }

        // Print Completion times:
#if 0
        std::cerr << "--------------\n";
        for (auto const &i : actual_completion_times) {
            auto job = i.second.first;
            double completion_time = i.second.second;
            std::cerr << "- " << i.first.c_str()  <<  ":" << "\t-N:" <<
                    job->getServiceSpecificArguments()["-N"].c_str() << " -t:"<<
                    job->getServiceSpecificArguments()["-t"].c_str() << " (real=" <<
                    job->getTasks().at(0)->getFlops()/60 << ")   \tCT="<<
                    completion_time/60.0 << "\n";
//            WRENCH_INFO("COMPLETION TIME %s (%s nodes, %s minutes): %lf",
//                        i.first.c_str(),
//                        job->getServiceSpecificArguments()["-N"].c_str(),
//                        job->getServiceSpecificArguments()["-t"].c_str(),
//                        completion_time);
        }
#endif

        return 0;
    }
};

//  Discrepancy with BATSCHED
//discrepancy: 6 jobs    seed=3!!
//discrepancy: 5 jobs    seed=6!!
//discrepancy: 4 jobs    seed=25!!
//discrepancy: 3 jobs    seed=25


#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceCONSERVATIVE_BFTest, LargeCONSERVATIVE_BFTest)
#else
TEST_F(BatchServiceCONSERVATIVE_BFTest, LargeCONSERVATIVE_BFTest)
#endif
{
//    DO_TEST_WITH_FORK(do_LargeCONSERVATIVE_BF_test);
    for  (int seed =1; seed < 2; seed++) {
//        std::cerr <<  "SEED = " << seed << "\n";
        DO_TEST_WITH_FORK_ONE_ARG(do_LargeCONSERVATIVE_BF_test, seed);
    }
}


void BatchServiceCONSERVATIVE_BFTest::do_LargeCONSERVATIVE_BF_test(int seed) {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    this->seed = seed;

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {
                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                             })));

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




/**********************************************************************/
/**  SIMPLE CONSERVATIVE_BF TEST WITH QUEUE PREDICTION               **/
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

        wrench::WorkflowTask *tasks[4];
        std::shared_ptr<wrench::StandardJob> jobs[4];
        for (int i=0; i < 4; i++) {
            tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60, 1, 1, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i], {});
        }

        std::map<std::string, std::string>
                one_hosts_ten_cores_1min,
                two_hosts_ten_cores_1min,
                two_hosts_ten_cores_2min,
                three_hosts_ten_cores_1min;

        one_hosts_ten_cores_1min["-N"] = "1";
        one_hosts_ten_cores_1min["-t"] = "1";
        one_hosts_ten_cores_1min["-c"] = "10";

        two_hosts_ten_cores_1min["-N"] = "2";
        two_hosts_ten_cores_1min["-t"] = "1";
        two_hosts_ten_cores_1min["-c"] = "10";

        two_hosts_ten_cores_2min["-N"] = "2";
        two_hosts_ten_cores_2min["-t"] = "2";
        two_hosts_ten_cores_2min["-c"] = "10";


        three_hosts_ten_cores_1min["-N"] = "3";
        three_hosts_ten_cores_1min["-t"] = "1";
        three_hosts_ten_cores_1min["-c"] = "10";


        std::map<std::string, std::string> job_args[4] = {
                two_hosts_ten_cores_2min,
                three_hosts_ten_cores_1min,
                three_hosts_ten_cores_1min,
                one_hosts_ten_cores_1min,
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

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);
        std::set<std::tuple<std::string,unsigned long,unsigned long, double>> set_of_jobs = {
                (std::tuple<std::string,unsigned long,unsigned long, double>){"job1",  1, 10, 60},    // 0
                (std::tuple<std::string,unsigned long,unsigned long, double>){"job2",  1, 10, 10000}, // 0
                (std::tuple<std::string,unsigned long,unsigned long, double>){"job3",  2, 10, 10},    // 60
                (std::tuple<std::string,unsigned long,unsigned long, double>){"job4",  2, 10, 80},    // 240
                (std::tuple<std::string,unsigned long,unsigned long, double>){"job5",  5, 10, 400},   // -1
                (std::tuple<std::string,unsigned long,unsigned long, double>){"job6",  3, 10, 400},   // 240
                (std::tuple<std::string,unsigned long,unsigned long, double>){"job7",  4, 10, 400},  // 240
        };

        // Get Predictions

        // Expectations
        std::map<std::string, double> expectations;
        expectations.insert(std::make_pair("job1", 0));
        expectations.insert(std::make_pair("job2", 0));
        expectations.insert(std::make_pair("job3", 60));
        expectations.insert(std::make_pair("job4", 240));
        expectations.insert(std::make_pair("job5", -1));
        expectations.insert(std::make_pair("job6", 240));
        expectations.insert(std::make_pair("job7", 240));

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
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                             {wrench::BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY,   "0"}})));

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
/**  BATSCHED BROKEN TEST                                            **/
/**********************************************************************/

class BatschedBrokenTestWMS : public wrench::WMS {

public:
    BatschedBrokenTestWMS(BatchServiceCONSERVATIVE_BFTest *test,
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

        wrench::WorkflowTask *tasks[5];
        std::shared_ptr<wrench::StandardJob> jobs[5];
        for (int i=0; i < 5; i++) {
            tasks[i] = this->getWorkflow()->addTask("task" + std::to_string(i), 60, 1, 1, 0);
            jobs[i] = job_manager->createStandardJob(tasks[i], {});
        }

        std::map<std::string, std::string>
                job1,
                job2,
                job3,
                job4,
                job5;

        job1["-N"] = "4";
        job1["-t"] = "75";
        job1["-c"] = "10";

        job2["-N"] = "2";
        job2["-t"] = "25";
        job2["-c"] = "10";

        job3["-N"] = "4";
        job3["-t"] = "67";
        job3["-c"] = "10";

        job4["-N"] = "2";
        job4["-t"] = "25";
        job4["-c"] = "10";

        job5["-N"] = "4";
        job5["-t"] = "59";
        job5["-c"] = "10";

        int num_jobs_submitted = 0;

        // Submit jobs
        try {
            job_manager->submitJob(jobs[0], this->test->compute_service, job1); num_jobs_submitted++;
            job_manager->submitJob(jobs[1], this->test->compute_service, job2); num_jobs_submitted++;
            job_manager->submitJob(jobs[2], this->test->compute_service, job3); num_jobs_submitted++;
            job_manager->submitJob(jobs[3], this->test->compute_service, job4); num_jobs_submitted++;
            job_manager->submitJob(jobs[4], this->test->compute_service, job5); num_jobs_submitted++;
        } catch (wrench::WorkflowExecutionException &e) {
            throw std::runtime_error("Unexpected exception while submitting job");
        }

        std::map<std::string, std::pair<std::shared_ptr<wrench::StandardJob>, double>>  actual_completion_times;
        for (int i=0; i < num_jobs_submitted; i++) {
            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                actual_completion_times[real_event->standard_job->getName()] =
                        std::make_pair(real_event->standard_job,  wrench::Simulation::getCurrentSimulatedDate());
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        // Print Completion times:
        WRENCH_INFO("----------");
        for (auto const &i : actual_completion_times) {
            auto job = i.second.first;
            double completion_time = i.second.second;
            WRENCH_INFO("COMPLETION TIME %s (%s nodes, %s minutes): %lf",
                        i.first.c_str(),
                        job->getServiceSpecificArguments()["-N"].c_str(),
                        job->getServiceSpecificArguments()["-t"].c_str(),
                        completion_time);
        }

//        // Check
//        for (int i=0; i < 5; i++) {
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
TEST_F(BatchServiceCONSERVATIVE_BFTest, DISABLED_BatschedBrokenTest)
#else
TEST_F(BatchServiceCONSERVATIVE_BFTest, BatschedBrokenTest)
#endif
{
    DO_TEST_WITH_FORK(do_BatschedBroken_test);
}


void BatchServiceCONSERVATIVE_BFTest::do_BatschedBroken_test() {

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
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                             {wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BatschedBrokenTestWMS(
                    this,  {compute_service}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}
