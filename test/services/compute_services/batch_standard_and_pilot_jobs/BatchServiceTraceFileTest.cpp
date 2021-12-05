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
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/util/TraceFileLoader.h>
#include <wrench/job/PilotJob.h>
#include <unistd.h>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(batch_service_trace_file_test, "Log category for BatchServiceTest");


class BatchServiceTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;
    wrench::Simulation *simulation;

    void do_BatchTraceFileReplayTest_test();

    void do_WorkloadTraceFileTestSWF_test();
    void do_WorkloadTraceFileTestSWFBatchServiceShutdown_test();
    void do_WorkloadTraceFileRequestedTimesSWF_test();
    void do_WorkloadTraceFileDifferentTimeOriginSWF_test();
    void do_BatchTraceFileReplayTestWithFailedJob_test();
    void do_WorkloadTraceFileTestJSON_test();
    void do_GetQueueState_test();


protected:
    BatchServiceTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "           <prop id=\"ram\" value=\"2048B\"/> "
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "           <prop id=\"ram\" value=\"2048B\"/> "
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "           <prop id=\"ram\" value=\"2048B\"/> "
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"> "
                          "           <prop id=\"ram\" value=\"2048B\"/> "
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <link id=\"3\" bandwidth=\"50000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"1\"/> </route>"
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
/**  BATCH TRACE FILE REPLAY SIMULATION TEST **/
/**********************************************************************/

