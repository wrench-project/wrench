/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <gtest/gtest.h>
#include <wrench-dev.h>
#include <smpi/smpi.h>

#include <utility>

#include "../../../include/TestWithFork.h"
#include "../../../include/UniqueTmpPathPrefix.h"

WRENCH_LOG_CATEGORY(batch_compute_service_mpi_action_test, "Log category for BatchComputeServiceMPIAction test");

#define EPSILON 0.0001

class BatchComputeServiceMPIActionTest : public ::testing::Test {
public:
    std::shared_ptr<wrench::BatchComputeService> compute_service = nullptr;

    void do_MPIAction_test();

protected:
    BatchComputeServiceMPIActionTest() {

        // Create a platform file
        std::string xml = "<?xml version='1.0'?>"
                          "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">"
                          "<platform version=\"4.1\"> "
                          "   <zone id=\"AS0\" routing=\"Full\"> "
                          "       <host id=\"Host1\" speed=\"1f\" core=\"12\"> "
                          "       </host>"
                          "       <host id=\"Host2\" speed=\"1f\" core=\"12\"> "
                          "       </host>"
                          "       <host id=\"Host3\" speed=\"1f\" core=\"12\"> "
                          "       </host>"
                          "       <host id=\"Host4\" speed=\"1f\" core=\"12\">  "
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
        FILE *platform_file = fopen(platform_file_path.c_str(), "w");
        fprintf(platform_file, "%s", xml.c_str());
        fclose(platform_file);
    }

    std::string platform_file_path = UNIQUE_TMP_PATH_PREFIX + "platform.xml";
};


/**********************************************************************/
/**  ONE MPI ACTION TEST                                            **/
/**********************************************************************/

class BatchMPIActionTestWMS : public wrench::ExecutionController {
public:
    BatchMPIActionTestWMS(BatchComputeServiceMPIActionTest *test,
                          std::shared_ptr<wrench::BatchComputeService> batch_compute_service,
                          std::string &hostname) : wrench::ExecutionController(hostname, "test"), test(test), batch_compute_service(std::move(batch_compute_service)) {
    }

private:
    BatchComputeServiceMPIActionTest *test;
    std::shared_ptr<wrench::BatchComputeService> batch_compute_service;

    int main() override {

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        {
            /* Create a compound job that will hold an MPI action that's "too big" */
            auto job_to_big = job_manager->createCompoundJob("my_mpi_job_too_big");

            /* MPI code to execute - noop */
            auto mpi_code_noop = [](const std::shared_ptr<wrench::ExecutionController> &controller) {
            };

            /* Add an action with 100 MPI processes, each of which has 10 cores */
            auto action_too_big = job_to_big->addMPIAction("my_mpi_action_too_big", mpi_code_noop, 100, 10);

            /* Submit the job to the batch compute service */
            WRENCH_INFO("Submitting job %s to the batch service", job_to_big->getName().c_str());
            std::map<std::string, std::string> service_specific_args =
                    {{"-N", std::to_string(100)},
                     {"-c", std::to_string(10)},
                     {"-t", std::to_string(3600 * 1000)}};

            try {
                job_manager->submitJob(job_to_big, batch_compute_service, service_specific_args);
                throw std::runtime_error("Should not be able to submit a job that's too big");
            } catch (wrench::ExecutionException &e) {
                if (not std::dynamic_pointer_cast<wrench::NotEnoughResources>(e.getCause())) {
                    throw std::runtime_error("Unexpected failure cause: " + e.getCause()->toString());
                }
            }
        }

        /* Create a compound job that will hold an MPI action */
        auto job = job_manager->createCompoundJob("my_mpi_job");

        /* MPI code to execute */
        auto mpi_code = [](const std::shared_ptr<wrench::ExecutionController> &controller) {
            int num_procs;
            int my_rank;

            MPI_Init();
            MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
            MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

            WRENCH_INFO("I am MPI process: %d/%d", my_rank, num_procs);
            MPI_Barrier(MPI_COMM_WORLD);

            // Create my own data movement manager
            auto data_manager = controller->createDataMovementManager();

            int num_comm_bytes = 1000000;
            void *data = SMPI_SHARED_MALLOC(num_comm_bytes * num_procs);

            // Do a bulk-synchronous loop of 10 iterations
            for (unsigned long iter = 0; iter < 10; iter++) {

                if (my_rank == 0) {
                    WRENCH_INFO("Iteration %lu", iter);
                }
                MPI_Barrier(MPI_COMM_WORLD);

                // Perform some computation
                double flops = 100000;
                wrench::Simulation::compute(flops);

                // Participate in an all-to-all communication
                MPI_Alltoall(data, num_comm_bytes, MPI_CHAR, data, num_comm_bytes, MPI_CHAR, MPI_COMM_WORLD);
            }
            SMPI_SHARED_FREE(data);
            MPI_Barrier(MPI_COMM_WORLD);
            WRENCH_INFO("Action with rank %d completed!", my_rank);

            MPI_Finalize();
        };

        /* Add an action with 10 MPI processes, each of which has 2 cores */
        auto action = job->addMPIAction("my_mpi_action", mpi_code, 10, 2);

        action->getNumProcesses();
        action->getNumCoresPerProcess();


        /* Submit the job to the batch compute service */
        WRENCH_INFO("Submitting job %s to the batch service", job->getName().c_str());
        std::map<std::string, std::string> service_specific_args =
                {{"-N", std::to_string(4)},
                 {"-c", std::to_string(6)},
                 {"-t", std::to_string(3600 * 1000)}};
        job_manager->submitJob(job, batch_compute_service, service_specific_args);

        /* Wait for an execution event */
        auto event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected event: " + event->toString());
        }

        return 0;
    }
};

TEST_F(BatchComputeServiceMPIActionTest, MPIAction) {
    DO_TEST_WITH_FORK(do_MPIAction_test);
}

void BatchComputeServiceMPIActionTest::do_MPIAction_test() {
    // Create and initialize a simulation
    auto simulation = wrench::Simulation::createSimulation();

    int argc = 2;
    auto argv = (char **) calloc(argc, sizeof(char *));
    argv[0] = strdup("one_action_test");
    argv[1] = strdup("--wrench-commport-pool-size=10000");
    //        argv[2] = strdup("--wrench-full-log");

    ASSERT_NO_THROW(simulation->init(&argc, argv));

    // Setting up the platform
    ASSERT_NO_THROW(simulation->instantiatePlatform(platform_file_path));

    // Create a Compute Service
    ASSERT_NO_THROW(compute_service = simulation->add(
                            new wrench::BatchComputeService("Host1",
                                                            {"Host1", "Host2", "Host3", "Host4"},
                                                            "",
                                                            {})));

    // Create a WMS
    std::string hostname = "Host1";
    ASSERT_NO_THROW(simulation->add(new BatchMPIActionTestWMS(
            this, compute_service,
            hostname)));

    // Running a "do nothing" simulation
    ASSERT_NO_THROW(simulation->launch());

    for (int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}
