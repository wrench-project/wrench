/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/simulation/SimulationMessage.h>
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

#define EPSILON 0.05

WRENCH_LOG_CATEGORY(batch_service_easy_bf_test, "Log category for BatchServiceEASYBFTest");

class BatchServiceEASY_BFTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::Workflow> workflow;

    std::shared_ptr<wrench::BatchComputeService> compute_service = nullptr;

    void do_EASY_BF_test(int num_compute_nodes, std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec);
//    void do_SimpleEASY_BF_test_1();
//    void do_SimpleEASY_BF_test_2();
//    void do_SimpleEASY_BF_test_3(int seed);

    int _seed{};


protected:
    ~BatchServiceEASY_BFTest() override {
        workflow->clear();
        wrench::Simulation::removeAllFiles();
    }

    BatchServiceEASY_BFTest() {

        // Create the simplest workflow
        workflow = wrench::Workflow::createWorkflow();

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host5\" speed=\"1f\" core=\"10\"/> "
                          "       <host id=\"Host6\" speed=\"1f\" core=\"10\"/> "
                          "       <link id=\"1\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"3\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host5\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host1\" dst=\"Host6\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host5\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host2\" dst=\"Host6\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host5\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host3\" dst=\"Host6\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host4\" dst=\"Host5\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host4\" dst=\"Host6\"> <link_ctn id=\"1\"/> </route>"
                          "       <route src=\"Host5\" dst=\"Host6\"> <link_ctn id=\"1\"/> </route>"
                          "   </zone> "
                          "</platform>";
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  SIMPLE EASY_BF GENERIC      (DEPTH=0)                           **/
/**********************************************************************/

class EASY_BFTest_WMS : public wrench::ExecutionController {

public:
    EASY_BFTest_WMS(BatchServiceEASY_BFTest *test,
                    const std::string& hostname,
                    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->spec = spec;
    }

private:
    BatchServiceEASY_BFTest *test;
    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::vector<std::shared_ptr<wrench::CompoundJob>> jobs;
        std::map<std::string, double> completion_times;
        std::map<std::string, double> expected_completion_times;

        bool print_completion_times = false;
        // Create and submit all jobs
        for (auto const &job_spec : spec) {
            std::string job_name =std::get<0>(job_spec);
            int num_nodes = std::get<1>(job_spec);
            int duration = std::get<2>(job_spec);
            int sleep_after = std::get<3>(job_spec);
            expected_completion_times[job_name] = std::get<4>(job_spec);
            if (expected_completion_times[job_name] < 0) {
                print_completion_times = true;
            }

            auto job = job_manager->createCompoundJob(job_name);
            job->addSleepAction("sleep" + std::to_string(duration), duration);
            std::map<std::string, std::string> args = {{"-N", std::to_string(num_nodes)}, {"-t", std::to_string(duration)}, {"-c", "10"}};
            job_manager->submitJob(job, this->test->compute_service, args);
            jobs.push_back(job);
            wrench::Simulation::sleep(sleep_after);
        }

        for (unsigned int i=0; i < jobs.size(); i++) {
            auto event = this->waitForNextEvent();
            if (auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
                completion_times[real_event->job->getName()] = wrench::Simulation::getCurrentSimulatedDate();
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        if (print_completion_times) {
            for (auto const &item: completion_times) {
                std::cerr << "  " << item.first << ": " << item.second << std::endl;
            }
        } else {
            for (auto const &item: completion_times) {
                if (std::abs(item.second - expected_completion_times[item.first]) > 0.001) {
                    throw std::runtime_error("Invalid job completion time for " + item.first + ": " +
                                             std::to_string(item.second) + "(should be " +
                                             std::to_string(expected_completion_times[item.first]) + ")");
                }
            }
        }

        return 0;
    }
};

void BatchServiceEASY_BFTest::do_EASY_BF_test(int num_compute_nodes,
                                              std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

#ifdef ENABLE_BATSCHED
    std::string scheduling_algorithm = "easy_bf";
#else
    std::string scheduling_algorithm = "easy_bf_depth0";
#endif

    std::vector<std::string> compute_hosts;
    for (int i=1; i <= num_compute_nodes; i++) {
        compute_hosts.push_back("Host" + std::to_string(i));
    }

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, compute_hosts, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
                                             {wrench::BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY, "0"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    ASSERT_NO_THROW(simulation->add(new EASY_BFTest_WMS(this, hostname, spec)));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_1)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_1)
#endif
{
    // job_name, num_nodes, duration, sleep_after, expected CT
    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec = {
            {"job1", 2, 60, 0, 60},
            {"job2", 4, 30, 0, 110},
            {"job3", 2, 30, 0, 30},
            {"job4", 2, 50, 0, 80}
    };

    DO_TEST_WITH_FORK_TWO_ARGS(do_EASY_BF_test, 4, spec);
}


#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_2)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_2)
#endif
{
    // job_name, num_nodes, duration, sleep_after, expected CT
    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec = {
            {"job1", 1, 6000, 0, 6000},
            {"job2", 2, 70, 0, 70},
            {"job3", 4, 20, 0, 90},
            {"job4", 5, 20, 0, 6020},
            {"job5", 1, 6000, 0, 6000},
    };

    DO_TEST_WITH_FORK_TWO_ARGS(do_EASY_BF_test, 6, spec);
}

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_3)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_3)
#endif
{
    // job_name, num_nodes, duration, sleep_after, expected CT
    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec = {
            {"job1", 3, 660, 1, 660},
            {"job2", 1, 120, 1, 121},
            {"job3", 3, 1740, 1, 660 + 1740},
            {"job4", 1, 1080, 1, 1083},
    };

    DO_TEST_WITH_FORK_TWO_ARGS(do_EASY_BF_test, 6, spec);
}

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_RANDOM)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_RANDOM)
#endif
{
    int num_jobs = 5;
    for (int seed = 0; seed < 10; seed++) {
        std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec;
        std::cerr << "SEED: " << seed << "\n";
        unsigned int random = seed;
        for (int i = 1; i <= num_jobs; i++) {
            std::string job_name = "job" + std::to_string(i);
            random = random * 17 + 4123451;
            unsigned int num_nodes = 1 + random % 4;
            random = random * 17 + 4123451;
            unsigned int duration = 60 + 60 * (random % 30);
            int expected_ct = -1;
            spec.push_back(std::make_tuple(job_name, num_nodes, duration, 0, expected_ct));
        }
        DO_TEST_WITH_FORK_TWO_ARGS(do_EASY_BF_test, 6, spec);
    }
}




