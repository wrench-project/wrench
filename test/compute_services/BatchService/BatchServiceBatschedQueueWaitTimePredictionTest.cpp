/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <set>
#include <wrench-dev.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/SimulationMessage.h>
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/util/TraceFileLoader.h>
#include "wrench/workflow/job/PilotJob.h"

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_queue_wait_time_prediction_test, "Log category for BatchServiceBatschedQueueWaitTimePredictionTest");


class BatchServiceBatschedQueueWaitTimePredictionTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::StorageService> storage_service1 = nullptr;
    std::shared_ptr<wrench::StorageService> storage_service2 = nullptr;
    std::shared_ptr<wrench::ComputeService> compute_service = nullptr;

    void do_BatchJobBrokenEstimateWaitingTimeTest_test();

    void do_BatchJobBasicEstimateWaitingTimeTest_test();

    void do_BatchJobEstimateWaitingTimeTest_test();

    void do_BatchJobLittleComplexEstimateWaitingTimeTest_test();


protected:
    BatchServiceBatschedQueueWaitTimePredictionTest() {

        // Create the simplest workflow
        workflow = std::unique_ptr<wrench::Workflow>(new wrench::Workflow());

        // Create a four-host 10-core platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"10\"> "
                          "          <disk id=\"large_disk\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"10000000000000B\"/>"
                          "             <prop id=\"mount\" value=\"/\"/>"
                          "          </disk>"
                          "          <disk id=\"scratch\" read_bw=\"100MBps\" write_bw=\"40MBps\">"
                          "             <prop id=\"size\" value=\"100B\"/>"
                          "             <prop id=\"mount\" value=\"/scratch\"/>"
                          "          </disk>"
                          "       </host>"
                          "       <link id=\"1\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"2\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <link id=\"3\" bandwidth=\"5000GBps\" latency=\"0us\"/>"
                          "       <route src=\"Host3\" dst=\"Host1\"> <link_ctn id=\"2\"/> </route>"
                          "       <route src=\"Host4\" dst=\"Host1\"> <link_ctn id=\"2\"/> </route>"
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
/**  BROKEN QUERY/ANSWER BATCH_JOB_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobBrokenEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobBrokenEstimateWaitingTimeTestWMS(BatchServiceBatschedQueueWaitTimePredictionTest *test,
                                             const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                             const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                             std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceBatschedQueueWaitTimePredictionTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {
            // Create a sequential task that lasts one min and requires 2 cores
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 299, 1, 1, 1.0, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            auto job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service1)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service2))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "5"; //time in minutes
            batch_job_args["-c"] = "1"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception"
                );
            }
            double first_job_running = wrench::Simulation::getCurrentSimulatedDate();

            auto batch_service = (*this->getAvailableComputeServices<wrench::BatchComputeService>().begin());
            std::string job_id = "my_tentative_job";
            unsigned int nodes = 2;
            double walltime_seconds = 1000;
            //std::tuple<std::string,unsigned int,double> my_job = {job_id,nodes,walltime_seconds};
            std::tuple<std::string,unsigned int,unsigned int, double> my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
            std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {my_job};

            std::map<std::string,double> jobs_estimated_start_times;

            try {
                jobs_estimated_start_times = batch_service->getStartTimeEstimates(set_of_jobs);
                throw std::runtime_error("Should not be able to get a queue waiting time estimate");
            } catch (wrench::WorkflowExecutionException &e) {
                auto cause = std::dynamic_pointer_cast<wrench::FunctionalityNotAvailable>(e.getCause());
                if (not cause) {
                    throw std::runtime_error("Got an expected exception but unexpected failure cause : " +
                                             e.getCause()->toString() + " (expected: FunctionalityNotAvailable)");
                }
                if (cause->getService() != batch_service) {
                    throw std::runtime_error(
                            "Got the expected exception, but the failure cause does not point to the right service");
                }
                cause->toString();
                WRENCH_INFO("toString: %s", cause->toString().c_str());  // for coverage
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }

            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
            this->getWorkflow()->removeTask(task);

        }


        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, BatchJobBrokenEstimateWaitingTimeTest)
#else
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, DISABLED_BatchJobBrokenEstimateWaitingTimeTest)
#endif
{
    DO_TEST_WITH_FORK(do_BatchJobBrokenEstimateWaitingTimeTest_test);
}


void BatchServiceBatschedQueueWaitTimePredictionTest::do_BatchJobBrokenEstimateWaitingTimeTest_test() {

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

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Batch Service
    ASSERT_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "",  {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "BOGUS"}
                                            })), std::invalid_argument);
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "easy_bf"}
                                            })));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new BatchJobBrokenEstimateWaitingTimeTestWMS(
            this, {compute_service}, {storage_service1, storage_service2}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

//  std::unique_ptr<wrench::FileRegistryService> file_registry_service(
//          new wrench::FileRegistryService(hostname));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  BASIC QUERY/ANSWER BATCH_JOB_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobBasicEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobBasicEstimateWaitingTimeTestWMS(BatchServiceBatschedQueueWaitTimePredictionTest *test,
                                            const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                            const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                            std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceBatschedQueueWaitTimePredictionTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {

            // Create a sequential task that lasts one min and requires 2 cores
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 299, 1, 1, 1.0, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service1)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service2))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "5"; //time in minutes
            batch_job_args["-c"] = "1"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception"
                );
            }
            auto first_job_running = wrench::Simulation::getCurrentSimulatedDate();

            auto batch_service = (*this->getAvailableComputeServices<wrench::BatchComputeService>().begin());
            std::string job_id = "my_tentative_job";
            unsigned int nodes = 2;
            double walltime_seconds = 1000;
            //std::tuple<std::string,unsigned int,double> my_job = {job_id,nodes,walltime_seconds};
            std::tuple<std::string,unsigned int,unsigned int,double> my_job = std::make_tuple(job_id,nodes,1,walltime_seconds);
            std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {my_job};

            std::map<std::string,double> jobs_estimated_start_times;
            try {
                jobs_estimated_start_times = batch_service->getStartTimeEstimates(set_of_jobs);
            } catch (std::runtime_error &e) {
                throw std::runtime_error("Exception while getting queue waiting time estimate: " + std::string(e.what()));
            }
            double expected_wait_time = 300 - first_job_running;
            double tolerance = 1; // in seconds
            double delta = std::abs(expected_wait_time - (jobs_estimated_start_times[job_id] - tolerance));
            if (delta > 1) {
                throw std::runtime_error("Estimated start time incorrect (expected: " + std::to_string(expected_wait_time) + ", got: " + std::to_string(jobs_estimated_start_times[job_id]) + ")");
            }

            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }

            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
            this->getWorkflow()->removeTask(task);

        }


        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, BatchJobBasicEstimateWaitingTimeTest)
#else
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, DISABLED_BatchJobBasicEstimateWaitingTimeTest)
#endif
{
    DO_TEST_WITH_FORK(do_BatchJobBasicEstimateWaitingTimeTest_test);
}



void BatchServiceBatschedQueueWaitTimePredictionTest::do_BatchJobBasicEstimateWaitingTimeTest_test() {

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

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"}
                                            })));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new BatchJobBasicEstimateWaitingTimeTestWMS(
            this, {compute_service}, {storage_service1, storage_service2}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));


    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  QUERY/ANSWER BATCH_JOB_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobEstimateWaitingTimeTestWMS(BatchServiceBatschedQueueWaitTimePredictionTest *test,
                                       const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                       const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                       std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceBatschedQueueWaitTimePredictionTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {

            // Create a sequential task that lasts 5 minutes and requires 2 cores
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 299, 1, 1, 1.0, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service1)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service2))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "5"; //time in minutes
            batch_job_args["-c"] = "1"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception"
                );
            }
            double first_job_running = wrench::Simulation::getCurrentSimulatedDate();


            auto batch_service = (*this->getAvailableComputeServices<wrench::BatchComputeService>().begin());
            std::string job_id = "my_job1";
            unsigned int nodes = 2;
            double walltime_seconds = 1000;
            std::tuple<std::string,unsigned int,unsigned int,double> my_job = std::make_tuple(job_id,nodes,1,walltime_seconds);
            std::set<std::tuple<std::string,unsigned int,unsigned int,double>> set_of_jobs = {my_job};
            std::map<std::string,double> jobs_estimated_start_times = batch_service->getStartTimeEstimates(set_of_jobs);
            double expected_start_time = 300 - first_job_running; // in seconds
            double delta = std::abs(expected_start_time - (jobs_estimated_start_times[job_id] - 1));
            if (delta > 1) { // 1 second accuracy threshold
                throw std::runtime_error("Estimated start time incorrect (expected: " + std::to_string(expected_start_time) + ", got: " + std::to_string(jobs_estimated_start_times[job_id]) + ")");
            }


            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
            this->getWorkflow()->removeTask(task);

        }

        {

            // Create a sequential task that lasts one min and requires 2 cores
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 60, 2, 2, 1.0, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file"));


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service1)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service2))});

            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "5"; //time in minutes
            batch_job_args["-c"] = "4"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception"
                );
            }

            auto batch_service = (*this->getAvailableComputeServices<wrench::BatchComputeService>().begin());
            std::set<std::tuple<std::string, unsigned int, unsigned int, double>> set_of_jobs = {};
            for (int i=0; i<10; i++) {
                std::string job_id = "new_job"+std::to_string(i);
                unsigned int nodes = rand() % 4 + 1;
                double walltime_seconds = nodes * (rand() % 10 + 1);
                std::tuple<std::string, unsigned int, unsigned int, double> my_job = std::make_tuple(job_id, nodes,1, walltime_seconds);
                set_of_jobs.insert(my_job);
            }
            std::map<std::string, double> jobs_estimated_start_times = batch_service->getStartTimeEstimates(
                    set_of_jobs);

            // Wait for a workflow execution event
            std::shared_ptr<wrench::WorkflowExecutionEvent> event;
            try {
                event = this->getWorkflow()->waitForNextExecutionEvent();
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
            }
            if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
            }
            this->getWorkflow()->removeTask(task);

        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, BatchJobEstimateWaitingTimeTest)
#else
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, DISABLED_BatchJobEstimateWaitingTimeTest)
#endif
{
    DO_TEST_WITH_FORK(do_BatchJobEstimateWaitingTimeTest_test);
}


void BatchServiceBatschedQueueWaitTimePredictionTest::do_BatchJobEstimateWaitingTimeTest_test() {

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

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(

            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                                    {wrench::BatchComputeServiceProperty::BATCH_RJMS_DELAY, "0"}
                                            })));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new BatchJobEstimateWaitingTimeTestWMS(
            this, {compute_service}, {storage_service1, storage_service2}, hostname)));

    // Create two workflow files
    wrench::WorkflowFile *input_file = this->workflow->addFile("input_file", 10000.0);
    wrench::WorkflowFile *output_file = this->workflow->addFile("output_file", 20000.0);

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


/**********************************************************************/
/**  QUERY/ANSWER BATCH_JOB_LITTLE_COMPLEX_ESTIMATE_WAITING_TIME **/
/**********************************************************************/

class BatchJobLittleComplexEstimateWaitingTimeTestWMS : public wrench::WMS {

public:
    BatchJobLittleComplexEstimateWaitingTimeTestWMS(BatchServiceBatschedQueueWaitTimePredictionTest *test,
                                                    const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                                    const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                                    std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceBatschedQueueWaitTimePredictionTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        {

            // Submit the first job for 300 seconds and using 4 full cores
            wrench::WorkflowTask *task = this->getWorkflow()->addTask("task", 299, 1, 1, 1.0, 0);
            task->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task->addOutputFile(this->getWorkflow()->getFileByID("output_file1"));


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job = job_manager->createStandardJob(
                    {task},
                    {
                            {*(task->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service1)},
                            {*(task->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service2))});


            std::map<std::string, std::string> batch_job_args;
            batch_job_args["-N"] = "4";
            batch_job_args["-t"] = "5"; //time in minutes
            batch_job_args["-c"] = "1"; //number of cores per node
            try {
                job_manager->submitJob(job, this->test->compute_service, batch_job_args);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception"
                );
            }


            // Submit the second job for next 300 seconds and using 2 cores
            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask("task1", 299, 1, 1, 1.0, 0);
            task1->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task1->addOutputFile(this->getWorkflow()->getFileByID("output_file2"));


            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job1 = job_manager->createStandardJob(
                    {task1},
                    {
                            {*(task1->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service1)},
                            {*(task1->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service2))});

            std::map<std::string, std::string> batch_job_args1;
            batch_job_args1["-N"] = "2";
            batch_job_args1["-t"] = "5"; //time in minutes
            batch_job_args1["-c"] = "4"; //number of cores per node
            try {
                job_manager->submitJob(job1, this->test->compute_service, batch_job_args1);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception"
                );
            }

            // Submit the third job for next 300 seconds and using 4 cores
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask("task2", 299, 1, 1, 1.0, 0);
            task2->addInputFile(this->getWorkflow()->getFileByID("input_file"));
            task2->addOutputFile(this->getWorkflow()->getFileByID("output_file3"));

            // Create a StandardJob with some pre-copies and post-deletions (not useful, but this is testing after all)

            wrench::StandardJob *job2 = job_manager->createStandardJob(
                    {task2},
                    {
                            {*(task2->getInputFiles().begin()),  wrench::FileLocation::LOCATION(this->test->storage_service1)},
                            {*(task2->getOutputFiles().begin()), wrench::FileLocation::LOCATION(this->test->storage_service1)}
                    },
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>, std::shared_ptr<wrench::FileLocation>>(
                            this->getWorkflow()->getFileByID("input_file"), wrench::FileLocation::LOCATION(this->test->storage_service1),
                            wrench::FileLocation::LOCATION(this->test->storage_service2))},
                    {},
                    {std::tuple<wrench::WorkflowFile *, std::shared_ptr<wrench::FileLocation>>(this->getWorkflow()->getFileByID("input_file"),
                                                                                                 wrench::FileLocation::LOCATION(this->test->storage_service2))});

            std::map<std::string, std::string> batch_job_args2;
            batch_job_args2["-N"] = "4";
            batch_job_args2["-t"] = "5"; //time in minutes
            batch_job_args2["-c"] = "4"; //number of cores per node
            try {
                job_manager->submitJob(job2, this->test->compute_service, batch_job_args2);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error(
                        "Got some exception"
                );
            }

            auto batch_service = (*this->getAvailableComputeServices<wrench::BatchComputeService>().begin());

            /** Note that below we have extra seconds since when submitting
             *  Jobs to batsched we always ask for one extra second
             *  (the BATSCHED_JOB_EXTRA_TIME constant in BatchComputeService::sendAllQueuedJobsToBatsched()
             */
            std::string job_id = "my_job1";
            unsigned int nodes = 1;
            double walltime_seconds = 400;
            std::tuple<std::string,unsigned int,unsigned int,double> my_job = std::make_tuple(job_id,nodes,1,walltime_seconds);
            std::set<std::tuple<std::string,unsigned int,unsigned int, double>> set_of_jobs = {my_job};
            std::map<std::string,double> jobs_estimated_start_times = batch_service->getStartTimeEstimates(set_of_jobs);

            if ((jobs_estimated_start_times[job_id] - 903) > 1) {
                throw std::runtime_error("A) Estimated queue start time incorrect (expected: " + std::to_string(903) + ", got: " + std::to_string(jobs_estimated_start_times[job_id]) + ")");
            }

            job_id = "my_job2";
            nodes = 1;
            walltime_seconds = 299;
            my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
            set_of_jobs = {my_job};
            jobs_estimated_start_times = batch_service->getStartTimeEstimates(set_of_jobs);

            if (std::abs(jobs_estimated_start_times[job_id] - 301) > 1) {
                throw std::runtime_error("B) Estimated start time incorrect (expected: " + std::to_string(301) + ", got: " + std::to_string(jobs_estimated_start_times[job_id]) + ")");
            }


            job_id = "my_job3";
            nodes = 2;
            walltime_seconds = 299;
            my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
            set_of_jobs = {my_job};
            jobs_estimated_start_times = batch_service->getStartTimeEstimates(set_of_jobs);

            if (std::abs(jobs_estimated_start_times[job_id] - 301) > 1) {
                throw std::runtime_error("C) Estimated start time incorrect (expected: " + std::to_string(301) + ", got: " + std::to_string(jobs_estimated_start_times[job_id]) + ")");
            }


            job_id = "my_job4";
            nodes = 3;
            walltime_seconds = 299;
            my_job = std::make_tuple(job_id,nodes,1, walltime_seconds);
            set_of_jobs = {my_job};
            jobs_estimated_start_times = batch_service->getStartTimeEstimates(set_of_jobs);

            if (std::abs(jobs_estimated_start_times[job_id] - 903) > 1) {
                throw std::runtime_error("D) Estimated start time incorrect (expected: " + std::to_string(903) + ", got: " + std::to_string(jobs_estimated_start_times[job_id]) + ")");
            }

            for (unsigned char i=0; i<3; i++) {

                // Wait for a workflow execution event
                std::shared_ptr<wrench::WorkflowExecutionEvent> event;
                try {
                    event = this->getWorkflow()->waitForNextExecutionEvent();
                } catch (wrench::WorkflowExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }
                if (not std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event)) {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
            }

            this->getWorkflow()->removeTask(task);
            this->getWorkflow()->removeTask(task1);
            this->getWorkflow()->removeTask(task2);

        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, BatchJobLittleComplexEstimateWaitingTimeTest)
