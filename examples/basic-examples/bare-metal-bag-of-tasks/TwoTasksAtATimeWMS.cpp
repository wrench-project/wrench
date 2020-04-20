/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Workflow Management System (WMS) implementation that operates as follows:
 **  - While the workflow is not done, repeat:
 **    - Pick up to two ready tasks
 **    - Submit both of them as part of a single job to the first available BareMetalComputeService so tbat:
 **       - The most expensive task uses 6 cores
 **       - The least expensive task uses 4 cores
 **       - Each task reads the input file from the StorageService
 **       - Each task reads the output file from the StorageService
 **/

#include <iostream>

#include "TwoTasksAtATimeWMS.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(custom_wms, "Log category for TwoTasksAtATimeWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    TwoTasksAtATimeWMS::TwoTasksAtATimeWMS(const std::set<std::shared_ptr<ComputeService>> &compute_services,
                                           const std::set<std::shared_ptr<StorageService>> &storage_services,
                                           const std::string &hostname) : WMS(
            nullptr, nullptr,
            compute_services,
            storage_services,
            {}, nullptr,
            hostname,
            "two-tasks-at-a-time") {}

    /**
     * @brief main method of the TwoTasksAtATimeWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int TwoTasksAtATimeWMS::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Get the first available bare-metal compute service and storage service  */
        auto compute_service = *(this->getAvailableComputeServices<BareMetalComputeService>().begin());
        auto storage_service = *(this->getAvailableStorageServices().begin());

        /* While the workflow isn't done, repeat the main loop */
        while (not this->getWorkflow()->isDone()) {

            /* Get the ready tasks */
            auto ready_tasks = this->getWorkflow()->getReadyTasks();

            /* Sort them by flops */
            std::sort(ready_tasks.begin(), ready_tasks.end(),
                      [](const WorkflowTask *t1, const WorkflowTask  *t2) -> bool {

                          if (t1->getFlops() == t2->getFlops()) {
                              return ((uintptr_t) t1 > (uintptr_t) t2);
                          } else {
                              return (t1->getFlops() < t2->getFlops());
                          }
                      });

            /*  Pick the least and most expensive task */
            auto cheap_ready_task = ready_tasks.at(0);
            auto expensive_ready_task = ready_tasks.at(ready_tasks.size() - 1);

            /* Create a standard job for the tasks */
            if (expensive_ready_task) {
                WRENCH_INFO("Creating a job for tasks %s and %s",
                            cheap_ready_task->getID().c_str(),
                            expensive_ready_task->getID().c_str());
            } else  {
                WRENCH_INFO("Creating a job for task %s",
                            cheap_ready_task->getID().c_str());
            }

            /* First, we need to create a map of file locations, stating for each file
             * where is should be read/written */
            std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
            file_locations[cheap_ready_task->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            file_locations[cheap_ready_task->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            file_locations[expensive_ready_task->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            file_locations[expensive_ready_task->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);

            /* Create the job  */
            auto standard_job = job_manager->createStandardJob(
                    {cheap_ready_task, expensive_ready_task}, file_locations);

            /* Then, we create the "service-specific arguments" that make it possible to configure
             * the way in which tasks in a job can run on the compute service */
            std::map<std::string, std::string> service_specific_args;
            service_specific_args[cheap_ready_task->getID()] = "4";
            service_specific_args[expensive_ready_task->getID()] = "6";

            WRENCH_INFO("Submitting the job, asking for %s cores for task %s, and "
                        "%s cores for task %s",
                        service_specific_args[cheap_ready_task->getID()].c_str(),
                        cheap_ready_task->getID().c_str(),
                        service_specific_args[expensive_ready_task->getID()].c_str(),
                        expensive_ready_task->getID().c_str());

            /* Submit the job to the compute service */
            job_manager->submitJob(standard_job, compute_service, service_specific_args);

            /* Wait for a workflow execution event and process it. In this case we know that
             * the event will be a StandardJobCompletionEvent, which is processed by the method
             * processEventStandardJobCompletion() that this class overrides. */
            WRENCH_INFO("Waiting for next event");
            this->waitForAndProcessNextEvent();
        }

        WRENCH_INFO("Workflow execution complete");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void TwoTasksAtATimeWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's tasks */
        for (auto const &task : job->getTasks()) {
            WRENCH_INFO("Notified that a standard job has completed task %s",
                        task->getID().c_str());
        }
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void TwoTasksAtATimeWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        /* Print some error message */
        WRENCH_INFO("Notified that a standard job has failed for task %s with error %s",
                    task->getID().c_str(),
                    event->failure_cause->toString().c_str());
        throw std::runtime_error("This should not happen in this example");
    }


}
