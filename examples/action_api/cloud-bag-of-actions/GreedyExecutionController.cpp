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

WRENCH_LOG_CATEGORY(custom_controller, "Log category for GreedyExecutionController");

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
                                                         const std::shared_ptr<CloudComputeService> compute_service,
                                                         const std::shared_ptr<SimpleStorageService> storage_service,
                                                         const std::string &hostname) : ExecutionController(hostname, "me"),
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
        std::uniform_real_distribution<double> gflop_dist(10000 * GFLOP, 100000 * GFLOP);
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
        for (int i = 0; i < num_actions; i++) {
            // Create action work
            auto work = gflop_dist(rng);
            // Create input file
            auto input_file = wrench::Simulation::addFile("input_file_" + std::to_string(i), mb_dist(rng));
            // Create a copy of the input file on the storage service
            wrench::Simulation::createFile(input_file, wrench::FileLocation::LOCATION(this->storage_service));
            auto output_file = wrench::Simulation::addFile("output_file_" + std::to_string(i), mb_dist(rng));
            action_specs.push_back(std::make_tuple(work, input_file, output_file));
        }

        /* Create two VMs on the cloud service and start them */
        auto vm1_name = this->compute_service->createVM(7, 100 * MB);
        auto vm1_cs = this->compute_service->startVM(vm1_name);
        auto vm2_name = this->compute_service->createVM(8, 100 * MB);
        auto vm2_cs = this->compute_service->startVM(vm2_name);

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /** Submit all action_specs in pairs using a random job configuration */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Submitting jobs to the batch compute service");
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        std::vector<std::shared_ptr<ComputeAction>> compute_actions;
        for (int i = 0; i < this->num_actions; i += 2) {
            // Create a compound job and submit it to the first VM
            auto job1 = job_manager->createCompoundJob("job_" + std::to_string(i));
            auto file_read_1 = job1->addFileReadAction("file_read_" + std::to_string(i), std::get<1>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_1 = job1->addComputeAction("computation_" + std::to_string(i), std::get<0>(action_specs.at(i + 1)), 0.0, 1, 10, wrench::ParallelModel::AMDAHL(0.9));
            auto file_write_1 = job1->addFileWriteAction("file_write_" + std::to_string(i), std::get<2>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            job1->addActionDependency(file_read_1, compute_1);
            job1->addActionDependency(compute_1, file_write_1);
            compute_actions.push_back(compute_1);
            WRENCH_INFO("Submitting job %s for executing action %s on VM %s",
                        job1->getName().c_str(), compute_1->getName().c_str(), vm1_name.c_str());
            job_manager->submitJob(job1, vm1_cs);

            // Create a compound job and submit it to the second VM
            auto job2 = job_manager->createCompoundJob("job_" + std::to_string(i + 1));
            auto file_read_2 = job2->addFileReadAction("file_read_" + std::to_string(i + 1), std::get<1>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_2 = job2->addComputeAction("computation_" + std::to_string(i + 1), std::get<0>(action_specs.at(i + 1)), 0.0, 1, 10, wrench::ParallelModel::AMDAHL(0.9));
            auto file_write_2 = job2->addFileWriteAction("file_write_" + std::to_string(i + 1), std::get<2>(action_specs.at(i + 1)), wrench::FileLocation::LOCATION(this->storage_service));
            job2->addActionDependency(file_read_2, compute_2);
            job2->addActionDependency(compute_2, file_write_2);
            compute_actions.push_back(compute_2);
            WRENCH_INFO("Submitting job %s for executing action %s on VM %s",
                        job2->getName().c_str(), compute_2->getName().c_str(), vm2_name.c_str());
            job_manager->submitJob(job2, vm2_cs);

            TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
            WRENCH_INFO("Waiting for execution events");
            TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

            this->waitForAndProcessNextEvent();
            this->waitForAndProcessNextEvent();
        }

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Inspecting task states");
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        for (auto const &action: compute_actions) {
            if (action->getState() != Action::State::COMPLETED) {
                TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);
            }
            WRENCH_INFO("Action %s: %s", action->getName().c_str(), action->getStateAsString().c_str());
            WRENCH_INFO("  - ran as part of job %s, which was submitted at time %.2lf and finished at time %.2lf",
                        action->getJob()->getName().c_str(),
                        action->getJob()->getSubmitDate(), action->getJob()->getEndDate());
            WRENCH_INFO("     ran on host %s (physical host: %s)",
                        action->getExecutionHistory().top().execution_host.c_str(),
                        action->getExecutionHistory().top().physical_execution_host.c_str());
            if (action->getState() != Action::State::COMPLETED) {
                WRENCH_INFO("  - action failure cause: %s", action->getFailureCause()->toString().c_str());
            }
            TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        }
        return 0;
    }

    /**
     * @brief Method to handle a compound job failure
     * @param event: the event
     */
    void GreedyExecutionController::processEventCompoundJobFailure(std::shared_ptr<CompoundJobFailedEvent> event) {
        WRENCH_INFO("Job %s failed!", event->job->getName().c_str());
    }

    /**
     * @brief Method to handle a compound job completion
     * @param event: the event
     */
    void GreedyExecutionController::processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent> event) {
        WRENCH_INFO("Job %s completed sucessfully!", event->job->getName().c_str());
    }


}// namespace wrench
