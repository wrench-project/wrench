/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>

#include "CondorWMS.h"
#include "CondorTimestamp.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for Custom WMS");

/**
 ** This WMS submits n tasks for execution to HTCondor. The first ~n/2 are submitted together
 *  as a single grid-universe job. The remaining tasks are submitted as individual non-grid-universe
 *  jobs.
 **/


namespace wrench {

    CondorWMS::CondorWMS(
            const std::shared_ptr<Workflow> &workflow,
            const std::shared_ptr<wrench::HTCondorComputeService> &htcondor_compute_service,
            const std::shared_ptr<wrench::BatchComputeService> &batch_compute_service,
            const std::shared_ptr<wrench::CloudComputeService> &cloud_compute_service,
            const std::shared_ptr<wrench::StorageService> &storage_service,
            std::string hostname) : ExecutionController(hostname, "condor-grid"),
                                    workflow(workflow),
                                    htcondor_compute_service(htcondor_compute_service),
                                    batch_compute_service(batch_compute_service),
                                    cloud_compute_service(cloud_compute_service),
                                    storage_service(storage_service) {}

    /**
     * Main method of the WMS
     */
    int CondorWMS::main() {

        // Set the logging output to GREEN
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create and start a 5-core VM with 32GB of RAM on the Cloud compute service */
        WRENCH_INFO("Creating a 5-core VM instance on the cloud service");
        cloud_compute_service->createVM(5, 32.0 * 1000 * 1000 * 1000, "my_vm", {}, {});
        WRENCH_INFO("Starting the VM instance, which exposes a usable bare-metal compute service");
        auto vm_cs = cloud_compute_service->startVM("my_vm");

        /* Add the VM's BareMetalComputeService to the HTCondor compute service */
        WRENCH_INFO("Adding the VM's bare-metal compute service to HTCondor");
        htcondor_compute_service->addComputeService(vm_cs);

        WRENCH_INFO("At this point, HTCondor has access to one batch_standard_and_pilot_jobs compute service and one bare-metal service (which runs on a VM)");

        // Create a map of files, which are all supposed to be on the local SS
        std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
        for (auto const &t: this->workflow->getTasks()) {
            for (auto const &f: t->getInputFiles()) {
                file_locations[f] = wrench::FileLocation::LOCATION(storage_service);
            }
            for (auto const &f: t->getOutputFiles()) {
                file_locations[f] = wrench::FileLocation::LOCATION(storage_service);
            }
        }

        // Split the tasks into two groups
        std::vector<std::shared_ptr<wrench::WorkflowTask>> first_tasks;
        std::vector<std::shared_ptr<wrench::WorkflowTask>> last_tasks;
        unsigned long task_count = 0;
        unsigned long num_tasks = this->workflow->getTasks().size();
        for (auto const &t: this->workflow->getTasks()) {
            if (task_count < num_tasks / 2) {
                first_tasks.push_back(t);
            } else {
                last_tasks.push_back(t);
            }
            task_count++;
        }

        // Submit the first tasks as part of a single "grid universe" job to HTCondor
        WRENCH_INFO("Creating a standard job with the first %ld tasks", first_tasks.size());
        auto grid_universe_job = job_manager->createStandardJob(first_tasks, file_locations);
        WRENCH_INFO("Submitting the job as a grid-universe job to HTCondor, asking for 3 compute nodes");
        std::map<std::string, std::string> htcondor_service_specific_arguments;
        htcondor_service_specific_arguments["-universe"] = "grid";
        htcondor_service_specific_arguments["-N"] = "3";
        htcondor_service_specific_arguments["-c"] = "5";
        htcondor_service_specific_arguments["-t"] = "3600";
        // This argument below is not required, as there is a single batch_standard_and_pilot_jobs service in this example
        htcondor_service_specific_arguments["-service"] = batch_compute_service->getName();
        job_manager->submitJob(grid_universe_job, htcondor_compute_service, htcondor_service_specific_arguments);
        WRENCH_INFO("Job submitted!");

        /* Submit the last tasks as individual non "grid universe" jobs to HTCondor */
        for (auto const &task: last_tasks) {
            WRENCH_INFO("Creating and submitting a single-task job (for task %s) as a non-grid-universe job to HTCondor (will run on the VM)",
                        task->getID().c_str());
            auto job = job_manager->createStandardJob(task, file_locations);
            job_manager->submitJob(job, htcondor_compute_service);
        }

        WRENCH_INFO("Waiting for Workflow Execution Events until the workflow execution is finished...");
        /* Wait for all execution events */
        while (not this->workflow->isDone()) {
            this->waitForAndProcessNextEvent();
        }

        htcondor_compute_service->stop();
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void CondorWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        WRENCH_INFO("Notified that a standard job has completed: ");
        for (auto const &task: job->getTasks()) {
            WRENCH_INFO("    - Task %s ran on host %s (started at time %.2lf and finished at time %.2lf)",
                        task->getID().c_str(),
                        task->getPhysicalExecutionHost().c_str(),
                        task->getStartDate(),
                        task->getEndDate());
        }
    }


}// namespace wrench
