/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <cmath>
#include <gtest/gtest.h>
#include <wrench-dev.h>

#include <memory>
#include <utility>
#include <utility>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(batch_compute_service_host_reclaim_test,
                    "Log category for BatchComputeServiceHostReclaimTest test");

#define EPSILON 0.0001
#define HOUR 3600.0

std::vector<std::string> scheduling_algorithms = {"fcfs", "easy_bf_depth0", "easy_bf_depth1", "conservative_bf"};

class BatchComputeServiceHostReclaimTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::BatchComputeService> compute_service = nullptr;

    void do_BasicReclaim_test(std::string scheduling_algorithm);
    void do_KillTooBigJob_test(std::string scheduling_algorithm);
    void do_TwoSmallJobsBehindBigOne_test(std::string scheduling_algorithm);
    void do_TwoSmallJobsBehindMediumOne_test(std::string scheduling_algorithm);
    void do_BasicReclaimRelease_test(std::string scheduling_algorithm);
    void do_LessBasicReclaimRelease_test(std::string scheduling_algorithm);
    void do_EvenLessBasicReclaimRelease_test(std::string scheduling_algorithm);
    void do_RandomReclaimRelease_test(std::string scheduling_algorithm, int seed);

    std::shared_ptr<wrench::Workflow> workflow;

