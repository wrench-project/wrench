/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Workflow Management System (WMS) implementation that operates as follows:
 **
 **  - While the workflow is not done, repeat:
 **    - Pick up to two ready tasks
 **    - Submit both of them as a single batch_standard_and_pilot_jobs job to the compute service
 **       - the job asks for two whole 10-core compute nodes to run two
 **         tasks at once (unless a single task is left), but requesting,
 **         with high probability, an amount of time only sufficient to run a single task.
 **         So one task succeeds, the other one failed, but  will be resubmitted later.
 **/

#include <iostream>

#include "TwoTasksAtATimeBatchWMS.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for TwoTasksAtATimeBatchWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    TwoTasksAtATimeBatchWMS::TwoTasksAtATimeBatchWMS(std::shared_ptr<Workflow> workflow,
                                                     const std::shared_ptr<BatchComputeService> &batch_compute_service,
                                                     const std::shared_ptr<StorageService> &storage_service,
                                                     const std::string &hostname) :
                                                     ExecutionController(hostname,"two-tasks-at-a-time-batch_standard_and_pilot_jobs"),
                                                     workflow(workflow), batch_compute_service(batch_compute_service), storage_service(storage_service) {}

    /**
     * @brief main method of the TwoTasksAtATimeBatchWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int TwoTasksAtATimeBatchWMS::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());WRENCH_INFO(
                "About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Get the first available bare-metal compute service and storage service  */

        /* Record the batch_standard_and_pilot_jobs node's core flop rate */
        double core_flop_rate = (*(batch_compute_service->getCoreFlopRate().begin())).second;

        /* For each task, estimate its execution time in minutes */
        std::map<std::shared_ptr<WorkflowTask>, long> execution_times_in_minutes;
        for (auto  const &t : this->workflow->getTasks())  {
            double parallel_efficiency =
                    std::dynamic_pointer_cast<wrench::ConstantEfficiencyParallelModel>(t->getParallelModel())->getEfficiency();
            double in_seconds = (t->getFlops() / core_flop_rate) /  (10 * parallel_efficiency);
            execution_times_in_minutes[t] = 1 + std::lround(in_seconds / 60.0);
            // The +1 above is just  so that we don't cut it too tight
        }

        /* Initialize and seed a RNG */
        std::uniform_int_distribution<long> dist(0, 1000000000);
        std::mt19937 rng(42);

        /* While the workflow isn't done, repeat the main loop */
        while (not this->workflow->isDone()) {

            /* Get the ready tasks */
            auto ready_tasks = this->workflow->getReadyTasks();

            /*  Pick a random index and tasks */
            int index = dist(rng) % ready_tasks.size();
            auto ready_task1 = ready_tasks.at(index);
            auto ready_task2 = (ready_tasks.size() > 1 ? ready_tasks.at((index + 1) % ready_tasks.size()) : nullptr);

            /* Swap the tasks if ready_task2 is cheaper than ready_task 1, for the  sake of the example */
            if (ready_task2 and (ready_task2->getFlops() < ready_task1->getFlops())) {
                auto tmp = ready_task1;
                ready_task1 = ready_task2;
                ready_task2 = tmp;
            }

            if (ready_task2) { WRENCH_INFO(
                        "Creating a job to execute tasks %s (%.1lf Gflops) and task %s (%.1lf Gflops)",
                        ready_task1->getID().c_str(), ready_task1->getFlops() / 1000000000.0,
                        ready_task2->getID().c_str(), ready_task2->getFlops() / 1000000000.0);
            } else { WRENCH_INFO("Creating a job to execute task %s (%.1lf Gflops)",
                                 ready_task1->getID().c_str(), ready_task1->getFlops() / 1000000000.0);
            }

            /* Create a map of file locations, stating for each file
             * where is should be read/written */
            std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
            file_locations[ready_task1->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            file_locations[ready_task1->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            if (ready_task2) {
                file_locations[ready_task2->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
                file_locations[ready_task2->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            }

            /* Create the job  */
            std::shared_ptr<StandardJob> standard_job;
            if (ready_task2) {
                standard_job = job_manager->createStandardJob({ready_task1, ready_task2}, file_locations);
            } else {
                standard_job = job_manager->createStandardJob(ready_task1, file_locations);
            }

            /* Construct the required (Slurm-like) service-specific arguments */
            std::map<std::string, std::string> service_specific_arguments;
            // number of nodes
            service_specific_arguments["-N"] = "2";
            // number of cores
            service_specific_arguments["-c"] = "10";
            // time
            WRENCH_INFO("Task %s should run in under %ld minutes",
                        ready_task1->getID().c_str(), execution_times_in_minutes[ready_task1]);
            if (ready_task2) {
                WRENCH_INFO("Task %s should run in under %ld minutes",
                            ready_task2->getID().c_str(), execution_times_in_minutes[ready_task2]);
            }

            // But let's submit the job so that it requests time sufficient only
            // for the cheaper task1, which will lead to the
            // expensive task1, if any, to be terminated prematurely (i.e., it will fail and thus still be ready).

            service_specific_arguments["-t"] = std::to_string(execution_times_in_minutes[ready_task1]);

            WRENCH_INFO("Submitting the job, asking for %s %s-core nodes for %s minutes",
                        service_specific_arguments["-N"].c_str(),
                        service_specific_arguments["-c"].c_str(),
                        service_specific_arguments["-t"].c_str());

            /* Submit the job to the small VM */
            job_manager->submitJob(standard_job, batch_compute_service, service_specific_arguments);

            /* Wait for a workflow execution event, which should be a failure
             * Note that this does not use the higher-level waitForAndProcessNextEvent()
             * method, but instead calls the lower-level waitForNextExecutionEvent() method and
             * then process the event manually */
            WRENCH_INFO("Waiting for the next event");
            
            try {
                auto event = this->waitForNextEvent();
                // Check that it is the expected event, just in  case
                if (auto job_failed_event = std::dynamic_pointer_cast<wrench::StandardJobFailedEvent>(
                        event)) { WRENCH_INFO("Notified of a job failure event (%s)",
                                              job_failed_event->failure_cause->toString().c_str());
                } else if (auto job_completed_event = std::dynamic_pointer_cast<wrench::StandardJobCompletedEvent>(
                        event)) { WRENCH_INFO("Notified of a job completion event!");
                } else {
                    throw std::runtime_error("Unexpected event (" + event->toString() + ")");
                }
            } catch (ExecutionException &e) {
                throw std::runtime_error("Unexpected workflow execution exception (" +
                                         std::string(e.what()) + ")");
            }

            /* At this point, the cheap task1 should have completed, and the expensive one,
             * if any, should have failed. Let's check and print some logging info */
            if (ready_task1->getState() != WorkflowTask::COMPLETED) {
                throw std::runtime_error("Task " + ready_task1->getID() + "should have completed successfully!");
            } else { WRENCH_INFO("Task %s has completed successfully :)", ready_task1->getID().c_str());
            }
            if (ready_task2) {
                if (ready_task2->getState() != WorkflowTask::READY) {
                    throw std::runtime_error("Task " + ready_task2->getID() + "should have failed");
                } else { WRENCH_INFO("Task %s has not completed successfully :(",
                                     ready_task2->getID().c_str());
                }
            }
        }

        WRENCH_INFO("Workflow execution complete");
        return 0;
    }

}
