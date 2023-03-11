/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Controller that creates a job with custom actions that communicate
 **/

#include <iostream>
#include <utility>

#include "BatchMPIActionController.h"
#include "smpi/smpi.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

#define COMMUNICATOR_SIZE 16UL

WRENCH_LOG_CATEGORY(custom_controller, "Log category for MPIActionController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param batch_cs: a batch compute service
     * @param ss: a storage service
     * @param hostname: the name of the host on which to start the Controller
     */
    BatchMPIActionController::BatchMPIActionController(
            std::shared_ptr<BatchComputeService> batch_cs,
            std::shared_ptr<StorageService> ss,
            const std::string &hostname) : ExecutionController(hostname, "mamj"),
                                           batch_cs(std::move(batch_cs)),
                                           ss(std::move(ss)) {}

    /**
     * @brief main method of the MPIActionController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int BatchMPIActionController::main() {
        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Controller starting on host %s", Simulation::getHostName().c_str());

        auto storage_service = this->ss;

        /* Create some file on the storage service */
        auto file = Simulation::addFile("big_file", 10000 * MB);
        StorageService::createFileAtLocation(FileLocation::LOCATION(ss, file));

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Create a compound job that will hold all the actions */
        WRENCH_INFO("Creating a compound job");
        auto job = job_manager->createCompoundJob("my_mpi_job");

        /* MPI code to execute as part of the action. Note that there are extra barriers
         * so that the log output is not weirdly interleaved */
        auto mpi_code = [storage_service, file](const std::shared_ptr<ActionExecutor> &action_executor) {
            int num_procs;
            int my_rank;

            MPI_Init();
            MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
            MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

            WRENCH_INFO("I am MPI process: %d/%d", my_rank, num_procs);
            MPI_Barrier(MPI_COMM_WORLD);

            // Create my own data movement manager
            auto data_manager = action_executor->createDataMovementManager();

            // "Allocate" memory for the data that will be communicated
            int num_comm_bytes = 1000000;
            void *data = SMPI_SHARED_MALLOC(num_comm_bytes * num_procs);

            // Do a bulk-synchronous loop of 10 iterations
            for (unsigned long iter = 0; iter < 10; iter++) {

                if (my_rank == 0) {
                    WRENCH_INFO("Iteration %lu", iter);
                }
                MPI_Barrier(MPI_COMM_WORLD);

                // Perform some computation on 4 cores
                double flops = 100 * GFLOP;
                int num_threads = 4;
                double thread_creation_overhead = 0.01;
                double sequential_work = 0.1 * flops;
                double parallel_per_thread_work = (0.9 * flops) / num_threads;
                wrench::Simulation::computeMultiThreaded(num_threads, thread_creation_overhead, sequential_work, parallel_per_thread_work);

                // Launch an asynchronous IO read to the storage service
                double num_io_bytes = 100 * MB;
                data_manager->initiateAsynchronousFileRead(FileLocation::LOCATION(storage_service, file), num_io_bytes);

                // Participate in an all-to-all communication
                MPI_Alltoall(data, num_comm_bytes, MPI_CHAR, data, num_comm_bytes, MPI_CHAR, MPI_COMM_WORLD);

                // Wait for the asynchronous IO read to complete
                auto event = action_executor->waitForNextEvent();
                auto io_event = std::dynamic_pointer_cast<wrench::FileReadCompletedEvent>(event);
                if (not io_event) {
                    throw std::runtime_error("Custom action: unexpected IO event: " + io_event->toString());
                }
            }
            SMPI_SHARED_FREE(data);
            MPI_Barrier(MPI_COMM_WORLD);
            WRENCH_INFO("Action with rank %d completed!", my_rank);

            MPI_Finalize();
        };

        /* Add an action with 16 MPI processes, each of which has 4 cores */
        int num_mpi_processes = 16;
        int num_cores_per_process = 4;
        job->addMPIAction("my_mpi_action", mpi_code, num_mpi_processes, num_cores_per_process);

        /* Submit the job to the batch compute service */
        WRENCH_INFO("Submitting job %s to the batch service", job->getName().c_str());
        std::map<std::string, std::string> service_specific_args =
                {{"-N", std::to_string(num_mpi_processes)},    // Number of compute nodes
                 {"-c", std::to_string(num_cores_per_process)},// Number of cores per compute node
                 {"-t", std::to_string(3600 * 100)}};          // Requested time in seconds
        job_manager->submitJob(job, batch_cs, service_specific_args);

        /* Wait for an execution event */
        auto event = this->waitForNextEvent();
        if (not std::dynamic_pointer_cast<wrench::CompoundJobCompletedEvent>(event)) {
            throw std::runtime_error("Unexpected event: " + event->toString());
        }

        WRENCH_INFO("Controller terminating");
        return 0;
    }

}// namespace wrench
