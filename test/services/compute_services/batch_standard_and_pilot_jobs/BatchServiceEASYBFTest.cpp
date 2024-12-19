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

    void do_EASY_BF_test(int num_compute_nodes,
                         std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec,
                         bool print_completion_times);

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
                    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> &spec,
                    bool print_completion_times) : wrench::ExecutionController(hostname, "test") {
        this->test = test;
        this->spec = spec;
        this->print_completion_times = print_completion_times;
    }

private:
    BatchServiceEASY_BFTest *test;
    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec;
    bool print_completion_times;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::vector<std::shared_ptr<wrench::CompoundJob>> jobs;
        std::map<std::string, std::tuple<double, std::string, std::string, double, double>> completion_times;
        std::map<std::string, double> expected_completion_times;

        // Create and submit all jobs
        for (auto const &job_spec : spec) {
            std::string job_name =std::get<0>(job_spec);
            auto num_nodes = std::get<1>(job_spec);
            auto duration = std::get<2>(job_spec);
            auto sleep_after = std::get<3>(job_spec);
            expected_completion_times[job_name] = std::get<4>(job_spec);

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
                auto job = real_event->job;
                auto sleep_action = *(job->getActions().begin());
                completion_times[real_event->job->getName()] =
                        std::make_tuple(job->getSubmitDate(),
                                        job->getServiceSpecificArguments()["-N"],
                                        job->getServiceSpecificArguments()["-t"],
                                        sleep_action->getStartDate(),
                                        wrench::Simulation::getCurrentSimulatedDate());
            } else {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        if (print_completion_times) {
            for (auto const &item: completion_times) {
                std::cerr << "  " << item.first <<
                          " (arr=" << std::get<0>(item.second) <<
                          ", N=" << std::get<1>(item.second) <<
                          ", t=" << std::get<2>(item.second) << "): " <<
                          std::get<3>(item.second) << " -> " <<
                          std::get<4>(item.second) << std::endl;
            }
        }
        for (auto const &item: completion_times) {
            if ((expected_completion_times[item.first] > 0) and (std::abs(std::get<4>(item.second) - expected_completion_times[item.first]) > 0.001)) {
                throw std::runtime_error("Invalid job completion time for " + item.first + ": " +
                                         std::to_string(std::get<3>(item.second)) + " (expected: " +
                                         std::to_string(expected_completion_times[item.first]) + ")");
            }
        }

        return 0;
    }
};

void BatchServiceEASY_BFTest::do_EASY_BF_test(int num_compute_nodes,
                                              std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec,
                                              bool print_completion_times) {

    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();
    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
    argv[1] = strdup("--wrench-commport-pool-size=50000");
//    argv[2] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

#ifdef ENABLE_BATSCHED
    std::string scheduling_algorithm = "easy_bf";
#else
    std::string scheduling_algorithm = "easy_bf_depth1";
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
    ASSERT_NO_THROW(simulation->add(new EASY_BFTest_WMS(this, hostname, spec, print_completion_times)));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, DISABLED_SimpleEASY_BFTest_1)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_1)
#endif
{
    // job_name, num_nodes, duration, sleep_after, expected CT
    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec = {
            {"job1", 2, 60, 0, 60},
            {"job2", 4, 30, 0, 90},
            {"job3", 2, 30, 0, 30},
            {"job4", 2, 50, 0, 140}
    };

    DO_TEST_WITH_FORK_THREE_ARGS(do_EASY_BF_test, 4, spec, false);
}


#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, DISABLED_SimpleEASY_BFTest_2)
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

    DO_TEST_WITH_FORK_THREE_ARGS(do_EASY_BF_test, 6, spec, false);
}

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, DISABLED_SimpleEASY_BFTest_3)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_3)
#endif
{
    // job_name, num_nodes, duration, sleep_after, expected CT
    std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec = {
            {"job1", 3, 660, 1, 660},
            {"job2", 1, 120, 1, 121},
            {"job3", 3, 1740, 1, 121 + 1740},
            {"job4", 1, 1080, 1, 660 + 1080},
    };

    DO_TEST_WITH_FORK_THREE_ARGS(do_EASY_BF_test, 6, spec, false);
}

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_RANDOM)
#else
TEST_F(BatchServiceEASY_BFTest, SimpleEASY_BFTest_RANDOM)
#endif
{
    int num_jobs = 1000;
    for (int seed = 0; seed < 10; seed++) {
        std::vector<std::tuple<std::string, unsigned int, unsigned int, unsigned int, int>> spec;
//        std::cerr << "SEED= " << seed << "\n";
        unsigned int random = seed;
        for (int i = 1; i <= num_jobs; i++) {
            std::string job_name = "job" + std::to_string(i);
            random = random * 17 + 4123451;
            unsigned int num_nodes = 1 + random % 4;
            random = random * 17 + 4123451;
            unsigned int duration = 60 + 60 * (random % 30);
            int expected_ct = -1;
            spec.emplace_back(job_name, num_nodes, duration, 0, expected_ct);
        }
        DO_TEST_WITH_FORK_THREE_ARGS(do_EASY_BF_test, 6, spec, false);
    }
}


