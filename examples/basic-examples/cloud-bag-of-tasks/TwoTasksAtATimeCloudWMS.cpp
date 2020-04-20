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
 **  - Create two VMs
 **  - While the workflow is not done, repeat:
 **    - Pick up to two ready tasks
 **    - Submit both of them as  two  different jobs to the VMs
 **       - The most expensive task on the more powerful VM
 **       - The least expensive task on the less powerful VM
 **       - Each task reads the input file from the StorageService
 **       - Each task reads the output file from the StorageService
 **/

#include <iostream>

#include "TwoTasksAtATimeCloudWMS.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(two_tasks_at_a_time_cloud_wms, "Log category for TwoTasksAtATimeCloudWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    TwoTasksAtATimeCloudWMS::TwoTasksAtATimeCloudWMS(const std::set<std::shared_ptr<ComputeService>> &compute_services,
                                         const std::set<std::shared_ptr<StorageService>> &storage_services,
                                         const std::string &hostname) : WMS(
            nullptr, nullptr,
            compute_services,
            storage_services,
            {}, nullptr,
            hostname,
            "two-tasks-at-a-time-cloud") {}

    /**
     * @brief main method of the TwoTasksAtATimeCloudWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int TwoTasksAtATimeCloudWMS::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Get the first available bare-metal compute service and storage service  */
        auto cloud_service = *(this->getAvailableComputeServices<CloudComputeService>().begin());
        auto storage_service = *(this->getAvailableStorageServices().begin());

        /* Create a VM instance with 5 cores and one with 2 cores (and 500M of RAM) */
        auto large_vm = cloud_service->createVM(5, 500000);
        auto small_vm = cloud_service->createVM(2, 500000);

        /* Start the VMs */
        auto large_vm_compute_service = cloud_service->startVM(large_vm);
        auto small_vm_compute_service = cloud_service->startVM(small_vm);

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

            /*  Pick the least and most (if any) expensive task */
            auto cheap_ready_task = ready_tasks.at(0);
            auto expensive_ready_task = (ready_tasks.size() > 1 ? ready_tasks.at(ready_tasks.size() - 1) : nullptr);

            /* Submit the cheap task to the small VM */
            /* First, we need to create a map of file locations, stating for each file
             * where is should be read/written */
            std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
            file_locations[cheap_ready_task->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            file_locations[cheap_ready_task->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);

            /* Create the job  */
            auto standard_job = job_manager->createStandardJob(cheap_ready_task, file_locations);

             /* Submit the job to the small VM */
            job_manager->submitJob(standard_job, small_vm_compute_service);

            if (expensive_ready_task) {
                /* Submit the cheap task to the small VM */
                /* First, we need to create a map of file locations, stating for each file
                 * where is should be read/written */
                std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
                file_locations[expensive_ready_task->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
                file_locations[expensive_ready_task->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);

                /* Create the job  */
                auto standard_job = job_manager->createStandardJob(expensive_ready_task, file_locations);

                /* Submit the job to the small VM */
                job_manager->submitJob(standard_job, large_vm_compute_service);
            }

            /* Wait for  workflow execution event and process it. In this case we know that
             * the event will be a StandardJobCompletionEvent, which is processed by the method
             * processEventStandardJobCompletion() that this class overrides. */
            this->waitForAndProcessNextEvent();

            /* And again! */
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
    void TwoTasksAtATimeCloudWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        WRENCH_INFO("Notified that a standard job has completed task %s", task->getID().c_str());
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void TwoTasksAtATimeCloudWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        /* Print some error message */
        WRENCH_INFO("Notified that a standard job has failed for task %s with error %s",
                    task->getID().c_str(),
                    event->failure_cause->toString().c_str());
        throw std::runtime_error("ABORTING DUE TO JOB FAILURE");
    }


}
