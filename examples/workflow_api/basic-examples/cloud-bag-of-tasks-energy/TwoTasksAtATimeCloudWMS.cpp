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

WRENCH_LOG_CATEGORY(custom_wms, "Log category for TwoTasksAtATimeCloudWMS");

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
        WRENCH_INFO("Creating a 'large' VM with 5 cores  and a 'small' VM with 2 cores");
        auto large_vm = cloud_service->createVM(5, 500000);
        auto small_vm = cloud_service->createVM(2, 500000);

        /* Start the VMs */
        WRENCH_INFO("Starting both VMs");
        auto large_vm_compute_service = cloud_service->startVM(large_vm);
        auto small_vm_compute_service = cloud_service->startVM(small_vm);

        /* While the workflow isn't done, repeat the main loop */
        while (not this->getWorkflow()->isDone()) {

            /* Get the ready tasks */
            auto ready_tasks = this->getWorkflow()->getReadyTasks();

            /* Sort them by flops */
            std::sort(ready_tasks.begin(), ready_tasks.end(),
                      [](const std::shared_ptr<WorkflowTask>t1, const WorkflowTask  *t2) -> bool {

                          if (t1->getFlops() == t2->getFlops()) {
                              return ((uintptr_t) t1 > (uintptr_t) t2);
                          } else {
                              return (t1->getFlops() < t2->getFlops());
                          }
                      });

            /*  Pick the least and most expensive task */
            auto cheap_ready_task = ready_tasks.at(0);
            auto expensive_ready_task = ready_tasks.at(ready_tasks.size() - 1);

            /* Submit the cheap task to the small VM */
            /* First, we need to create a map of file locations, stating for each file
             * where is should be read/written */
            std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations1;
            file_locations1[cheap_ready_task->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            file_locations1[cheap_ready_task->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);

            /* Create the job  */
            WRENCH_INFO("Creating a job to run task1 %s (%.2lf)",
                    cheap_ready_task->getID().c_str(),  cheap_ready_task->getFlops());

            auto standard_job1 = job_manager->createStandardJob(cheap_ready_task, file_locations1);

            /* Submit the job to the small VM */
            WRENCH_INFO("Submit this job to the small VM");
            job_manager->submitJob(standard_job1, small_vm_compute_service);

            /* Submit the expensive task1 to the large VM */
            /* First, we need to create a map of file locations, stating for each file
             * where is should be read/written */
            std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations2;
            file_locations2[expensive_ready_task->getInputFiles().at(0)] = FileLocation::LOCATION(storage_service);
            file_locations2[expensive_ready_task->getOutputFiles().at(0)] = FileLocation::LOCATION(storage_service);

            /* Create the job  */
            WRENCH_INFO("Creating a job to run task1 %s (%.2lf)",
                        expensive_ready_task->getID().c_str(),  expensive_ready_task->getFlops());

            auto standard_job2 = job_manager->createStandardJob(expensive_ready_task, file_locations2);

            /* Submit the job to the large VM */
            WRENCH_INFO("Submit this job to the large VM");
            job_manager->submitJob(standard_job2, large_vm_compute_service);

            /* Sleep 10 seconds */
            WRENCH_INFO("Sleeping for 10 seconds");
            Simulation::sleep(10);

            /* Change the pstate of large VM  */
            WRENCH_INFO("Changing the pstate of the large VM to the lower pstate to save on energy temporarily");
            this->simulation->setPstate(large_vm, 0);

            /* Sleep 10 seconds */
            WRENCH_INFO("Sleeping for 10 seconds");
            Simulation::sleep(10);

            /* Change the pstate of CloudHost1  */
            WRENCH_INFO("Changing the pstate of the large VM back to the higher pstate");
            this->simulation->setPstate(large_vm, 1);

            /* Wait for  workflow execution event and process it. In this case we know that
             * the event will be a StandardJobCompletionEvent, which is processed by the method
             * processEventStandardJobCompletion() that this class overrides. */
            WRENCH_INFO("Wait for next event");
            this->waitForAndProcessNextEvent();

            /* And again! */
            WRENCH_INFO("Wait for next event again");
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
        /* Retrieve the job's first (and in our case only) task1 */
        auto task = job->getTasks().at(0);
        WRENCH_INFO("Notified that a standard job has completed task1 %s", task->getID().c_str());
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void TwoTasksAtATimeCloudWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task1 */
        auto task = job->getTasks().at(0);
        /* Print some error message */
        WRENCH_INFO("Notified that a standard job has failed for task1 %s with error %s",
                    task->getID().c_str(),
                    event->failure_cause->toString().c_str());
        throw std::runtime_error("ABORTING DUE TO JOB FAILURE");
    }


}
