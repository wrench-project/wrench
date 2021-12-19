/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller implementation that greedily submits actions to a batch
 ** compute service using various allocations...
 **/

#include <iostream>

#include "GreedyExecutionController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

WRENCH_LOG_CATEGORY(custom_execution_controller, "Log category for GreedyExecutionController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param num_actions: the number of actions
     * @param compute_services: a set of compute services available to run actions
     * @param storage_services: a set of storage services available to store data files
     * @param hostname: the name of the host on which to start the WMS
     */
    GreedyExecutionController::GreedyExecutionController(int num_actions,
                                                         const std::shared_ptr<BatchComputeService> compute_service,
                                                         const std::shared_ptr<SimpleStorageService> storage_service,
                                                         const std::string &hostname) :
            ExecutionController(hostname, "me"),
            compute_service(compute_service), storage_service(storage_service), num_actions(num_actions) {
    }

    /**
     * @brief main method of the GreedyExecutionController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int GreedyExecutionController::main() {

        /* Initialize and seed a RNG */
        std::mt19937 rng(42);

        /* Initialized random distributions to generate application workload */
        std::uniform_real_distribution<double> gflop_dist(10000 * GFLOP,100000 * GFLOP);
        std::uniform_real_distribution<double> mb_dist(1.0 * MB, 100.0 * MB);

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Execution controller starting on host %s at time %lf",
                    Simulation::getHostName().c_str(),
                    Simulation::getCurrentSimulatedDate());

        WRENCH_INFO("About to execute a workload with %d compute action_specs", this->num_actions);

        /* Create random amounts of work in GFlop, input sizes in bytes, and output sizes in bytes */
        std::vector<std::tuple<double, std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::DataFile>>> action_specs;
        action_specs.reserve(num_actions);
        for (int i=0; i < num_actions; i++) {
            // Create action work
            auto work = gflop_dist(rng);
            // Create input file
            auto input_file = wrench::Simulation::addFile("input_file_" + std::to_string(i), mb_dist(rng));
            // Create a copy of the input file on the storage service
            this->storage_service->createFile(input_file, wrench::FileLocation::LOCATION(this->storage_service));
            auto output_file = wrench::Simulation::addFile("output_file_" + std::to_string(i), mb_dist(rng));
            action_specs.push_back(std::make_tuple(work, input_file, output_file));
        }

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /** Submit all action_specs in pairs using a random job configuration */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Submitting jobs to the batch compute service");
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        std::uniform_int_distribution<int> node_dist(1, 2);
        std::uniform_int_distribution<int> core_dist(1, 10);
        std::uniform_int_distribution<int> minute_dist(5, 15);

        std::vector<std::shared_ptr<ComputeAction>> compute_actions;
        for (int i=0; i < this->num_actions; i += 2) {
            // Create a compound job
            auto job = job_manager->createCompoundJob("job_" + std::to_string(i/2));
            // Add action_specs to the job
            auto file_read_1 = job->addFileReadAction("file_read_" + std::to_string(i), std::get<1>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_1 = job->addComputeAction("computation_" + std::to_string(i), std::get<0>(action_specs.at(i + 1)), 0.0, 1, 10, wrench::ParallelModel::AMDAHL(0.9));
            auto file_write_1 = job->addFileWriteAction("file_write_" + std::to_string(i), std::get<2>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            job->addActionDependency(file_read_1, compute_1);
            job->addActionDependency(compute_1, file_write_1);
            compute_actions.push_back(compute_1);
            auto file_read_2 = job->addFileReadAction("file_read_" + std::to_string(i+1), std::get<1>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_2 = job->addComputeAction("computation_" + std::to_string(i+1), std::get<0>(action_specs.at(i + 1)), 0.0, 1, 10, wrench::ParallelModel::AMDAHL(0.9));
            auto file_write_2 = job->addFileWriteAction("file_write_" + std::to_string(i+1), std::get<2>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            job->addActionDependency(file_read_2, compute_2);
            job->addActionDependency(compute_2, file_write_2);
            compute_actions.push_back(compute_2);

            // Pick job configuration parameters
            int num_nodes = node_dist(rng);
            int num_cores_per_nodes = core_dist(rng);
            int num_minutes= minute_dist(rng);
            std::map<std::string, std::string> service_specific_args =
                    {{"-N", std::to_string(num_nodes)},
                     {"-c", std::to_string(num_cores_per_nodes)},
                     {"-t", std::to_string(num_minutes)}};

            // Submit the job!
            WRENCH_INFO("Submitting job %s (%d nodes, %d cores per node, %d minutes) for executing actions %s and %s",
                        job->getName().c_str(),
                        num_nodes, num_cores_per_nodes, num_minutes,
                        compute_1->getName().c_str(), compute_2->getName().c_str());
            job_manager->submitJob(job, this->compute_service, service_specific_args);


        }

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Waiting for execution events");
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        /* Wait for execution events, one per job. Some job will likely fail! The code below doe "manual"
         * processing of the event, rather than using registered callback functions */
        for (int i=0; i < this->num_actions/2; i++) {
            WRENCH_INFO("Number of queued jobs at the batch service: %lu", this->compute_service->getQueue().size());
            auto event = this->waitForNextEvent();
            WRENCH_INFO("Received an execution event: %s", event->toString().c_str());
        }

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Inspecting task states");
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        for (auto const &action : compute_actions) {
            if (action->getState() != Action::State::COMPLETED) {
                TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);

            }
            WRENCH_INFO("Action %s: %s", action->getName().c_str(), action->getStateAsString().c_str());
            WRENCH_INFO("  - ran as part of job %s, which was submitted at time %.2lf and finished at time %.2lf",
                        action->getJob()->getName().c_str(),
                        action->getJob()->getSubmitDate(), action->getJob()->getEndDate());
            if (action->getState() == Action::State::KILLED) {
                WRENCH_INFO("  - action failure cause: %s", action->getFailureCause()->toString().c_str());
            }
            TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        }
        return 0;
    }

}
