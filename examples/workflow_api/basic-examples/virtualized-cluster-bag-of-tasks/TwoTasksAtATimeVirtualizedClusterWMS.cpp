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
 **  - Create two VMs, each one running on its own host
 **  - While the workflow is not done, repeat:
 **    - Pick up to two ready tasks
 **    - Submit both of them as  two different jobs to the VMs
 **       - The most expensive task on the more powerful VM
 **       - The least expensive task on the less powerful VM
 **       - Each task reads the input file from the StorageService
 **       - Each task reads the output file from the StorageService
 **    - After a while, migrate one VM
 **/

#include <iostream>

#include "TwoTasksAtATimeVirtualizedClusterWMS.h"

constexpr sg_size_t GB = 1000000000ULL;

WRENCH_LOG_CATEGORY(custom_wms, "Log category for TwoTasksAtATimeVirtualizedClusterWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param workflow: the workflow to execute
     * @param virtualized_cluster_compute_service: a virtualized cluster compute service available to run tasks
     * @param storage_service: a storage service available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    TwoTasksAtATimeVirtualizedClusterWMS::TwoTasksAtATimeVirtualizedClusterWMS(
            const std::shared_ptr<Workflow> &workflow,
            const std::shared_ptr<VirtualizedClusterComputeService> &virtualized_cluster_compute_service,
            const std::shared_ptr<StorageService> &storage_service,
            const std::string &hostname) : ExecutionController(hostname, "two-tasks-at-a-time-virtualized-cluster"),
                                           workflow(workflow), virtualized_cluster_compute_service(virtualized_cluster_compute_service), storage_service(storage_service) {}

    /**
     * @brief main method of the TwoTasksAtATimeVirtualizedClusterWMS daemon
     *
     * @return 0 on completion
     *
     */
    int TwoTasksAtATimeVirtualizedClusterWMS::main() {
        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Create a VM instance with 5 cores and one with 2 cores (and 500M of RAM) */
        WRENCH_INFO("Creating a 'large' VM with 5 cores  and a 'small' VM with 2 cores, both of them with 5GB RAM");
        auto large_vm = virtualized_cluster_compute_service->createVM(5, 5 * GB, "VirtualizedClusterHost1");
        auto small_vm = virtualized_cluster_compute_service->createVM(2, 5 * GB, "VirtualizedClusterHost2");

        /* Start the VMs */
        WRENCH_INFO("Start the large VM on host VirtualizedClusterHost1");
        auto large_vm_compute_service = virtualized_cluster_compute_service->startVM(large_vm);
        WRENCH_INFO("Start the small VM on host VirtualizedClusterHost2");
        auto small_vm_compute_service = virtualized_cluster_compute_service->startVM(small_vm);

        /* While the workflow isn't done, repeat the main loop */
        while (not this->workflow->isDone()) {
            /* Get the ready tasks */
            auto ready_tasks = this->workflow->getReadyTasks();

            /* Sort them by flops */
            std::sort(ready_tasks.begin(), ready_tasks.end(),
                      [](const std::shared_ptr<WorkflowTask> &t1, const std::shared_ptr<WorkflowTask> &t2) -> bool {
                          if (t1->getFlops() == t2->getFlops()) {
                              return ((uintptr_t) t1.get() > (uintptr_t) t2.get());
                          } else {
                              return (t1->getFlops() < t2->getFlops());
                          }
                      });

            /*  Pick the least and most expensive task */
            auto cheap_ready_task = ready_tasks.at(0);
            auto expensive_ready_task = ready_tasks.at(ready_tasks.size() - 1);

            /* Submit the cheap task to the small VM */
            /* First, we need to create a map of file locations, stating for each file
             * where it should be read/written */
            std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations1;
            auto input_file = cheap_ready_task->getInputFiles().at(0);
            auto output_file = cheap_ready_task->getOutputFiles().at(0);
            file_locations1[input_file] = FileLocation::LOCATION(storage_service, input_file);
            file_locations1[output_file] = FileLocation::LOCATION(storage_service, output_file);

            /* Create the job  */
            WRENCH_INFO("Creating a job to run task %s (%.2lf)",
                        cheap_ready_task->getID().c_str(), cheap_ready_task->getFlops());

            auto standard_job1 = job_manager->createStandardJob(cheap_ready_task, file_locations1);

            /* Submit the job to the small VM */
            WRENCH_INFO("Submit this job to the small VM");
            job_manager->submitJob(standard_job1, small_vm_compute_service);

            /* Submit the expensive task to the large VM */
            /* First, we need to create a map of file locations, stating for each file
             * where it should be read/written */
            std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations2;
            input_file = expensive_ready_task->getInputFiles().at(0);
            output_file = expensive_ready_task->getOutputFiles().at(0);
            file_locations2[input_file] = FileLocation::LOCATION(storage_service, input_file);
            file_locations2[output_file] = FileLocation::LOCATION(storage_service, output_file);

            /* Create the job  */
            WRENCH_INFO("Creating a job to run task %s (%.2lf)",
                        expensive_ready_task->getID().c_str(), expensive_ready_task->getFlops());

            auto standard_job2 = job_manager->createStandardJob(expensive_ready_task, file_locations2);

            /* Submit the job to the large VM */
            WRENCH_INFO("Submit this job to the large VM");
            job_manager->submitJob(standard_job2, large_vm_compute_service);

            /* Sleeping for 10 seconds */
            Simulation::sleep(10);

            /* Migrate the VM */
            WRENCH_INFO("Migrating the small VM from VirtualizedClusterHost1 to VirtualizedClusterHost2");
            virtualized_cluster_compute_service->migrateVM(small_vm, "VirtualizedClusterHost2");
            WRENCH_INFO("VM Migrated!");

            /* Wait for  workflow execution event and process it. In this case we know that
             * the event will be a StandardJobCompletionEvent, which is processed by the method
             * processEventStandardJobCompletion() that this class overrides. */
            WRENCH_INFO("Wait for next event");
            this->waitForAndProcessNextEvent();

            /* And again! */
            WRENCH_INFO("Wait for next event again");
            this->waitForAndProcessNextEvent();
        }

        // Shutting down and destroying VMs
        virtualized_cluster_compute_service->shutdownVM(large_vm);
        virtualized_cluster_compute_service->destroyVM(large_vm);
        virtualized_cluster_compute_service->shutdownVM(small_vm);
        virtualized_cluster_compute_service->destroyVM(small_vm);

        WRENCH_INFO("Workflow execution complete");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void TwoTasksAtATimeVirtualizedClusterWMS::processEventStandardJobCompletion(const std::shared_ptr<StandardJobCompletedEvent> &event) {
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
    void TwoTasksAtATimeVirtualizedClusterWMS::processEventStandardJobFailure(const std::shared_ptr<StandardJobFailedEvent> &event) {
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


}// namespace wrench