protected:
    ~BatchComputeServiceHostReclaimTest() override {
        wrench::Simulation::removeAllFiles();
    }

    BatchComputeServiceHostReclaimTest() {
        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
            "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
            "<platform version=\"4.1\"> "
            "   <zone id=\"AS0\" routing=\"Full\"> "
            "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
            "       </host>"
            "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
            "       </host>"
            "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
            "       </host>"
            "       <host id=\"Host4\" speed=\"1f\" core=\"10\">  "
            "       </host>  "
            "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
            "       <link id=\"2\" bandwidth=\"100MBps\" latency=\"0us\"/>"
            "       <route src=\"Host1\" dst=\"Host2\"> <link_ctn id=\"1\"/> </route>"
            "       <route src=\"Host2\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
            "       <route src=\"Host2\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
            "       <route src=\"Host3\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
            "       <route src=\"Host1\" dst=\"Host4\"> <link_ctn id=\"2\"/> </route>"
            "       <route src=\"Host1\" dst=\"Host3\"> <link_ctn id=\"2\"/> </route>"
            "   </zone> "
            "</platform>";
        FILE* platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  BASIC RECLAIM TEST                                              **/
/**********************************************************************/

class BatchSimpleReclaimTestWMS : public wrench::ExecutionController {
public:
    BatchSimpleReclaimTestWMS(BatchComputeServiceHostReclaimTest* test,
                              std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                              std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test),
                                                       batch_compute_service(std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Submit a compound job that uses all 4 hosts and submit it
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Reclaim Host1
        this->test->compute_service->reclaimHost("Host1");
        try {
            this->test->compute_service->reclaimHost("Host1");
            throw std::runtime_error("Shouldn't be able to reclaim an already-reclaimed host");
        }
        catch (const std::exception& ignore) {
        }

        // Wait for the workflow execution event
        auto event = this->waitForNextEvent();
        auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
        if (not real_event) {
            throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
        }

        // Check job state
        if (real_event->job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
            throw std::runtime_error("Unexpected job state: " + real_event->job->getStateAsString());
        }

        {
            // Re-submit a new job that tries to use the whole machine, which should fail
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            try {
                job_manager->submitJob(job, this->test->compute_service, service_specific_args);
                throw std::runtime_error("Should not be able to even submit a 4-node job");
            }
            catch (wrench::ExecutionException& e) {
                if (not std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause())) {
                    throw std::runtime_error("Should have gotten a 'not enough resources' failure cause");
                }
            }
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_BasicReclaim) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, BasicReclaim) {
#endif
    for (auto const& alg : scheduling_algorithms) {
        DO_TEST_WITH_FORK_ONE_ARG(do_BasicReclaim_test, alg);
    }
}

void BatchComputeServiceHostReclaimTest::do_BasicReclaim_test(std::string scheduling_algorithm) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {{wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm}})));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchSimpleReclaimTestWMS(this, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  BASIC RECLAIM WITH TOO BIG PENDING JOB TEST                     **/
/**********************************************************************/

class BatchReclaimFailTooBigJobTestWMS : public wrench::ExecutionController {
public:
    BatchReclaimFailTooBigJobTestWMS(BatchComputeServiceHostReclaimTest* test,
                                     std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                                     std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test),
                                                              batch_compute_service(std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Submit a compound job that uses all 4 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second compound job that uses all 4 hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Reclaim Host1
        this->test->compute_service->reclaimHost("Host1");
        try {
            this->test->compute_service->reclaimHost("Host1");
            throw std::runtime_error("Shouldn't be able to reclaim an already-reclaimed host");
        }
        catch (const std::exception& ignore) {
        }

        // At this point, the first job should fail because it was killed
        // AND the second job should fail because it's not too big

        {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Check job state
            if (real_event->job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
                throw std::runtime_error("Unexpected job state: " + real_event->job->getStateAsString());
            }

            // std::cerr << real_event->failure_cause->toString() << std::endl;
            // for (auto const& action : real_event->job->getActions()) {
            //     std::cerr << "   --> " << action->getFailureCause()->toString() << std::endl;
            // }
        }

        {
            // Wait for ANOTHER workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Check job state
            if (real_event->job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
                throw std::runtime_error("Unexpected job state: " + real_event->job->getStateAsString());
            }

            // std::cerr << real_event->failure_cause->toString() << std::endl;
            // for (auto const& action : real_event->job->getActions()) {
            //     std::cerr << "   --> " << action->getFailureCause()->toString() << std::endl;
            // }
        }


        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_KillTooBigJob) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, KillTooBigJob) {
#endif
    for (auto const& alg : scheduling_algorithms) {
        std::cout << "[ INFO     ] Testing with " << alg << std::endl;
        DO_TEST_WITH_FORK_ONE_ARG(do_KillTooBigJob_test, alg);
    }
}

void BatchComputeServiceHostReclaimTest::do_KillTooBigJob_test(std::string scheduling_algorithm) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {
            {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
            })));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchReclaimFailTooBigJobTestWMS(this, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  BASIC RECLAIM WITH TWO SMALL JOBS BEING A BIG ONE               **/
/**********************************************************************/

class BatchReclaimTwoSmallJobsBehindBigOneTestWMS : public wrench::ExecutionController {
public:
    BatchReclaimTwoSmallJobsBehindBigOneTestWMS(BatchComputeServiceHostReclaimTest* test,
                                                std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                                                std::string& hostname) : wrench::ExecutionController(hostname, "test"),
                                                                         test(test),
                                                                         batch_compute_service(
                                                                             std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Submit a compound job that uses all 4 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second compound job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second compound job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Reclaim Host1
        this->test->compute_service->reclaimHost("Host1");


        // At this point, the first job should fail because it was killed
        // AND other two jobs will not run in sequence

        {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Check job state
            if (real_event->job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
                throw std::runtime_error("Unexpected job state: " + real_event->job->getStateAsString());
            }
        }

        {
            // Wait for next TWO workflow execution event, which should be successes
            auto event1 = this->waitForNextEvent();
            auto real_event1 = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event1);
            auto event2 = this->waitForNextEvent();
            auto real_event2 = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event2);
            if (not real_event1) {
                throw std::runtime_error(
                    "Unexpected workflow execution event for first short job: " + event1->toString());
            }
            if (not real_event2) {
                throw std::runtime_error(
                    "Unexpected workflow execution event for second short job: " + event2->toString());
            }

            // Check job states
            if (real_event1->job->getState() != wrench::CompoundJob::State::COMPLETED) {
                throw std::runtime_error(
                    "Unexpected job state for first short job: " + real_event1->job->getStateAsString());
            }
            if (real_event2->job->getState() != wrench::CompoundJob::State::COMPLETED) {
                throw std::runtime_error(
                    "Unexpected job state for second short job: " + real_event2->job->getStateAsString());
            }

            // Check completion times
            if (fabs(real_event2->job->getEndDate() - real_event1->job->getEndDate() - 3600) > EPSILON) {
                throw std::runtime_error("Unexpected even dates for the short job completions " +
                    std::to_string(real_event1->job->getEndDate()) + " and " +
                    std::to_string(real_event2->job->getEndDate()));
            }
        }


        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_TwoSmallJobsBehindBigOne) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, TwoSmallJobsBehindBigOne) {
#endif
    for (auto const& alg : scheduling_algorithms) {
        std::cout << "[ INFO     ] Testing with " << alg << std::endl;
        DO_TEST_WITH_FORK_ONE_ARG(do_TwoSmallJobsBehindBigOne_test, alg);
    }
}

