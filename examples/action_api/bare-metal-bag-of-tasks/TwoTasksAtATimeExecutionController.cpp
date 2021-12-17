/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller implementation that repeatedly submits two tasks at a time
 ** to the bare-metal compute service
 **/

#include <iostream>

#include "TwoTasksAtATimeExecutionController.h"

#define GFLOP (1000.0 * 1000.0 * 1000.0)
#define MB (1000.0 * 1000.0)

WRENCH_LOG_CATEGORY(custom_execution_controller, "Log category for TwoTasksAtATimeExecutionController");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param num_tasks: the number of tasks
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    TwoTasksAtATimeExecutionController::TwoTasksAtATimeExecutionController(int num_tasks,
                                                           const std::shared_ptr<BareMetalComputeService> compute_service,
                                                           const std::shared_ptr<SimpleStorageService> storage_service,
                                                           const std::string &hostname) :
            ExecutionController(hostname, "me"),
            compute_service(compute_service), storage_service(storage_service), num_tasks(num_tasks) {
    }

    /**
     * @brief main method of the TwoTasksAtATimeExecutionController daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int TwoTasksAtATimeExecutionController::main() {

        /* Initialize and seed a RNG */
        std::uniform_real_distribution<double> gflop_dist(10 * GFLOP,1000 * GFLOP);
        std::uniform_real_distribution<double> mb_dist(1.0 * MB, 100.0 * MB);
        std::mt19937 rng(42);

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Execution controller starting on host %s at time %lf",
                    Simulation::getHostName().c_str(),
                    Simulation::getCurrentSimulatedDate());

        WRENCH_INFO("About to execute a workload with %d tasks", this->num_tasks);

        /* Create random amounts of work in GFlop, input sizes in bytes, and output sizes in bytes */
        std::vector<std::tuple<double, std::shared_ptr<wrench::DataFile>, std::shared_ptr<wrench::DataFile>>> tasks;
        tasks.reserve(num_tasks);
        for (int i=0; i < num_tasks; i++) {
            // Create task work
            auto task_work = gflop_dist(rng);
            // Create input file
            auto input_file = wrench::Simulation::addFile("input_file_" + std::to_string(i), mb_dist(rng));
            // Create a copy of the input file on the storage service
            this->storage_service->createFile(input_file, wrench::FileLocation::LOCATION(this->storage_service));
            auto output_file = wrench::Simulation::addFile("output_file_" + std::to_string(i), mb_dist(rng));
            tasks.push_back(std::make_tuple(task_work, input_file, output_file));
        }
        
        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Submit each pair of tasks */
        for (int i=0; i < num_tasks; i+= 2) {

            WRENCH_INFO("Creating a 2-task job");
            auto job = job_manager->createCompoundJob("job_" + std::to_string(i/2));
            // Task i with 2 cores
            auto file_read_1 = job->addFileReadAction("file_read_" + std::to_string(i), std::get<1>(tasks.at(i)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_1 = job->addComputeAction("computation_" + std::to_string(i), std::get<0>(tasks.at(i)), 0.0, 2, 2, wrench::ParallelModel::AMDAHL(0.9));
            auto file_write_1 = job->addFileWriteAction("file_write_" + std::to_string(i), std::get<2>(tasks.at(i)), wrench::FileLocation::LOCATION(this->storage_service));
            job->addActionDependency(file_read_1, compute_1);
            job->addActionDependency(compute_1, file_write_1);

            // Task i+1 with 4 cores
            auto file_read_2 = job->addFileReadAction("file_read_" + std::to_string(i+1), std::get<1>(tasks.at(i+1)), wrench::FileLocation::LOCATION(this->storage_service));
            auto compute_2 = job->addComputeAction("computation_" + std::to_string(i+1), std::get<0>(tasks.at(i+1)), 0.0, 4, 4, wrench::ParallelModel::AMDAHL(0.9));
            auto file_write_2 = job->addFileWriteAction("file_write_" + std::to_string(i+1), std::get<2>(tasks.at(i+1)), wrench::FileLocation::LOCATION(this->storage_service));
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
    void TwoTasksAtATimeExecutionController::processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent> event) {
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
    void TwoTasksAtATimeExecutionController::processEventCompoundJobFailure(std::shared_ptr<CompoundJobFailedEvent> event) {
        WRENCH_INFO("Compound job %s has failed!", event->job->getName().c_str());
        throw std::runtime_error("This should not happen in this example");
    }

}
