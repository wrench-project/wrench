/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Controller that creates a job with a powerful custom action
 **/

#include <iostream>
#include <utility>

#include "CommunicatingActionsController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

#define COMMUNICATOR_SIZE 16
#define SQRT_COMMUNICATOR_SIZE 4

#define MATRIX_SIZE 10000
#define BLOCK_SIZE ((double) MATRIX_SIZE / SQRT_COMMUNICATOR_SIZE)

WRENCH_LOG_CATEGORY(custom_controller, "Log category for CommunicatingActionsController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param batch_cs: a batch compute service
     * @param hostname: the name of the host on which to start the Controller
     */
    CommunicatingActionsController::CommunicatingActionsController(
            std::shared_ptr<BatchComputeService> batch_cs,
            const std::string &hostname) : ExecutionController(hostname, "mamj"),
                                           batch_cs(std::move(batch_cs)) {}

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
        WRENCH_INFO("Adding %d actions (that will communicate with each other) to the job", COMMUNICATOR_SIZE);
        for (int i = 0; i < COMMUNICATOR_SIZE; i++) {
            auto lambda_execute = [communicator](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {
                auto my_rank = communicator->join();
                auto my_col = my_rank % SQRT_COMMUNICATOR_SIZE;
                auto num_procs = communicator->getNumRanks();
                auto my_row = my_rank / SQRT_COMMUNICATOR_SIZE;
                communicator->barrier();
                WRENCH_INFO("I am an action with rank %lu in a communicator (row %lu / col %lu 2-D coordinates)", my_rank, my_row, my_col);
                for (int i = 0; i < SQRT_COMMUNICATOR_SIZE; i++) {
                    communicator->barrier();
                    if (my_rank == 0) {
                        WRENCH_INFO("Iteration %d's computation phase begins...", i);
                    }
                    communicator->barrier();
                    double flops = 2 * std::pow<double>(BLOCK_SIZE, 3);
                    Simulation::compute(flops);
                    if (my_rank == 0) {
                        WRENCH_INFO("Iteration %d's computation phase ended", i);
                        WRENCH_INFO("Iteration %d's communication phase begins...", i);
                    }
                    // Send messages to processes in my row and my column
                    std::map<unsigned long, double> sends;
                    double message_size = std::pow<double>(BLOCK_SIZE, 2);
                    for (int j = 0; j < SQRT_COMMUNICATOR_SIZE; j++) {
                        if (j != my_col) {
                            sends[my_row * SQRT_COMMUNICATOR_SIZE + j] = message_size;
                        }
                        if (j != my_row) {
                            sends[j * SQRT_COMMUNICATOR_SIZE + my_col] = message_size;
                        }
                    }
                    communicator->sendAndReceive(sends, (SQRT_COMMUNICATOR_SIZE - 1) + (SQRT_COMMUNICATOR_SIZE - 1));
                    if (my_rank == 0) {
                        WRENCH_INFO("Iteration %d's communication phase ended", i);
                    }
                }
                communicator->barrier();
                WRENCH_INFO("Action with rank %lu (row %lu / col %lu 2-D coordinates) completed!", my_rank, my_row, my_col);
                communicator->barrier();
            };
            auto lambda_terminate = [](const std::shared_ptr<wrench::ActionExecutor> &action_executor) {};

            job->addCustomAction("action_" + std::to_string(i), 0, 1, lambda_execute, lambda_terminate);
        }

        /* Submit the job to the batch compute service */
        WRENCH_INFO("Submitting job %s to the batch service", job->getName().c_str());
        std::map<std::string, std::string> service_specific_args =
                {{"-N", std::to_string(16)},
                 {"-c", std::to_string(1)},
                 {"-t", std::to_string(3600)}};
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