class BatchTraceFileReplayTestWMS : public wrench::WMS {

public:
    BatchTraceFileReplayTestWMS(BatchServiceTest *test,
                                const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services,
                        {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        // Submit the jobs
        unsigned long num_submitted_jobs = 0;
        {
            std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>> trace_file_jobs;
            //Let's load the trace file
            try {
                trace_file_jobs = wrench::TraceFileLoader::loadFromTraceFile("../test/trace_files/NASA-iPSC-1993-3.swf", false, 0);
            } catch (std::invalid_argument &e) {
                // Ignore and try alternate path
                trace_file_jobs = wrench::TraceFileLoader::loadFromTraceFile("test/trace_files/NASA-iPSC-1993-3.swf", false, 0);
                // If this doesn't work, then we have a problem, so we simply let the exception be uncaught
            }


            int counter = 0;
            for (auto const &job : trace_file_jobs) {
                double sub_time = std::get<1>(job);
                double curtime = wrench::S4U_Simulation::getClock();
                double sleeptime = sub_time - curtime;
                if (sleeptime > 0)
                    wrench::S4U_Simulation::sleep(sleeptime);
                std::string username = std::get<0>(job);
                double flops = std::get<2>(job);
                double requested_flops = std::get<3>(job);
                double requested_ram = std::get<4>(job);
                int num_nodes = std::get<5>(job);
                int min_num_cores = 10;
                int max_num_cores = 10;
                double ram = 0.0;
                // Ignore jobs that are too big
                if (num_nodes > 4) {
                    continue;
                }
//          std::cerr << "SUBMITTING " << "sub="<< sub_time << "num_nodes=" << num_nodes << " id="<<id << " flops="<<flops << " rflops="<<requested_flops << " ram="<<requested_ram << "\n";
                // TODO: Should we use the "requested_ram" instead of 0 below?
                auto task = this->getWorkflow()->addTask(username + "_" + std::to_string(counter++), flops, min_num_cores, max_num_cores, ram);

                auto standard_job = job_manager->createStandardJob(task);

                std::map<std::string, std::string> batch_job_args;
                batch_job_args["-N"] = std::to_string(num_nodes);
                batch_job_args["-t"] = std::to_string(requested_flops);
                batch_job_args["-c"] = std::to_string(max_num_cores); //use all cores
                try {
                    job_manager->submitJob(standard_job, this->test->compute_service, batch_job_args);
                } catch (wrench::ExecutionException &e) {
                    throw std::runtime_error("Failed to submit a job");
                }

                num_submitted_jobs++;

            }
        }

        // Wait for the execution events
        for (unsigned long i=0; i < num_submitted_jobs; i++) {
            // Wait for a workflow execution event
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
            } catch (wrench::ExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, BatchTraceFileReplayTest) {
    DO_TEST_WITH_FORK(do_BatchTraceFileReplayTest_test);
}

void BatchServiceTest::do_BatchTraceFileReplayTest_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, {})));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BatchTraceFileReplayTestWMS(
                    this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  BATCH TRACE FILE REPLAY SIMULATION TEST: FAILED JOB             **/
/**********************************************************************/

class BatchTraceFileReplayTestWithFailedJobWMS : public wrench::WMS {

public:
    BatchTraceFileReplayTestWithFailedJobWMS(BatchServiceTest *test,
                                             const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                             const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services,
                        {}, nullptr, hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {

        this->simulation->sleep(10 * 3600);

        return 0;
    }
};

TEST_F(BatchServiceTest, BatchTraceFileReplayTestWithFailedJob) {
    DO_TEST_WITH_FORK(do_BatchTraceFileReplayTestWithFailedJob_test);
}

void BatchServiceTest::do_BatchTraceFileReplayTestWithFailedJob_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Valid trace file
    std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "swf_trace.swf";

    FILE *trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600 -1\n");  // job that takes the whole machine
    fprintf(trace_file, "2 1 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fprintf(trace_file, "3 10000 -1 3600 -1 -1 -1 100 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);


    // Create a Batch Service with a the valid trace file
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            )));


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(
            new BatchTraceFileReplayTestWithFailedJobWMS(
                    this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  WORKLOAD TRACE FILE TEST SWF BATCH SERVICE SHUTDOWN             **/
/**********************************************************************/

class WorkloadTraceFileSWFBatchServiceShutdownTestWMS : public wrench::WMS {

public:
    WorkloadTraceFileSWFBatchServiceShutdownTestWMS(BatchServiceTest *test,
                                                    const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                    const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        wrench::Simulation::sleep(20);

        // Shutdown the batch_standard_and_pilot_jobs service
        (*(this->getAvailableComputeServices<wrench::ComputeService>().begin()))->stop();

        // At this point there should be some warning messages from the
        // Workload Trace file replayer (job failures, impossible to submit the last job)

        wrench::Simulation::sleep(1000);
        return 0;
    }
};

TEST_F(BatchServiceTest, WorkloadTraceFileSWFBatchServiceShutdownTest) {
    DO_TEST_WITH_FORK(do_WorkloadTraceFileTestSWFBatchServiceShutdown_test);
}

void BatchServiceTest::do_WorkloadTraceFileTestSWFBatchServiceShutdown_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";


    std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "swf_trace.swf";
    FILE *trace_file;


    // Create a Valid trace file (not the "wrong" estimates, which are ignored due to
    // passing the correct option to the BatchComputeService constructor
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 5600 -1\n");  // job that takes the whole machine
    fprintf(trace_file, "2 1 -1 3600 -1 -1 -1 2 8666 -1\n");  // job that takes half the machine (and times out, for coverage)
    fprintf(trace_file, "3 200 -1 3600 -1 -1 -1 2 500 100000\n");  // for TraceFileLoader coverage
    fclose(trace_file);


    // Create a Batch Service with a the valid trace file
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {
                                                    {wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                                    {wrench::BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE, "true"}
                                            }
            )));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new WorkloadTraceFileSWFBatchServiceShutdownTestWMS(
            this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

/**********************************************************************/
/**  WORKLOAD TRACE FILE TEST SWF                                    **/
/**********************************************************************/

class WorkloadTraceFileSWFTestWMS : public wrench::WMS {

public:
    WorkloadTraceFileSWFTestWMS(BatchServiceTest *test,
                                const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        wrench::Simulation::sleep(10);
        // At this point, using the fcfs algorithm, a 2-node 30-min job should complete around t=1.5 hours
        // and 4-node 30-min job should complete around t=2.5 hours

        std::vector<wrench::WorkflowTask *> tasks;
        std::map<std::string, std::string> batch_job_args;

        // Create and submit a job that needs 2 nodes and 30 minutes
        for (size_t i = 0; i < 2; i++) {
            double time_fudge = 1; // 1 second seems to make it all work!
            double task_flops = 10 * (1 * (1800 - time_fudge));
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->getWorkflow()->addTask("test_job_1_task_" + std::to_string(i),
                                                         task_flops,
                                                         num_cores, num_cores, ram));
        }

        // Create a Standard Job with only the tasks
        auto standard_job_2_nodes = job_manager->createStandardJob(tasks);

        // Create the batch_standard_and_pilot_jobs-specific argument
        batch_job_args["-N"] = std::to_string(2); // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(1800); // Time in minutes (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10); //number of cores per task1

        // Submit this job to the batch_standard_and_pilot_jobs service
        job_manager->submitJob(standard_job_2_nodes, *(this->getAvailableComputeServices<wrench::ComputeService>().begin()), batch_job_args);


        // Create and submit a job that needs 2 nodes and 30 minutes
        tasks.clear();
        for (size_t i = 0; i < 4; i++) {
            double time_fudge = 1; // 1 second seems to make it all work!
            double task_flops = 10 * (1 * (1800 - time_fudge));
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->getWorkflow()->addTask("test_job_2_task_" + std::to_string(i),
                                                         task_flops,
                                                         num_cores, num_cores, ram));
        }

        // Create a Standard Job with only the tasks
        auto standard_job_4_nodes = job_manager->createStandardJob(tasks);

        // Create the batch_standard_and_pilot_jobs-specific argument
        batch_job_args.clear();
        batch_job_args["-N"] = std::to_string(4); // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(1800); // Time in minutes (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10); //number of cores per task1

        // Submit this job to the batch_standard_and_pilot_jobs service
        job_manager->submitJob(standard_job_4_nodes, *(this->getAvailableComputeServices<wrench::ComputeService>().begin()), batch_job_args);


        // Wait for the two execution events
        for (auto job : {standard_job_2_nodes, standard_job_4_nodes}) {
            // Wait for the workflow execution event
            WRENCH_INFO("Waiting for job completion of job %s", job->getName().c_str());
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
                auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);
                if (real_event) {
                    if (real_event->standard_job != job) {
                        throw std::runtime_error("Wrong job completion order: got " +
                                                 real_event->standard_job->getName() + " but expected " + job->getName());
                    }
                } else {
                    throw std::runtime_error(
                            "Unexpected workflow execution event: " + event->toString());
                }
            } catch (wrench::ExecutionException &e) {
                //ignore (network error or something)
            }

            double completion_time = wrench::Simulation::getCurrentSimulatedDate();
            double expected_completion_time;
            if (job == standard_job_2_nodes) {
                expected_completion_time = 3600  + 1800;
            } else if (job == standard_job_4_nodes) {
                expected_completion_time = 3600 * 2 + 1800;
            } else {
                throw std::runtime_error("Phantom job completion!");
            }
            double delta = std::abs(expected_completion_time - completion_time);
            double tolerance = 5;
            if (delta > tolerance) {
                throw std::runtime_error("Unexpected job completion time for job " + job->getName() + ": " +
                                         std::to_string(completion_time) + " (expected: " + std::to_string(expected_completion_time) + ")");
            }

        }
        return 0;
    }
};

