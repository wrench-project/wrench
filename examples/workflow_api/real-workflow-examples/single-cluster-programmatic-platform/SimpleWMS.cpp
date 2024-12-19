/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>

#include "SimpleWMS.h"

WRENCH_LOG_CATEGORY(simple_wms, "Log category for Simple WMS");

namespace wrench {

    /**
     * @brief Constructor that creates a Simple WMS with
     *        a scheduler implementation, and a list of compute services
     *
     * @param workflow: a workflow to execute
     * @param bare_metal_compute_services: bare-metal compute services available to run jobs
     * @param storage_service: a storage service available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    SimpleWMS::SimpleWMS(const std::shared_ptr<Workflow> &workflow,
                         const std::set<std::shared_ptr<wrench::BareMetalComputeService>> &bare_metal_compute_services,
                         const std::shared_ptr<StorageService> &storage_service,
                         const std::string &hostname) : ExecutionController(hostname, "simple"),
                                                        workflow(workflow),
                                                        bare_metal_compute_services(bare_metal_compute_services),
                                                        storage_service(storage_service) {}

    /**
     * @brief main method of the SimpleWMS daemon
     *
     * @return 0 on completion
     *
     */
    int SimpleWMS::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Starting on host %s", S4U_Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        // Create a job manager
        this->job_manager = this->createJobManager();

        // Populate data structure to keep track of idle cores at each compute service
        for (auto const &cs : this->bare_metal_compute_services) {
            this->core_utilization_map[cs] = cs->getTotalNumCores(false);
        }


        while (true) {
            scheduleReadyTasks(workflow->getReadyTasks());

            // Wait for a workflow execution event, and process it
            try {
                this->waitForAndProcessNextEvent();
            } catch (ExecutionException &e) {
                WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again",
                            (e.getCause()->toString().c_str()));
                continue;
            }
            if (this->workflow->isDone()) {
                break;
            }
        }

        S4U_Simulation::sleep(10);

        WRENCH_INFO("--------------------------------------------------------");
        if (this->workflow->isDone()) {
            WRENCH_INFO("Workflow execution is complete!");
        } else {
            WRENCH_INFO("Workflow execution is incomplete!");
        }

        WRENCH_INFO("WMS terminating");

        return 0;
    }

    /**
     * @brief Process a StandardJobFailedEvent
     *
     * @param event: a workflow execution event
     */
    void SimpleWMS::processEventStandardJobFailure(const std::shared_ptr<StandardJobFailedEvent> &event) {
        auto job = event->standard_job;
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);
        WRENCH_INFO("Task %s has failed", (*job->getTasks().begin())->getID().c_str());
        WRENCH_INFO("failure cause: %s", event->failure_cause->toString().c_str());
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
    }

    /**
    * @brief Process a StandardJobCompletedEvent
    *
    * @param event: a workflow execution event
    */
    void SimpleWMS::processEventStandardJobCompletion(const std::shared_ptr<StandardJobCompletedEvent> &event) {
        auto job = event->standard_job;
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Task %s has COMPLETED (on service %s)",
                    (*job->getTasks().begin())->getID().c_str(),
                    job->getParentComputeService()->getName().c_str());
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        this->core_utilization_map[job->getParentComputeService()]++;
    }


    /**
     * @brief Helper method to schedule a task one available compute services. The naive scheduling
     *        strategy is to pick the task with the most computational work, and run it on
     *        the compute services with the fastest cores. In this example, all compute services
     *        are homogeneous, so we just pick the first available.
     *
     * @param ready_task: the ready tasks to schedule
     * @return
     */
    void SimpleWMS::scheduleReadyTasks(std::vector<std::shared_ptr<WorkflowTask>> ready_tasks) {

        if (ready_tasks.empty()) {
            return;
        }

        WRENCH_INFO("Trying to schedule %zu ready tasks", ready_tasks.size());
        // Sort the tasks
        std::sort(ready_tasks.begin(), ready_tasks.end(),
                  [](const std::shared_ptr<wrench::WorkflowTask> &x,
                     const std::shared_ptr<wrench::WorkflowTask> &y) {
                      if (x->getFlops() < y->getFlops()) {
                          return true;
                      } else if (x->getFlops() > y->getFlops()) {
                          return false;
                      } else {
                          return (x.get() > y.get());
                      }
                  }
        );

        unsigned long num_tasks_scheduled = 0;
        for (auto const &task: ready_tasks) {
            bool scheduled = false;
            for (auto const &cs: this->bare_metal_compute_services) {
                if (this->core_utilization_map[cs] > 0) {
                    // Specify that ALL files are read/written from the one storage service
                    std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
                    for (auto const &f: task->getInputFiles()) {
                        file_locations[f] = wrench::FileLocation::LOCATION(this->storage_service, f);
                    }
                    for (auto const &f: task->getOutputFiles()) {
                        file_locations[f] = wrench::FileLocation::LOCATION(this->storage_service, f);
                    }
                    try {
                        auto job = job_manager->createStandardJob(task, file_locations);
                        WRENCH_INFO(
                                "Submitting task %s to compute service %s", task->getID().c_str(),
                                cs->getName().c_str());
                        job_manager->submitJob(job, cs);
                        this->core_utilization_map[cs]--;
                        num_tasks_scheduled++;
                        scheduled = true;
                    } catch (ExecutionException &e) {
                        WRENCH_INFO("WARNING: Was not able to submit task %s, likely due to the pilot job having expired "
                                    "(I should get a notification of its expiration soon)",
                                    task->getID().c_str());
                    }
                    break;
                }
            }
            if (not scheduled) break;
        }
        WRENCH_INFO("Was able to schedule %lu out of %zu ready tasks", num_tasks_scheduled, ready_tasks.size());
    }

}// namespace wrench