#else
TEST_F(BatchServiceBatschedQueueWaitTimePredictionTest, DISABLED_BatchJobLittleComplexEstimateWaitingTimeTest)
#endif
{
    DO_TEST_WITH_FORK(do_BatchJobLittleComplexEstimateWaitingTimeTest_test);
}

void BatchServiceBatschedQueueWaitTimePredictionTest::do_BatchJobLittleComplexEstimateWaitingTimeTest_test() {

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

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service1 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Storage Service
    ASSERT_NO_THROW(storage_service2 = simulation->add(
            new wrench::SimpleStorageService(hostname, {"/large_disk"})));

    // Create a Batch Service
    ASSERT_NO_THROW(compute_service = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                                    {wrench::BatchComputeServiceProperty::BATCH_RJMS_DELAY, "0"}
                                            })));

    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new BatchJobLittleComplexEstimateWaitingTimeTestWMS(
            this, {compute_service}, {storage_service1, storage_service2}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    simulation->add(new wrench::FileRegistryService(hostname));

    // Create two workflow files
    auto input_file = this->workflow->addFile("input_file", 10000.0);
    this->workflow->addFile("output_file", 20000.0);
    this->workflow->addFile("output_file1", 20000.0);
    this->workflow->addFile("output_file2", 20000.0);
    this->workflow->addFile("output_file3", 20000.0);

    // Staging the input_file on the storage service
    ASSERT_NO_THROW(simulation->stageFile(input_file, storage_service1));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}