TEST_F(BatchServiceTest, WorkloadTraceFileSWFTest) {
    DO_TEST_WITH_FORK(do_WorkloadTraceFileTestSWF_test);
}

void BatchServiceTest::do_WorkloadTraceFileTestSWF_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a trace file with no extension, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/not_there_invalid"}}
            ), std::invalid_argument);

    // Create a Batch Service with a trace file with a bogus extension, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/not_there.invalid"}}
            ), std::invalid_argument);

    // Create a Batch Service with a non-existing workload trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/not_there.swf"}}
            ), std::invalid_argument);

    std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "swf_trace.swf";
    FILE *trace_file;

    // Create an invalid trace file
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600\n");     // MISSING FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);


    // Create an invalid trace file (SAME)
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600\n");     // MISSING FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_NO_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                             {wrench::BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE, "true"}}
            ));


    // Create another invalid trace file
    trace_file  = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 bogus -1\n");     // INVALID FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file  = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 bogus -1 3600 -1 -1 -1 4 3600 -1\n");     // INVALID FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file  = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 bogus -1 -1 -1 4 3600 -1\n");     // INVALID FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file  = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 bogus -1 -1 4 3600 -1\n");     // INVALID FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file  = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 3600 bogus\n");     // INVALID FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file  = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 bogus 3600 -1\n");     // INVALID FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file  = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 bogus -1\n");     // INVALID FIELD
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 -1 3600 -1\n");     // MISSING NUM PROCS
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 -1 -1 -1 -1 4 -1 -1\n");     // MISSING TIME
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 -1 -1 -1 -1 4 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 - 1\n");     // TOO MANY COLUMNS
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    // Create another invalid trace file
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 -1 -1 3600 -1 -1 -1 4 5600 -1\n");     // NEGATIVE submit time
    fprintf(trace_file, "2 0 -1 3600 -1 -1 -1 2 3600 -1\n");  // job that takes half the machine
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);


    // Create a Valid trace file (not the "wrong" estimates, which are ignored due to
    // passing the correct option to the BatchComputeService constructor
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 5600 -1\n");  // job that takes the whole machine
    fprintf(trace_file, "2 1 -1 3600 -1 -1 -1 2 8666 -1\n");  // job that takes half the machine (and times out, for coverage)
    fclose(trace_file);


    // Create a Batch Service with a the valid trace file
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {
                                                    {wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                                    {wrench::BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE, "true"}
                                            }
            )));


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new WorkloadTraceFileSWFTestWMS(
            this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());


    // Access task1 completion task1 stamps
    auto trace = simulation->getOutput().getTrace<wrench::SimulationTimestampTaskCompletion>();
    for (auto const &ts : trace) {
        auto task = ts->getContent()->getTask();
        auto host = task->getExecutionHost();
        auto workflow = task->getWorkflow();
        auto num_cores = task->getNumCoresAllocated();
        WRENCH_INFO("%s: %s %lu", task->getID().c_str(), host.c_str(), num_cores);
    }

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}







