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
#include "services/compute/standard_job_executor/StandardJobExecutorMessage.h"
#include <gtest/gtest.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/util/TraceFileLoader.h>
#include "wrench/workflow/job/PilotJob.h"

#include "../../include/TestWithFork.h"
#include "../../include/UniqueTmpPathPrefix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(batch_service_contiguity_test, "Log category for BatchServiceTest");


class BatchServiceBatschedContiguityTest : public ::testing::Test {

public:
    std::shared_ptr<wrench::ComputeService> batch_service_conservative_bf_contiguous = nullptr;
    std::shared_ptr<wrench::ComputeService> batch_service_conservative_bf_non_contiguous = nullptr;
    std::shared_ptr<wrench::ComputeService> batch_service_easy_bf_contiguous = nullptr;
    std::shared_ptr<wrench::ComputeService> batch_service_easy_bf_non_contiguous = nullptr;
    wrench::Simulation *simulation;


    void do_BatchJobContiguousAllocationTest_test();


protected:
    BatchServiceBatschedContiguityTest() {

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
/**  CONTIGUOUS ALLOCATION TEST                                      **/
/**********************************************************************/

class BatchJobContiguousAllocationTestWMS : public wrench::WMS {

public:
    BatchJobContiguousAllocationTestWMS(BatchServiceBatschedContiguityTest *test,
                                        const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                                        const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                                        std::string hostname) :
            wrench::WMS(nullptr, nullptr,  compute_services, storage_services, {}, nullptr,
                        hostname, "test") {
        this->test = test;
    }


private:

    BatchServiceBatschedContiguityTest *test;

    int main() {
        // Create a job manager
        auto job_manager = this->createJobManager();

        std::vector<std::shared_ptr<wrench::ComputeService>> compute_services;
        compute_services.push_back(this->test->batch_service_conservative_bf_contiguous);
        compute_services.push_back(this->test->batch_service_conservative_bf_non_contiguous);
        compute_services.push_back(this->test->batch_service_easy_bf_contiguous);
        compute_services.push_back(this->test->batch_service_easy_bf_non_contiguous);

        for (auto const &cs : compute_services) {

            wrench::WorkflowTask *task1 = this->getWorkflow()->addTask(cs->getName() + "task1", 59, 1, 1, 1.0, 0);
            wrench::WorkflowTask *task2 = this->getWorkflow()->addTask(cs->getName() + "task2", 118, 1, 1, 1.0, 0);
            wrench::WorkflowTask *task3 = this->getWorkflow()->addTask(cs->getName() + "task3", 59, 1, 1, 1.0, 0);
            wrench::WorkflowTask *task4 = this->getWorkflow()->addTask(cs->getName() + "task4", 59, 1, 1, 1.0, 0);
            wrench::StandardJob *job;

            double start_time = wrench::Simulation::getCurrentSimulatedDate();

            // Submit a sequential task that lasts one min and requires 1 host
            job = job_manager->createStandardJob(task1, {});
            std::map<std::string, std::string> batch_job_args1;
            batch_job_args1["-N"] = "1";
            batch_job_args1["-t"] = "1"; //time in minutes
            batch_job_args1["-c"] = "10"; // Get all cores
            try {
                job_manager->submitJob(job, cs, batch_job_args1);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Got some exception");
            }

            // Submit a sequential task that lasts one min and requires 2 host
            job = job_manager->createStandardJob(task2, {});
            std::map<std::string, std::string> batch_job_args2;
            batch_job_args2["-N"] = "2";
            batch_job_args2["-t"] = "2"; //time in minutes
            batch_job_args2["-c"] = "10"; // Get all cores
            try {
                job_manager->submitJob(job, cs, batch_job_args2);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Got some exception");
            }

            // Submit a sequential task that lasts one min and requires 2 host
            job = job_manager->createStandardJob(task3, {});
            std::map<std::string, std::string> batch_job_args3;
            batch_job_args3["-N"] = "1";
            batch_job_args3["-t"] = "1"; //time in minutes
            batch_job_args3["-c"] = "10"; // Get all cores
            try {
                job_manager->submitJob(job, cs, batch_job_args3);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Got some exception");
            }

            // Submit a sequential task that lasts one min and requires 2 host
            job = job_manager->createStandardJob(task4, {});
            std::map<std::string, std::string> batch_job_args4;
            batch_job_args4["-N"] = "2";
            batch_job_args4["-t"] = "1"; //time in minutes
            batch_job_args4["-c"] = "10"; // Get all cores
            try {
                job_manager->submitJob(job, cs, batch_job_args4);
            } catch (wrench::WorkflowExecutionException &e) {
                throw std::runtime_error("Got some exception");
            }


            bool contiguous = false;

            for (int i = 0; i < 4; i++) {
                // Wait for three workflow execution event
                std::shared_ptr<wrench::WorkflowExecutionEvent> event;
                try {
                    event = this->getWorkflow()->waitForNextExecutionEvent();
                } catch (wrench::WorkflowExecutionException &e) {
                    throw std::runtime_error("Error while getting and execution event: " + e.getCause()->toString());
                }

                auto real_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(event);

                if (real_event) {
                    wrench::WorkflowTask *task = *(real_event->standard_job->getTasks().begin());
                    if (task == task4) {
                        double now = wrench::Simulation::getCurrentSimulatedDate();
                        contiguous = (now >= start_time + 120);
                    }
                } else {
                    throw std::runtime_error("Unexpected workflow execution event: " + event->toString());
                }
            }

            if ((cs == this->test->batch_service_conservative_bf_contiguous) and (not contiguous)) {
                throw std::runtime_error("conservative_bf/contiguous is not contiguous!");
            }
            if ((cs == this->test->batch_service_conservative_bf_non_contiguous) and (contiguous)) {
                throw std::runtime_error("conservative_bf/non-contiguous is contiguous!");
            }
            if ((cs == this->test->batch_service_easy_bf_contiguous) and (not contiguous)) {
                throw std::runtime_error("easy_bf/contiguous is not contiguous!");
            }
            if ((cs == this->test->batch_service_easy_bf_non_contiguous) and (contiguous)) {
                throw std::runtime_error("easy_bf/non-contiguous is contiguous!");
            }
        }

        return 0;
    }
};

#ifdef ENABLE_BATSCHED
TEST_F(BatchServiceBatschedContiguityTest, BatchJobContiguousAllocationTest)
#else
TEST_F(BatchServiceBatschedContiguityTest, DISABLED_BatchJobContiguousAllocationTest)
#endif
{
    DO_TEST_WITH_FORK(do_BatchJobContiguousAllocationTest_test);
}


void BatchServiceBatschedContiguityTest::do_BatchJobContiguousAllocationTest_test() {

    // Create and initialize a simulation
    auto simulation = new wrench::Simulation();
    int argc = 1;
    auto argv = (char **) calloc(1, sizeof(char *));
    argv[0] = strdup("batch_service_test");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(this->platform_file_path));

    // Get a hostname
    std::string hostname = "Host1";

    // Create a Batch Service
    batch_service_conservative_bf_contiguous = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                                    {wrench::BatchComputeServiceProperty::BATCH_RJMS_DELAY, "0"},
                                                    {wrench::BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION,"true"}
                                            }));

    batch_service_conservative_bf_non_contiguous = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "conservative_bf"},
                                                    {wrench::BatchComputeServiceProperty::BATCH_RJMS_DELAY, "0"},
                                                    {wrench::BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION,"false"}
                                            }));

    batch_service_easy_bf_contiguous = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "easy_bf"},
                                                    {wrench::BatchComputeServiceProperty::BATCH_RJMS_DELAY, "0"},
                                                    {wrench::BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION,"true"}
                                            }));

    batch_service_easy_bf_non_contiguous = simulation->add(
            new wrench::BatchComputeService(hostname,
                                            {"Host1", "Host2", "Host3", "Host4"}, "", {
                                                    {wrench::BatchComputeServiceProperty::BATCH_SCHEDULING_ALGORITHM, "easy_bf"},
                                                    {wrench::BatchComputeServiceProperty::BATCH_RJMS_DELAY, "0"},
                                                    {wrench::BatchComputeServiceProperty::BATSCHED_CONTIGUOUS_ALLOCATION,"false"}
                                            }));


    // Create a WMS
    std::shared_ptr<wrench::WMS> wms = nullptr;;
    ASSERT_NO_THROW(wms = simulation->add(new BatchJobContiguousAllocationTestWMS(
            this, {}, {}, hostname)));

    ASSERT_NO_THROW(wms->addWorkflow(std::move(workflow.get())));

    // Running a "run a single task" simulation
    // Note that in these tests the WMS creates workflow tasks, which a user would
    // of course not be likely to do
    ASSERT_NO_THROW(simulation->launch());

    delete simulation;

    free(argv[0]);
    free(argv);
}


