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

#include "CommunicatingActionsController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

#define COMMUNICATOR_SIZE 16UL

WRENCH_LOG_CATEGORY(custom_controller, "Log category for CommunicatingActionsController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param batch_cs: a batch compute service
     * @param ss: a storage service
     * @param hostname: the name of the host on which to start the Controller
     */
    CommunicatingActionsController::CommunicatingActionsController(
            std::shared_ptr<BatchComputeService> batch_cs,
            std::shared_ptr<StorageService> ss,
            const std::string &hostname) : ExecutionController(hostname, "mamj"),
                                           batch_cs(std::move(batch_cs)),
                                           ss(std::move(ss)) {}

    /**
     * @brief main method of the CommunicatingActionsController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int CommunicatingActionsController::main() {
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
        auto job = job_manager->createCompoundJob("job");

        /* Create actions that will participate in a parallel computation that mimics what could
         * happen in a parallel (MPI) matrix multiplication using the SUMMA algorithm */

        /* First, let's create a communicator through which actions will communicate */
        auto communicator = wrench::Communicator::createCommunicator(COMMUNICATOR_SIZE);

        /* Now let's create all actions */
        WRENCH_INFO("Adding %lu actions (that will communicate with each other) to the job", COMMUNICATOR_SIZE);
        for (int i = 0; i < COMMUNICATOR_SIZE; i++) {

            auto lambda_execute = [communicator, storage_service, file](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
                auto my_rank = communicator->join();
                auto num_ranks = communicator->getNumRanks();

                // Create my own data movement manager
                auto data_manager = action_executor->createDataMovementManager();

                communicator->barrier();
                WRENCH_INFO("I am an action with rank %lu in my communicator", my_rank);
                communicator->barrier();

                // Do a bulk-synchronous loop of 10 iterations
                for (unsigned long iter = 0; iter < 10; iter++) {

                    if (my_rank == 0) {
                        WRENCH_INFO("Iteration %lu", iter);
                    }
                    communicator->barrier();

                    // Perform some computation
                    double flops = 100 * GFLOP;
                    Simulation::compute(flops);

                    // Launch an asynchronous IO read to the storage service
                    unsigned long num_io_bytes = 100 * MB;
                    data_manager->initiateAsynchronousFileRead(FileLocation::LOCATION(storage_service, file), num_io_bytes);

                    // Participate in an all to all communication
                    unsigned long num_comm_bytes = 1 * MB;
                    std::map<unsigned long, double> sends;
                    for (int i = 0; i < num_ranks; i++) {
                        if (i != my_rank) {
                            sends[i] = num_comm_bytes;
                        }
                    }
                    communicator->sendAndReceive(sends, num_ranks - 1);

                    // Wait for the asynchronous IO read to complete
                    auto event = action_executor->waitForNextEvent();
                    auto io_event = std::dynamic_pointer_cast<wrench::FileReadCompletedEvent>(event);
                    if (not io_event) {
                        throw std::runtime_error("Custom action: unexpected IO event: " + io_event->toString());
                    }
                }
                communicator->barrier();
                WRENCH_INFO("Action with rank %lu completed!", my_rank);
            };

            auto lambda_terminate = [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {};

            job->addCustomAction("action_" + std::to_string(i), 0, 1, lambda_execute, lambda_terminate);
        }

        /* Submit the job to the batch compute service */
        WRENCH_INFO("Submitting job %s to the batch service", job->getName().c_str());
        std::map<std::string, std::string> service_specific_args =
                {{"-N", std::to_string(16)},
                 {"-c", std::to_string(1)},
                 {"-t", std::to_string(3600 * 100)}};
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