/**********************************************************************/
/**  WORKLOAD TRACE FILE TEST SWF: REQUESTED != ACTUAL               **/
/**********************************************************************/

class WorkloadTraceFileSWFRequestedTimesTestWMS : public wrench::WMS {

public:
    WorkloadTraceFileSWFRequestedTimesTestWMS(BatchServiceTest *test,
                                              const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                              const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                              std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        wrench::Simulation::sleep(10);

        std::vector<wrench::WorkflowTask *> tasks;
        std::map<std::string, std::string> batch_job_args;

        // Create and submit a job that needs 2 nodes and 10 minutes
        for (size_t i = 0; i < 2; i++) {
            double time_fudge = 1; // 1 second seems to make it all work!
            double task_flops = 10 * (1 * (600 - time_fudge));
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->getWorkflow()->addTask("test_job_1_task_" + std::to_string(i),
                                                         task_flops,
                                                         num_cores, num_cores, ram));
        }

        // Create a Standard Job with only the tasks
        auto standard_job_2_nodes = job_manager->createStandardJob(tasks);

        // Create the batch_standard_and_pilot_jobs-specific argument
        batch_job_args["-N"] = std::to_string(2);     // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(10);  // Time in minutes (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10);  //number of cores per task1

        // Submit this job to the batch_standard_and_pilot_jobs service
        job_manager->submitJob(standard_job_2_nodes, *(this->getAvailableComputeServices<wrench::ComputeService>().begin()), batch_job_args);


        // Wait for the two execution events
        for (auto job : {standard_job_2_nodes}) {
            // Wait for the workflow execution event
            WRENCH_INFO("Waiting for job completion of job %s", job->getName().c_str());
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
                auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);
                if (real_event) {
                    if (real_event->standard_job != job) {
                        throw std::runtime_error("Wrong job completion order: got " +
                                                 real_event->standard_job->getName() + " but expected " + job->getName());
                    }
                } else {
                    throw std::runtime_error(
                            "Unexpected workflow execution event: " + event->toString());
                }
            } catch (wrench::ExecutionException &e) {
                //ignore (network error or something)
            }

            double completion_time = wrench::Simulation::getCurrentSimulatedDate();
            double expected_completion_time;
            if (job == standard_job_2_nodes) {
                expected_completion_time = 100 + 600;
            } else {
                throw std::runtime_error("Phantom job completion!");
            }
            double delta = fabs(expected_completion_time - completion_time);
            double tolerance = 5;
            if (delta > tolerance) {
                throw std::runtime_error("Unexpected job completion time for job " + job->getName() + ": " +
                                         std::to_string(completion_time) + " (expected: " + std::to_string(expected_completion_time) + ")");
            }

        }
        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, WorkloadTraceFileSWFRequestedTimesTest) {
#else
TEST_F(BatchServiceTest, DISABLED_WorkloadTraceFileSWFRequestedTimesTest) {
#endif
    DO_TEST_WITH_FORK(do_WorkloadTraceFileRequestedTimesSWF_test);
}



void BatchServiceTest::do_WorkloadTraceFileRequestedTimesSWF_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Valid trace file
    std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "swf_trace.swf";
    FILE *trace_file;
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 100 -1 -1 -1 2 1800 -1\n");
    fprintf(trace_file, "2 1 -1 1800 -1 -1 -1 2 1800 -1\n");
    fprintf(trace_file, "3 2 -1 3600 -1 -1 -1 4 3600 -1\n");
    fclose(trace_file);

    // Create a Batch Service with a the valid trace file
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "easy_bf"},
                                                    {wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                                    {wrench::BatchComputeServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP, "true"},
                                                    {wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"},
                                                    {wrench::BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE, "false"},
                                            }
            )));


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new WorkloadTraceFileSWFRequestedTimesTestWMS(
            this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}




/**********************************************************************/
/**  SWF DIFFERENT TIME ORIGIN TEST               **/
/**********************************************************************/