void BatchComputeServiceHostReclaimTest::do_TwoSmallJobsBehindBigOne_test(std::string scheduling_algorithm) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {
            {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
            })));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchReclaimTwoSmallJobsBehindBigOneTestWMS(this, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  BASIC RECLAIM WITH TWO SMALL JOBS BEING A MEDIUM ONE            **/
/**********************************************************************/

class BatchReclaimTwoSmallJobsBehindMediumOneTestWMS : public wrench::ExecutionController {
public:
    BatchReclaimTwoSmallJobsBehindMediumOneTestWMS(BatchComputeServiceHostReclaimTest* test,
                                                   std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                                                   std::string& hostname) : wrench::ExecutionController(
                                                                                hostname, "test"), test(test),
                                                                            batch_compute_service(
                                                                                std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Submit a compound job that uses  3 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "3";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second compound job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second compound job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Reclaim Host4
        // This is pretty "sketchy" in that we assume that the job will be running on
        // Host1, Host2, and Host 3.  But of course there is no guarantee, technically.
        this->test->compute_service->reclaimHost("Host4");

        // At this point, the first job should still be fine, and the other two jobs
        // should run in sequence

        {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Check job state
            if (real_event->job->getState() != wrench::CompoundJob::State::COMPLETED) {
                throw std::runtime_error("Unexpected job state: " + real_event->job->getStateAsString());
            }
        }

        {
            // Wait for next TWO workflow execution event, which should be successes
            auto event1 = this->waitForNextEvent();
            auto real_event1 = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event1);
            auto event2 = this->waitForNextEvent();
            auto real_event2 = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event2);
            if (not real_event1) {
                throw std::runtime_error(
                    "Unexpected workflow execution event for first short job: " + event1->toString());
            }
            if (not real_event2) {
                throw std::runtime_error(
                    "Unexpected workflow execution event for second short job: " + event2->toString());
            }

            // Check job states
            if (real_event1->job->getState() != wrench::CompoundJob::State::COMPLETED) {
                throw std::runtime_error(
                    "Unexpected job state for first short job: " + real_event1->job->getStateAsString());
            }
            if (real_event2->job->getState() != wrench::CompoundJob::State::COMPLETED) {
                throw std::runtime_error(
                    "Unexpected job state for second short job: " + real_event2->job->getStateAsString());
            }

            // Check completion times
            if (fabs(real_event2->job->getEndDate() - real_event1->job->getEndDate() - 3600) > EPSILON) {
                throw std::runtime_error("Unexpected even dates for the short job completions " +
                    std::to_string(real_event1->job->getEndDate()) + " and " +
                    std::to_string(real_event2->job->getEndDate()));
            }
        }


        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_TwoSmallJobsBehindMediumOne) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, TwoSmallJobsBehindMediumOne) {
#endif
    for (auto const& alg : scheduling_algorithms) {
        SCOPED_TRACE("Algorithm: " + alg);
        std::cout << "[ INFO     ] Testing with " << alg << std::endl;
        DO_TEST_WITH_FORK_ONE_ARG(do_TwoSmallJobsBehindMediumOne_test, alg);
    }
}

