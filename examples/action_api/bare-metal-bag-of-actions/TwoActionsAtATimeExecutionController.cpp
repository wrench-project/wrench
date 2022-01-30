/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller implementation that repeatedly submits two compute actions at a time
 ** to the bare-metal compute service
 **/

#include <iostream>

#include "TwoActionsAtATimeExecutionController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

WRENCH_LOG_CATEGORY(custom_controller, "Log category for TwoActionsAtATimeExecutionController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param num_actions: the number of actions
     * @param compute_services: a set of compute services available to run actions
     * @param storage_services: a set of storage services available to store data files
     * @param hostname: the name of the host on which to start the WMS
     */
    TwoActionsAtATimeExecutionController::TwoActionsAtATimeExecutionController(int num_actions,
                                                           const std::shared_ptr<BareMetalComputeService> compute_service,
                                                           const std::shared_ptr<SimpleStorageService> storage_service,
                                                           const std::string &hostname) :
            ExecutionController(hostname, "me"),
            compute_service(compute_service), storage_service(storage_service), num_actions(num_actions) {
    }

    /**
     * @brief main method of the TwoActionsAtATimeExecutionController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int TwoActionsAtATimeExecutionController::main() {

        /* Initialize and seed a RNG */
        std::uniform_real_distribution<double> gflop_dist(10 * GFLOP,1000 * GFLOP);
        std::uniform_real_distribution<double> mb_dist(1.0 * MB, 100.0 * MB);
        std::mt19937 rng(42);

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Execution controller starting on host %s at time %lf",
                    Simulation::getHostName().c_str(),
                    Simulation::getCurrentSimulatedDate());

        WRENCH_INFO("About to execute a workload with %d compute actions", this->num_actions);


        /* Create random amounts of work in GFlop, input sizes in bytes, and output sizes in bytes */
        std::vector<std::tuple<double, std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::DataFile>>> actions;
        actions.reserve(num_actions);
            auto input_file = wrench::Simulation::addFile("input_file_", mb_dist(rng));
            this->storage_service->createFile(input_file, wrench::FileLocation::LOCATION(this->storage_service));
            auto output_file = wrench::Simulation::addFile("output_file_", mb_dist(rng));
        for (int i=0; i < 1; i++) {
            // Create action work
            auto work = gflop_dist(rng);
            // Create input file
            // Create a copy of the input file on the storage service
            // Create output file
            actions.emplace_back(work, input_file, output_file);
        }
        
        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Submit each pair of actions */
        for (int i=0; i < num_actions; i+= 2) {

            WRENCH_INFO("Creating a 2-compute-action job");
            auto job = job_manager->createCompoundJob("job_" + std::to_string(i/2));
            // Action i with 2 cores
            auto file_read_1 = job->addFileReadAction("file_read_" + std::to_string(i), std::get<1>(actions.at(0)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_1 = job->addComputeAction("computation_" + std::to_string(i), std::get<0>(actions.at(0)), 0.0, 2, 2, wrench::ParallelModel::AMDAHL(0.9));
            auto file_write_1 = job->addFileWriteAction("file_write_" + std::to_string(i), std::get<2>(actions.at(0)), wrench::FileLocation::LOCATION(this->storage_service));
            job->addActionDependency(file_read_1, compute_1);
            job->addActionDependency(compute_1, file_write_1);

            // Action i+1 with 4 cores
            auto file_read_2 = job->addFileReadAction("file_read_" + std::to_string(i+1), std::get<1>(actions.at(0)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_2 = job->addComputeAction("computation_" + std::to_string(i+1), std::get<0>(actions.at(0)), 0.0, 4, 4, wrench::ParallelModel::AMDAHL(0.9));
            std::shared_ptr<Action> file_write_2 = job->addFileWriteAction("file_write_" + std::to_string(i+1), std::get<2>(actions.at(0)), wrench::FileLocation::LOCATION(this->storage_service));
            job->addActionDependency(file_read_2, compute_2);
            job->addActionDependency(compute_2, file_write_2);

            // Submit the job!
            WRENCH_INFO("Submitting the job for execution");
            job_manager->submitJob(job, this->compute_service);

            /* Wait for an execution event and process it. In this case we know that
             * the event will be a CompoundJobCompletionEvent, which is processed by the method
             * processEventCompoundJobCompletion() that this class overrides. */
            WRENCH_INFO("Waiting for next event");
            this->waitForAndProcessNextEvent();
        }

        WRENCH_INFO("Execution complete");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void TwoActionsAtATimeExecutionController::processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent> event) {
        WRENCH_INFO("Compound job %s has completed:", event->job->getName().c_str());
        // sort actions by start time
        auto job_actions = event->job->getActions();
        std::vector<std::shared_ptr<Action>> sorted_actions(job_actions.begin(), job_actions.end());
        std::sort(sorted_actions.begin(), sorted_actions.end(),
                  [](const std::shared_ptr<Action> &a1, const std::shared_ptr<Action> &a2) -> bool {

                      if (a1->getExecutionHistory().top().start_date == a2->getExecutionHistory().top().start_date) {
                          return ((uintptr_t) a1.get() > (uintptr_t) a2.get());
                      } else {
                          return (a1->getExecutionHistory().top().start_date < a2->getExecutionHistory().top().start_date);
                      }
                  });

        for (auto const &action : sorted_actions) {
            WRENCH_INFO("  - Action %s ran with %lu cores from time %.2lf until time %.2lf",
                        action->getName().c_str(), action->getExecutionHistory().top().num_cores_allocated,
                        action->getExecutionHistory().top().start_date,
                        action->getExecutionHistory().top().end_date);
        }
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void TwoActionsAtATimeExecutionController::processEventCompoundJobFailure(std::shared_ptr<CompoundJobFailedEvent> event) {
        WRENCH_INFO("Compound job %s has failed!", event->job->getName().c_str());
        throw std::runtime_error("This should not happen in this example");
    }

}