class WorkloadTraceFileSWFDifferentTimeOriginTestWMS : public wrench::WMS {

public:
    WorkloadTraceFileSWFDifferentTimeOriginTestWMS(BatchServiceTest *test,
                                                   const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                   const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                   std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        wrench::Simulation::sleep(10);

        std::vector<wrench::WorkflowTask *> tasks;
        std::map<std::string, std::string> batch_job_args;

        // Create and submit a job that needs 2 nodes and 10 minutes
        for (size_t i = 0; i < 2; i++) {
            double time_fudge = 1; // 1 second seems to make it all work!
            double task_flops = 10 * (1 * (600 - time_fudge));
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->getWorkflow()->addTask("test_job_1_task_" + std::to_string(i),
                                                         task_flops,
                                                         num_cores, num_cores, ram));
        }

        // Create a Standard Job with only the tasks
        auto standard_job_2_nodes = job_manager->createStandardJob(tasks);

        // Create the batch_standard_and_pilot_jobs-specific argument
        batch_job_args["-N"] = std::to_string(2);     // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(10);  // Time in minutes (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10);  //number of cores per task1

        // Submit this job to the batch_standard_and_pilot_jobs service
        job_manager->submitJob(standard_job_2_nodes, *(this->getAvailableComputeServices<wrench::ComputeService>().begin()), batch_job_args);


        // Wait for the two execution events
        for (auto job : {standard_job_2_nodes}) {
            // Wait for the workflow execution event
            WRENCH_INFO("Waiting for job completion of job %s", job->getName().c_str());
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
                auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);
                if (real_event) {
                    if (real_event->standard_job != job) {
                        throw std::runtime_error("Wrong job completion order: got " +
                                                 real_event->standard_job->getName() + " but expected " + job->getName());
                    }
                } else {
                    throw std::runtime_error(
                            "Unexpected workflow execution event: " + event->toString());
                }
            } catch (wrench::ExecutionException &e) {
                //ignore (network error or something)
            }

            double completion_time = wrench::Simulation::getCurrentSimulatedDate();
            double expected_completion_time;
            if (job == standard_job_2_nodes) {
                expected_completion_time = 100 + 600;
            } else {
                throw std::runtime_error("Phantom job completion!");
            }
            double delta = fabs(expected_completion_time - completion_time);
            double tolerance = 5;
            if (delta > tolerance) {
                throw std::runtime_error("Unexpected job completion time for job " + job->getName() + ": " +
                                         std::to_string(completion_time) + " (expected: " + std::to_string(expected_completion_time) + ")");
            }

        }
        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceTest, WorkloadTraceFileSWFDifferentTimeOriginTest) {
#else
TEST_F(BatchServiceTest, DISABLED_WorkloadTraceFileSWFDifferentTimeOriginTest) {
#endif
    DO_TEST_WITH_FORK(do_WorkloadTraceFileDifferentTimeOriginSWF_test);
}


void BatchServiceTest::do_WorkloadTraceFileDifferentTimeOriginSWF_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Valid trace file
    std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "swf_trace.swf";
    FILE *trace_file;
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 100 -1 100 -1 -1 -1 2 1800 -1\n");
    fprintf(trace_file, "2 101 -1 1800 -1 -1 -1 2 1800 -1\n");
    fprintf(trace_file, "3 102 -1 3600 -1 -1 -1 4 3600 -1\n");
    fclose(trace_file);

    // Create a Batch Service with a the valid trace file
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "easy_bf"},
                                                    {wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                                    {wrench::BatchComputeServiceProperty::SIMULATE_COMPUTATION_AS_SLEEP, "true"},
                                                    {wrench::BatchComputeServiceProperty::BATSCHED_LOGGING_MUTED, "true"},
                                                    {wrench::BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE, "false"},
                                                    {wrench::BatchComputeServiceProperty::SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE, "0"}

                                            }
            )));


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new WorkloadTraceFileSWFDifferentTimeOriginTestWMS(
            this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}



























/**********************************************************************/
/**  WORKLOAD TRACE FILE TEST JSON                                    **/
/**********************************************************************/

class WorkloadTraceFileJSONTestWMS : public wrench::WMS {

public:
    WorkloadTraceFileJSONTestWMS(BatchServiceTest *test,
                                 const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                 const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                 std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        wrench::Simulation::sleep(10);
        // At this point, using the fcfs algorithm, a 2-node 30-min job should complete around t=1.5 hours
        // and 4-node 30-min job should complete around t=2.5 hours

        std::vector<wrench::WorkflowTask *> tasks;
        std::map<std::string, std::string> batch_job_args;

        // Create and submit a job that needs 2 nodes and 30 minutes
        for (size_t i = 0; i < 2; i++) {
            double time_fudge = 1; // 1 second seems to make it all work!
            double task_flops = 10 * (1 * (1800 - time_fudge));
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->getWorkflow()->addTask("test_job_1_task_" + std::to_string(i),
                                                         task_flops,
                                                         num_cores, num_cores, ram));
        }