#if 0

/**********************************************************************/
/**  SIMPLE EASY_BF TEST #1      (DEPTH=0)                           **/
/**********************************************************************/

class SimpleEASY_BFTest_1_WMS : public wrench::ExecutionController {

public:
    SimpleEASY_BFTest_1_WMS(BatchServiceEASY_BFTest *test,
                            const std::string& hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BatchServiceEASY_BFTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // |333    222
        // |333    222
        // |111111 222 444
        // |111111 222 444

        auto job1 = job_manager->createCompoundJob("job1");
        job1->addSleepAction("sleep60", 60);
        std::map<std::string, std::string> job1_args = {{"-N", "2"}, {"-t", "60"}, {"-c", "10"}};
        job_manager->submitJob(job1, this->test->compute_service, job1_args);

        auto job2 = job_manager->createCompoundJob("job2");
        job2->addSleepAction("sleep60", 30);
        std::map<std::string, std::string> job2_args = {{"-N", "4"}, {"-t", "30"}, {"-c", "10"}};
        job_manager->submitJob(job2, this->test->compute_service, job2_args);

        auto job3 = job_manager->createCompoundJob("job3");
        job3->addSleepAction("sleep60", 30);
        std::map<std::string, std::string> job3_args = {{"-N", "2"}, {"-t", "30"}, {"-c", "10"}};
        job_manager->submitJob(job3, this->test->compute_service, job3_args);

        auto job4 = job_manager->createCompoundJob("job4");
        job4->addSleepAction("sleep50", 50);
        std::map<std::string, std::string> job4_args = {{"-N", "2"}, {"-t", "50"}, {"-c", "10"}};
        job_manager->submitJob(job4, this->test->compute_service, job4_args);

        std::map<std::string, double> completion_times;
        for (int i=0; i < 4; i++) {
            auto event = this->waitForNextEvent();
            if (auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
                completion_times[real_event->job->getName()] = wrench::Simulation::getCurrentSimulatedDate();
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        std::map<std::string, double> expected_completion_times = {
                {"job1", 60},
                {"job2", 110},
                {"job3", 30},
                {"job4", 80}
        };

        for (auto const &item : completion_times) {
            std::cerr << item.first << ": " << item.second << std::endl;
        }

        for (auto const &item : completion_times) {
            if (std::abs(item.second - expected_completion_times[item.first]) > 0.001) {
                throw std::runtime_error("Invalid job completion time for " + item.first + ": " +
                                         std::to_string(item.second) + "(should be " + std::to_string(expected_completion_times[item.first]) + ")");
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_1)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_1)
#endif
{
    DO_TEST_WITH_FORK(do_SimpleEASY_BF_test_1);
}

void BatchServiceEASY_BFTest::do_SimpleEASY_BF_test_1() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

#ifdef ENABLE_BATSCHED
    std::string scheduling_algorithm = "easy_bf";
#else
    std::string scheduling_algorithm = "easy_bf_depth0";
#endif

    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
                                             {wrench::BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY, "0"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    ASSERT_NO_THROW(simulation->add(new SimpleEASY_BFTest_1_WMS(this, hostname)));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  SIMPLE EASY_BF TEST #2         (DEPTH=0)                        **/
/**********************************************************************/

class SimpleEASY_BFTest_2_WMS : public wrench::ExecutionController {

public:
    SimpleEASY_BFTest_2_WMS(BatchServiceEASY_BFTest *test,
                            const std::string& hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BatchServiceEASY_BFTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // |         44
        // |       3344
        // |       3344
        // |22222223344
        // |22222223344
        // |1111111111111111111111111111111111111111111
        //
        // And then see if job 4 gets postponed by a long 1-node job (job 5)
        std::set<std::shared_ptr<wrench::CompoundJob>> jobs;

        {
            auto job = job_manager->createCompoundJob("job1");
            int num_nodes = 1;
            int time = 6000;
            job->addSleepAction("sleep" + std::to_string(time), time);
            std::map<std::string, std::string> job_args = {{"-N", std::to_string(num_nodes)},
                                                           {"-t", std::to_string(time)},
                                                           {"-c", "10"}};
            job_manager->submitJob(job, this->test->compute_service, job_args);
            jobs.insert(job);
        }

        {
            auto job = job_manager->createCompoundJob("job2");
            int num_nodes = 2;
            int time = 70;
            job->addSleepAction("sleep" + std::to_string(time), time);
            std::map<std::string, std::string> job_args = {{"-N", std::to_string(num_nodes)},
                                                           {"-t", std::to_string(time)},
                                                           {"-c", "10"}};
            job_manager->submitJob(job, this->test->compute_service, job_args);
            jobs.insert(job);
        }

        {
            auto job = job_manager->createCompoundJob("job3");
            int num_nodes = 4;
            int time = 20;
            job->addSleepAction("sleep" + std::to_string(time), time);
            std::map<std::string, std::string> job_args = {{"-N", std::to_string(num_nodes)},
                                                           {"-t", std::to_string(time)},
                                                           {"-c", "10"}};
            job_manager->submitJob(job, this->test->compute_service, job_args);
            jobs.insert(job);
        }

        {
            auto job = job_manager->createCompoundJob("job4");
            int num_nodes = 5;
            int time = 20;
            job->addSleepAction("sleep" + std::to_string(time), time);
            std::map<std::string, std::string> job_args = {{"-N", std::to_string(num_nodes)},
                                                           {"-t", std::to_string(time)},
                                                           {"-c", "10"}};
            job_manager->submitJob(job, this->test->compute_service, job_args);
            jobs.insert(job);
        }

        {
            auto job = job_manager->createCompoundJob("job5");
            int num_nodes = 1;
            int time = 6000;
            job->addSleepAction("sleep" + std::to_string(time), time);
            std::map<std::string, std::string> job_args = {{"-N", std::to_string(num_nodes)},
                                                           {"-t", std::to_string(time)},
                                                           {"-c", "10"}};
            job_manager->submitJob(job, this->test->compute_service, job_args);
            jobs.insert(job);
        }


        std::map<std::string, double> completion_times;
        for (unsigned int i=0; i < jobs.size(); i++) {
            auto event = this->waitForNextEvent();
            if (auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
                completion_times[real_event->job->getName()] = wrench::Simulation::getCurrentSimulatedDate();
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        std::map<std::string, double> expected_completion_times = {
                {"job1", 6000},
                {"job2", 70},
                {"job3", 90},
                {"job4", 6020},
                {"job5", 6000},
        };

        for (auto const &item : completion_times) {
            std::cerr << item.first << ": " << item.second << std::endl;
            if (std::abs(item.second - expected_completion_times[item.first]) > 0.001) {
                throw std::runtime_error("Invalid job completion time for " + item.first + ": " +
                                         std::to_string(item.second) + "(should be " + std::to_string(expected_completion_times[item.first]) + ")");
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
//TEST_F(BatchServiceEASY_BFTest, DISABLED_SimpleEASY_BFTest_2)
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_2)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_2)
#endif
{
    DO_TEST_WITH_FORK(do_SimpleEASY_BF_test_2);
}

void BatchServiceEASY_BFTest::do_SimpleEASY_BF_test_2() {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

#ifdef ENABLE_BATSCHED
    std::string scheduling_algorithm = "easy_bf";
#else
    std::string scheduling_algorithm = "easy_bf_depth0";
#endif


    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4", "Host5", "Host6"}, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
                                             {wrench::BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY, "0"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    ASSERT_NO_THROW(simulation->add(new SimpleEASY_BFTest_2_WMS(this, hostname)));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  SIMPLE EASY_BF TEST #3               (DEPTH=0)                  **/
/**********************************************************************/

#define NUM_JOBS 4

class SimpleEASY_BFTest_LARGE_WMS : public wrench::ExecutionController {

public:
    SimpleEASY_BFTest_LARGE_WMS(BatchServiceEASY_BFTest *test,
                                const std::string& hostname) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
    }

private:
    BatchServiceEASY_BFTest *test;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        unsigned int random = this->test->_seed;

        std::shared_ptr<wrench::CompoundJob> jobs[NUM_JOBS];
        // Create 4 1-min tasks and submit them as various shaped jobs
        for (int i = 0; i < NUM_JOBS; i++) {
            random = random * 17 + 4123451;
            jobs[i] = job_manager->createCompoundJob("job" + std::to_string(i));
            int sleep_time = 60 + 60 * (random % 30);
            jobs[i]->addSleepAction("sleep" + std::to_string(sleep_time), sleep_time);
        }

        // Submit jobs
        try {
            for (auto & job : jobs) {
                std::map<std::string, std::string> job_specific_args;
                random = random * 17 + 4123451;
                job_specific_args["-N"] = std::to_string(1 + random % 4);
                random = random * 17 + 4123451;
                job_specific_args["-t"] = std::to_string(60 * (1 + random % 100));
                auto sleep_action = std::dynamic_pointer_cast<wrench::SleepAction>(*job->getActions().begin());
                job_specific_args["-t"] = std::to_string(sleep_action->getSleepTime());
                job_specific_args["-c"] = "10";
                job_manager->submitJob(job, this->test->compute_service, job_specific_args);
                wrench::Simulation::sleep(1);
            }
        } catch (wrench::ExecutionException &e) {
            throw std::runtime_error(
                    "Unexpected exception while submitting job");
        }

        std::map<std::string, std::pair<std::shared_ptr<wrench::CompoundJob>, double>> actual_completion_times;
        for (int i = 0; i < NUM_JOBS; i++) {
            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
                actual_completion_times[real_event->job->getName()] = std::make_pair(real_event->job, wrench::Simulation::getCurrentSimulatedDate());
            } else if (auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event)) {
                actual_completion_times[real_event->job->getName()] = std::make_pair(real_event->job,
                                                                                     wrench::Simulation::getCurrentSimulatedDate());
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        // Print Completion times:
#if 1
        std::cerr << "--------------\n";
        for (auto const &i : actual_completion_times) {
            auto job = i.second.first;
            double completion_time = i.second.second;
            std::shared_ptr<wrench::SleepAction> sleep_action = std::dynamic_pointer_cast<wrench::SleepAction>(*job->getActions().begin());
            std::cerr << "- " << i.first.c_str()  <<  ":" << "\t-N:" <<
                      job->getServiceSpecificArguments()["-N"].c_str() << " -t:"<<
                      job->getServiceSpecificArguments()["-t"].c_str() << " (real=" <<
                      sleep_action->getSleepTime() << ")   \tCT="<<
                      completion_time << "\n";
//            WRENCH_INFO("COMPLETION TIME %s (%s nodes, %s seconds): %lf",
//                        i.first.c_str(),
//                        job->getServiceSpecificArguments()["-N"].c_str(),
//                        job->getServiceSpecificArguments()["-t"].c_str(),
//                        completion_time);
        }
#endif


        return 0;
    }
};



#ifdef ENABLE_BATSCHED
//TEST_F(BatchServiceEASY_BFTest, DISABLED_SimpleEASY_BFTest_3)
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_3)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_3)
#endif
{
    for (int i=7; i < 8; i++) {
        std::cerr << "SEED = " << i << "\n";
        DO_TEST_WITH_FORK_ONE_ARG(do_SimpleEASY_BF_test_3, i);
    }
}

void BatchServiceEASY_BFTest::do_SimpleEASY_BF_test_3(int seed) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    this->_seed = seed;

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

#ifdef ENABLE_BATSCHED
    std::string scheduling_algorithm = "easy_bf";
#else
    std::string scheduling_algorithm = "easy_bf_depth0";
#endif


    // Create a Batch Service with a fcfs scheduling algorithm
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname, {"Host1", "Host2", "Host3", "Host4", "Host5", "Host6"}, "",
                                            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
                                             {wrench::BatchComputeServiceProperty::BATCH_RJMS_PADDING_DELAY, "0"},
                                             {wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"}})));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    ASSERT_NO_THROW(simulation->add(new SimpleEASY_BFTest_LARGE_WMS(this, hostname)));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

#endif