void BatchComputeServiceHostReclaimTest::do_TwoSmallJobsBehindMediumOne_test(std::string scheduling_algorithm) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {
            {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
            })));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchReclaimTwoSmallJobsBehindMediumOneTestWMS(this, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  BASIC RECLAIM/RETURN TEST                                       **/
/**********************************************************************/

class BatchBasicReclaimReleaseTestWMS : public wrench::ExecutionController {
public:
    BatchBasicReclaimReleaseTestWMS(BatchComputeServiceHostReclaimTest* test,
                                    std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                                    std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test),
                                                             batch_compute_service(std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Submit a compound job that uses  4 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }


        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Reclaim Host1
        this->test->compute_service->reclaimHost("Host1");

        // At this point, the first job has been killed
        {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Check job state
            if (real_event->job->getState() != wrench::CompoundJob::State::DISCONTINUED) {
                throw std::runtime_error("Unexpected job state: " + real_event->job->getStateAsString());
            }
        }

        // Try to submit a job, and make sure that fails
        {
            // Submit a compound job that uses  4 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            try {
                job_manager->submitJob(job, this->test->compute_service, service_specific_args);
                throw std::runtime_error("Should be able to submit a 4-node jobs after one node has been reclaimed");
            }
            catch (std::exception& ignore) {
            }
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Release Host2 (coverage)
        try {
            this->test->compute_service->releaseHost("Host2");
            throw std::runtime_error("Should be able to return Host2 since it hasn't been reclaimed");
        }
        catch (std::exception& ignore) {
        }

        // Release Host1
        this->test->compute_service->releaseHost("Host1");

        {
            // Submit a compound job that uses 4 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            // Check job state
            if (real_event->job->getState() != wrench::CompoundJob::State::COMPLETED) {
                throw std::runtime_error("Unexpected job state: " + real_event->job->getStateAsString());
            }

            if (fabs(wrench::Simulation::getCurrentSimulatedDate() - 3620) > EPSILON) {
                throw std::runtime_error(
                    "Unexpected job completion date: " + std::to_string(wrench::Simulation::getCurrentSimulatedDate()));
            }
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_ReclaimRelease) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, ReclaimRelease) {
#endif
    for (auto const& alg : scheduling_algorithms) {
        SCOPED_TRACE("Algorithm: " + alg);
        std::cout << "[ INFO     ] Testing with " << alg << std::endl;
        DO_TEST_WITH_FORK_ONE_ARG(do_BasicReclaimRelease_test, alg);
    }
}

void BatchComputeServiceHostReclaimTest::do_BasicReclaimRelease_test(std::string scheduling_algorithm) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 2;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {
            {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
            })));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchBasicReclaimReleaseTestWMS(this, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  LESS BASIC RECLAIM/RETURN TEST                                  **/
/**********************************************************************/

class BatchLessBasicReclaimReleaseTestWMS : public wrench::ExecutionController {
public:
    BatchLessBasicReclaimReleaseTestWMS(BatchComputeServiceHostReclaimTest* test,
                                        std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                                        std::string& hostname) : wrench::ExecutionController(hostname, "test"),
                                                                 test(test),
                                                                 batch_compute_service(
                                                                     std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Submit a compound job that uses all 4 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "4";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second compound job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second compound job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("");
            auto action = job->addSleepAction("", 3600.0);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = "3600";
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        // Sleep for 10 seconds
        wrench::Simulation::sleep(10);

        // Reclaim Host1
        this->test->compute_service->reclaimHost("Host1");

        // At this point, the first job has been killed, and the second job should start immediately
        {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        wrench::Simulation::sleep(10);

        // Release Host1
        this->test->compute_service->releaseHost("Host1");

        // At this point, the third job should start as well
        {
            // Wait for the workflow execution event
            auto event1 = this->waitForNextEvent();
            auto real_event1 = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event1);
            if (not real_event1) {
                throw std::runtime_error("Unexpected workflow execution event: " + event1->toString());
            }

            // Wait for the workflow execution event
            auto event2 = this->waitForNextEvent();
            auto real_event2 = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event2);
            if (not real_event2) {
                throw std::runtime_error("Unexpected workflow execution event: " + event2->toString());
            }

            if (fabs(real_event1->job->getEndDate() - 3610) > EPSILON) {
                throw std::runtime_error(
                    "Unexpected job completion date for job 1: " + std::to_string(
                        wrench::Simulation::getCurrentSimulatedDate()));
            }

            if (fabs(real_event2->job->getEndDate() - 3620) > EPSILON) {
                throw std::runtime_error(
                    "Unexpected job completion date for job 2: " + std::to_string(
                        wrench::Simulation::getCurrentSimulatedDate()));
            }
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_ReclaimRelease) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, LessBasicReclaimRelease) {
#endif
    for (auto const& alg : scheduling_algorithms) {
        SCOPED_TRACE("Algorithm: " + alg);
        std::cout << "[ INFO     ] Testing with " << alg << std::endl;
        DO_TEST_WITH_FORK_ONE_ARG(do_LessBasicReclaimRelease_test, alg);
    }
}

void BatchComputeServiceHostReclaimTest::do_LessBasicReclaimRelease_test(std::string scheduling_algorithm) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {
            {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
            })));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchLessBasicReclaimReleaseTestWMS(this, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  EVEN LESS BASIC RECLAIM/RETURN TEST                             **/
/**********************************************************************/


class BatchEvenLessBasicReclaimReleaseTestWMS : public wrench::ExecutionController {
public:
    BatchEvenLessBasicReclaimReleaseTestWMS(BatchComputeServiceHostReclaimTest* test,
                                            std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                                            std::string& hostname) : wrench::ExecutionController(hostname, "test"),
                                                                     test(test),
                                                                     batch_compute_service(
                                                                         std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Submit a  job that uses  3 hosts and submit it (this one will start)
            auto job = job_manager->createCompoundJob("A");
            double sleep_time = 3 * HOUR;
            auto action = job->addSleepAction("", sleep_time);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "3";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = std::to_string(sleep_time);
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a second  job that uses only one host and submit it (this one will start
            auto job = job_manager->createCompoundJob("B");
            double sleep_time = 10 * HOUR;
            auto action = job->addSleepAction("", sleep_time);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "1";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = std::to_string(sleep_time);
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a third  job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("C");
            double sleep_time = 3 * HOUR;
            auto action = job->addSleepAction("", sleep_time);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = std::to_string(sleep_time);
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        {
            // Submit a fourth  job that uses only two hosts and submit it (this one will be waiting)
            auto job = job_manager->createCompoundJob("D");
            double sleep_time = 4 * HOUR;
            auto action = job->addSleepAction("", sleep_time);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = "2";
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = std::to_string(sleep_time);
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }


        // Sleep for 1 hour
        wrench::Simulation::sleep(1 * HOUR);

        // Reclaim Host1
        this->test->compute_service->reclaimHost("Host1"); // Should kill Job "A"

        // At this point, the first job has been killed, and the second job should start immediately
        {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobFailedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
            if (real_event->job->getName() != "A") {
                throw std::runtime_error("Wrong job was killed!");
            }
        }

        // Sleep for 1 hour
        wrench::Simulation::sleep(1 * HOUR);

        // Release Host1
        this->test->compute_service->releaseHost("Host1");

        // At this point, we will get three completions in a row
        std::vector<std::pair<std::string, double>> expected_events = {{"C", 4}, {"D", 8}, {"B", 10}};
        for (int i = 0; i < 3; i++) {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
            auto real_event = std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event);
            if (not real_event) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }

            if (real_event->job->getName() != expected_events[i].first) {
                throw std::runtime_error(
                    "Unexpected job in event #" + std::to_string(i) + ": " + real_event->job->getName());
            }
            if (fabs(real_event->job->getEndDate() - expected_events[i].second * HOUR) > EPSILON) {
                throw std::runtime_error(
                    "Unexpected job completion date for job " + real_event->job->getName() + ": " + std::to_string(
                        wrench::Simulation::getCurrentSimulatedDate() / HOUR));
            }
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_ReclaimRelease) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, EvenLessBasicReclaimRelease) {
#endif
    for (auto const& alg : scheduling_algorithms) {
        SCOPED_TRACE("Algorithm: " + alg);
        std::cout << "[ INFO     ] Testing with " << alg << std::endl;
        DO_TEST_WITH_FORK_ONE_ARG(do_EvenLessBasicReclaimRelease_test, alg);
    }
}

void BatchComputeServiceHostReclaimTest::do_EvenLessBasicReclaimRelease_test(std::string scheduling_algorithm) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {
            {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
            })));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchEvenLessBasicReclaimReleaseTestWMS(this, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  RANDOM RECLAIM/RETURN TEST                                      **/
/**********************************************************************/


class BatchRandomReclaimReleaseTestWMS : public wrench::ExecutionController {
public:
    BatchRandomReclaimReleaseTestWMS(BatchComputeServiceHostReclaimTest* test,
                                     int seed,
                                     std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                                     std::string& hostname) : wrench::ExecutionController(hostname, "test"), test(test),
                                                              seed(seed),
                                                              batch_compute_service(std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceHostReclaimTest* test;
    int seed;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {
        // Create a job manager
        auto job_manager = this->createJobManager();

        srand(this->seed);

        int num_jobs = 200;
        int num_reclaims_releases = 30;

        for (int i = 0; i < num_jobs; i++) {
            // Submit a  job
            auto job = job_manager->createCompoundJob("J" + std::to_string(i));
            double sleep_time = 5 + rand() % (10 * 3600);
            auto action = job->addSleepAction("", sleep_time);

            std::map<std::string, std::string> service_specific_args;
            service_specific_args["-N"] = std::to_string(1 + rand() % 4);
            service_specific_args["-c"] = "10";
            service_specific_args["-t"] = std::to_string(sleep_time);
            job_manager->submitJob(job, this->test->compute_service, service_specific_args);
        }

        // Do a few random reclaims/return
        for (int i = 0; i < num_reclaims_releases; i++) {
            wrench::Simulation::sleep(100 + (rand() % 3600));
            auto host = "Host" + std::to_string(1 + rand() % 4);
            this->test->compute_service->reclaimHost(host);
            wrench::Simulation::sleep(2 * 3600 + (rand() % 10 * 3600));
            this->test->compute_service->releaseHost(host);
        }

        for (int i = 0; i < num_jobs; i++) {
            // Wait for the workflow execution event
            auto event = this->waitForNextEvent();
        }

        // Stop the Job Manager manually, just for kicks
        job_manager->stop();

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchComputeServiceHostReclaimTest, DISABLED_ReclaimRelease) {
#else
TEST_F(BatchComputeServiceHostReclaimTest, RandomReclaimRelease) {
#endif
    // std::vector<std::string> scheduling_algorithms = {"conservative_bf"};
    for (auto const& alg : scheduling_algorithms) {
        std::cout << "[ INFO     ] Testing with " << alg << std::endl;
        for (int seed = 0; seed < 20; seed++) {
            SCOPED_TRACE("Algorithm: " + alg + ", seed = " + std::to_string(seed));
            DO_TEST_WITH_FORK_TWO_ARGS(do_RandomReclaimRelease_test, alg, seed);
        }
    }
}

void BatchComputeServiceHostReclaimTest::do_RandomReclaimRelease_test(std::string scheduling_algorithm, int seed) {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 1;
    auto argv = (char**)calloc(argc, sizeof(char*));
    argv[0] = strdup("batch_host_reclaim_test");
    // argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));
    ASSERT_THROW(simulation->instantiatePlatform(platform_file_path), std::runtime_error);

    ASSERT_THROW(simulation->add((wrench::ComputeService *) nullptr), std::invalid_argument);

    // Create a Compute Service
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    ASSERT_NO_THROW(compute_service = simulation->add(
        new wrench::BatchComputeService("Host1",
            {"Host1", "Host2", "Host3", "Host4"},
            "",
            {
            {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, scheduling_algorithm},
            })));

    // Create a Controller
    ASSERT_THROW(simulation->launch(), std::runtime_error);
    std::shared_ptr<wrench::ExecutionController> wms = nullptr;
    std::string hostname = "Host1";
    ASSERT_NO_THROW(wms = simulation->add(
        new BatchRandomReclaimReleaseTestWMS(this, seed, compute_service, hostname)));

    simulation->add(new wrench::FileRegistryService(hostname));

    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