        // Create a Standard Job with only the tasks
        auto standard_job_2_nodes = job_manager->createStandardJob(tasks);

        // Create the batch_standard_and_pilot_jobs-specific argument
        batch_job_args["-N"] = std::to_string(2); // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(1800); // Time in minutes (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10); //number of cores per task1

        // Submit this job to the batch_standard_and_pilot_jobs service
        job_manager->submitJob(standard_job_2_nodes, *(this->getAvailableComputeServices<wrench::ComputeService>().begin()), batch_job_args);


        // Create and submit a job that needs 2 nodes and 30 minutes
        tasks.clear();
        for (size_t i = 0; i < 4; i++) {
            double time_fudge = 1; // 1 second seems to make it all work!
            double task_flops = 10 * (1 * (1800 - time_fudge));
            int num_cores = 10;
            double ram = 0.0;
            tasks.push_back(this->getWorkflow()->addTask("test_job_2_task_" + std::to_string(i),
                                                         task_flops,
                                                         num_cores, num_cores, ram));
        }


        // Create a Standard Job with only the tasks
        auto standard_job_4_nodes = job_manager->createStandardJob(tasks);

        // Create the batch_standard_and_pilot_jobs-specific argument
        batch_job_args.clear();
        batch_job_args["-N"] = std::to_string(4); // Number of nodes/tasks
        batch_job_args["-t"] = std::to_string(1800); // Time in minutes (at least 1 minute)
        batch_job_args["-c"] = std::to_string(10); //number of cores per task1

        // Submit this job to the batch_standard_and_pilot_jobs service
        job_manager->submitJob(standard_job_4_nodes, *(this->getAvailableComputeServices<wrench::ComputeService>().begin()), batch_job_args);


        // Wait for the two execution events
        for (auto job : {standard_job_2_nodes, standard_job_4_nodes}) {
            // Wait for the workflow execution event
            WRENCH_INFO("Waiting for job completion of job %s", job->getName().c_str());
            std::shared_ptr<wrench::ExecutionEvent> event;
            try {
                event = this->waitForNextEvent();
                auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);
                if (real_event) {
                    if (real_event->standard_job != job) {
                        throw std::runtime_error("Wrong job completion order: got " +
                                                 real_event->standard_job->getName() + " but expected " + job->getName());
                    }
                } else {
                    throw std::runtime_error(
                            "Unexpected workflow execution event: " + event->toString());
                }
            } catch (wrench::ExecutionException &e) {
                //ignore (network error or something)
            }

            double completion_time = wrench::Simulation::getCurrentSimulatedDate();
            double expected_completion_time;
            if (job == standard_job_2_nodes) {
                expected_completion_time = 3600  + 1800;
            } else if (job == standard_job_4_nodes) {
                expected_completion_time = 3600 * 2 + 1800;
            } else {
                throw std::runtime_error("Phantom job completion!");
            }
            double delta = std::abs(expected_completion_time - completion_time);
            double tolerance = 5;
            if (delta > tolerance) {
                throw std::runtime_error("Unexpected job completion time for job " + job->getName() + ": " +
                                         std::to_string(completion_time) + " (expected: " + std::to_string(expected_completion_time) + ")");
            }

        }
        return 0;
    }
};

TEST_F(BatchServiceTest, WorkloadTraceFileJSONTest) {
    DO_TEST_WITH_FORK(do_WorkloadTraceFileTestJSON_test);
}

void BatchServiceTest::do_WorkloadTraceFileTestJSON_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service with a trace file with no extension, which should throw
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/not_there_invalid"}}
            )), std::invalid_argument);

    // Create a Batch Service with a trace file with a bogus extension, which should throw
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/not_there.invalid"}}
            )), std::invalid_argument);


    // Create a Batch Service with a non-existing workload trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, "/not_there.json"}}
            ), std::invalid_argument);

    std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "swf_trace.json";
    FILE *trace_file;


    /** Create an invalid trace file: Broken JSON Syntax **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    std::string json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"BOGUS\","
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 0,\n"
            "         \"profile\": \"86396\",\n"
            "         \"res\": 256,\n"
            "         \"subtime\": 0.0,\n"
            "         \"walltime\": 86400.0\n"
            "      }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    // Create a Batch Service with a bogus trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    /** Create an invalid trace file: Valid JSON Syntax but no jobs **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"BOGUS\": [\n"
            "      {\n"
            "         \"id\": 0,\n"
            "         \"profile\": \"86396\",\n"
            "         \"res\": 256,\n"
            "         \"subtime\": 0.0,\n"
            "         \"walltime\": 86400.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"1190\",\n"
            "         \"res\": 2048,\n"
            "         \"subtime\": 7644.0,\n"
            "         \"walltime\": 72000.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    // Create a Batch Service with a bogus trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);


    /** Create an invalid trace file: Valid JSON Syntax but weird job **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      \"weird\",\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"1190\",\n"
            "         \"res\": 2048,\n"
            "         \"subtime\": 7644.0,\n"
            "         \"walltime\": 72000.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    /** Create an invalid trace file: Valid JSON and jobs, but missing field **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 0,\n"
            "         \"profile\": \"86396\",\n"
            "         \"res\": 412,\n"  // Misssing sub time
            "         \"walltime\": 86400.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"1190\",\n"
            "         \"res\": 2048,\n"
            "         \"subtime\": 7644.0,\n"
            "         \"walltime\": 72000.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    // Create a Batch Service with a bogus trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    /** Do the same, but pass the BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE property **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 0,\n"
            "         \"profile\": \"86396\",\n"
            "         \"res\": 412,\n"  // Misssing sub time
            "         \"walltime\": 86400.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"1190\",\n"
            "         \"res\": 2048,\n"
            "         \"subtime\": 7644.0,\n"
            "         \"walltime\": 72000.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    // Create a Batch Service with a bogus trace file, which should throw
    ASSERT_NO_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                             {wrench::BatchComputeServiceProperty::IGNORE_INVALID_JOBS_IN_WORKLOAD_TRACE_FILE, "true"}}
            ));

    /** Create an invalid trace file: Valid JSON and jobs, but invalid res field **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 0,\n"
            "         \"profile\": \"86396\",\n"
            "         \"res\": 0,\n"
            "         \"subtime\": 3123.0,\n"
            "         \"walltime\": 86400.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"1190\",\n"
            "         \"res\": 2048,\n"
            "         \"subtime\": 7644.0,\n"
            "         \"walltime\": 72000.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    // Create a Batch Service with a bogus trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);


    /** Create an invalid trace file: Valid JSON and jobs, but invalid subtime field **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 0,\n"
            "         \"profile\": \"86396\",\n"
            "         \"res\": 10,\n"
            "         \"subtime\": -100,\n"
            "         \"walltime\": 86400.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"1190\",\n"
            "         \"res\": 2048,\n"
            "         \"subtime\": 7644.0,\n"
            "         \"walltime\": 72000.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    // Create a Batch Service with a bogus trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);

    /** Create an invalid trace file: Valid JSON and jobs, but invalid walltime field **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 0,\n"
            "         \"profile\": \"86396\",\n"
            "         \"res\": 10,\n"
            "         \"subtime\": 100,\n"
            "         \"walltime\": 86400.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"1190\",\n"
            "         \"res\": 2048,\n"
            "         \"subtime\": 7644.0,\n"
            "         \"walltime\": -1000.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);

    // Create a Batch Service with a bogus trace file, which should throw
    ASSERT_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            ), std::invalid_argument);


    /** Create an valid trace file with custom first job start time **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"666\",\n"
            "         \"res\": 4,\n"
            "         \"subtime\": 0.0,\n"
            "         \"walltime\": 3600.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 2,\n"
            "         \"profile\": \"666\",\n"
            "         \"res\": 2,\n"
            "         \"subtime\": 1.0,\n"
            "         \"walltime\": 3600.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);


    // Create a Batch Service with a non-existing workload trace file, which should throw
    ASSERT_NO_THROW(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                             {wrench::BatchComputeServiceProperty::SUBMIT_TIME_OF_FIRST_JOB_IN_WORKLOAD_TRACE_FILE, "10"}}
            ));


    /** Create an valid trace file **/

    trace_file = fopen(trace_file_path.c_str(), "w");

    json_string =
            "{\n"
            "  \"command\": \"/home/pfdutot/forge/batsim//tools/swf_to_batsim_workload_compute_only.py -t -gwo -i 1 -pf 93312 /tmp/expe_out/workloads/curie_downloaded.swf /tmp/expe_out/workloads/curie.json -cs 100e6 --verbose\",\n"
            "  \"date\": \"2018-07-10 17:08:09.673026\",\n"
            "  \"description\": \"this workload had been automatically generated\",\n"
            "  \"jobs\": [\n"
            "      {\n"
            "         \"id\": 1,\n"
            "         \"profile\": \"666\",\n"
            "         \"res\": 4,\n"
            "         \"subtime\": 0.0,\n"
            "         \"walltime\": 3600.0\n"
            "      },\n"
            "      {\n"
            "         \"id\": 2,\n"
            "         \"profile\": \"666\",\n"
            "         \"res\": 2,\n"
            "         \"subtime\": 1.0,\n"
            "         \"walltime\": 3600.0\n"
            "       }\n"
            "   ]\n"
            "}\n";

    fprintf(trace_file, "%s", json_string.c_str());
    fclose(trace_file);


    // Create a Batch Service with a non-existing workload trace file, which should throw
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {{wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path}}
            )));


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new WorkloadTraceFileJSONTestWMS(
            this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task1" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}


/**********************************************************************/
/**  GET QUEUE STATE TEST                                            **/
/**********************************************************************/

class GetQueueStateTestWMS : public wrench::WMS {

public:
    GetQueueStateTestWMS(BatchServiceTest *test,
                         const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                         const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                         std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }

private:

    BatchServiceTest *test;

    int main() {

        auto cs = *(this->getAvailableComputeServices<wrench::BatchComputeService>().begin());

        {
            wrench::Simulation::sleep(10);

            std::vector<std::tuple<std::string, std::string, int, int, int, double, double>> queue_state = cs->getQueue();
            if (queue_state.size() != 4) {
              throw std::runtime_error("Unexpected queue state (should have 4 entries - has " + std::to_string(queue_state.size()) + ")");
            }
            int num_positive_start_times = 0;
            for (auto const &j : queue_state) {
                if (std::get<6>(j) > 0) {
                    num_positive_start_times++;
                }
            }
            if (num_positive_start_times != 1) {
            throw std::runtime_error("Exactly one job should be shown as having started");
            }

//            WRENCH_INFO("QUEUE STATE:");
//            for (auto const &j : queue_state) {
//                WRENCH_INFO("%s %d %d %d %d %.2lf %.2lf",
//                            std::get<0>(j).c_str(),
//                            std::get<1>(j),
//                            std::get<2>(j),
//                            std::get<3>(j),
//                            std::get<4>(j),
//                            std::get<5>(j),
//                            std::get<6>(j));
//            }
        }

        {
            wrench::Simulation::sleep(4000);

            std::vector<std::tuple<std::string, std::string, int, int, int, double, double>> queue_state = cs->getQueue();
            if (queue_state.size() != 3) {
                throw std::runtime_error("Unexpected queue state (should have 3 entries - has " + std::to_string(queue_state.size()) + + ")");
            }
            int num_positive_start_times = 0;
            for (auto const &j : queue_state) {
                if (std::get<6>(j) > 0) {
                    num_positive_start_times++;
                }
            }
            if (num_positive_start_times != 2) {
                throw std::runtime_error("Exactly two jobs should be shown as having started");
            }

//            WRENCH_INFO("QUEUE STATE:");
//            for (auto const &j : queue_state) {
//                WRENCH_INFO("%s %s %d %d %d %.2lf %.2lf",
//                            std::get<0>(j).c_str(),
//                            std::get<1>(j),
//                            std::get<2>(j),
//                            std::get<3>(j),
//                            std::get<4>(j),
//                            std::get<5>(j),
//                            std::get<6>(j));
//            }
        }

        return 0;
    }
};

TEST_F(BatchServiceTest, GetQueueStateTest) {
    DO_TEST_WITH_FORK(do_GetQueueState_test);
}

void BatchServiceTest::do_GetQueueState_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("unit_test");
//    argv[1] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    std::string trace_file_path = UNIQUE_TMP_PATH_PREFIX + "swf_trace.swf";
    FILE *trace_file;

    // Create a trace file
    trace_file = fopen(trace_file_path.c_str(), "w");
    fprintf(trace_file, "1 0 -1 3600 -1 -1 -1 4 5600 -1 1 3\n");  // job that takes the whole machine
    fprintf(trace_file, "2 1 -1 3600 -1 -1 -1 2 8666 -1 1 12\n");  // job that takes half the machine
    fprintf(trace_file, "3 3 -1 3600 -1 -1 -1 2 9000 -1 1 3\n");  // job that takes half the machine
    fprintf(trace_file, "3 3 -1 3600 -1 -1 -1 4 3000 -1 1 5\n");  // job that takes half the machine
    fclose(trace_file);


    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",
                                            {
                                                    {wrench::BatchComputeServiceProperty::SIMULATED_WORKLOAD_TRACE_FILE, trace_file_path},
                                                    {wrench::BatchComputeServiceProperty::USE_REAL_RUNTIMES_AS_REQUESTED_RUNTIMES_IN_WORKLOAD_TRACE_FILE, "false"}
                                            }
            )));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new GetQueueStateTestWMS(
            this, {compute_service}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    for (int i=0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
