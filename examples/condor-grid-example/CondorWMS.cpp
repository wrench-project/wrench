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

    CondorWMS::CondorWMS(const std::set<std::shared_ptr<wrench::ComputeService>> &compute_services,
                         const std::set<std::shared_ptr<wrench::StorageService>> &storage_services,
                         std::string hostname) : WMS(
                    nullptr, nullptr,
                    compute_services,
                    storage_services,
                    {},
                    nullptr,
                    hostname,
                    "condor-grid"){}

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

        // Get reference to the storage service
        auto ss = *(this->getAvailableStorageServices().begin());

        // Get references to all compute services (note that all jobs will be submitted to the htcondor_cs)
        auto htcondor_cs = *(this->getAvailableComputeServices<wrench::HTCondorComputeService>().begin());
        auto batch_cs = *(this->getAvailableComputeServices<wrench::BatchComputeService>().begin());
        auto cloud_cs = *(this->getAvailableComputeServices<wrench::CloudComputeService>().begin());

        // Create and start a 5-core VM with 32GB of RAM on the Cloud compute service */
        WRENCH_INFO("Creating a 5-core VM instance on the cloud service");
        cloud_cs->createVM(5, 32.0*1000*1000*1000, "my_vm", {}, {});
        WRENCH_INFO("Starting the VM instance, which exposes a usable bare-metal compute service");
        auto vm_cs = cloud_cs->startVM("my_vm");

        /* Add the VM's BareMetalComputeService to the HTCondor compute service */
        WRENCH_INFO("Adding the VM's bare-metal compute service to HTCondor");
        htcondor_cs->addComputeService(vm_cs);

        WRENCH_INFO("At this point, HTCondor has access to one batch compute service and one bare-metal service (which runs on a VM)");

        // Create a map of files, which are all supposed to be on the local SS
        std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;
        for (auto const &t : this->getWorkflow()->getTasks()) {
            for (auto const &f : t->getInputFiles()) {
                file_locations[f] = wrench::FileLocation::LOCATION(ss);
            }
            for (auto const &f : t->getOutputFiles()) {
                file_locations[f] = wrench::FileLocation::LOCATION(ss);
            }
        }

        // Split the tasks into two groups
        std::vector<wrench::WorkflowTask *> first_tasks;
        std::vector<wrench::WorkflowTask *> last_tasks;
        unsigned long task_count = 0;
        unsigned long num_tasks = this->getWorkflow()->getTasks().size();
        for (auto const &t : this->getWorkflow()->getTasks()) {
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
        htcondor_service_specific_arguments["universe"] = "grid";
        htcondor_service_specific_arguments["-N"] = "3";
        htcondor_service_specific_arguments["-c"] = "5";
        htcondor_service_specific_arguments["-t"] = "3600";
        // This argument below is not required, as there is a single batch service in this example
        htcondor_service_specific_arguments["-service"] = batch_cs->getName();
        job_manager->submitJob(grid_universe_job, htcondor_cs, htcondor_service_specific_arguments);
        WRENCH_INFO("Job submitted!");

        /* Submit the last tasks as individual non "grid universe" jobs to HTCondor */
        for (auto const &task : last_tasks) {
            WRENCH_INFO("Creating and submitting a single-task1 job (for task1 %s) as a non-grid-universe job to HTCondor (will run on the VM)",
                        task->getID().c_str());
            auto job = job_manager->createStandardJob(task, file_locations);
            job_manager->submitJob(job, htcondor_cs);
        }

        WRENCH_INFO("Waiting for Workflow Execution Events until the workflow execution is finished...");
        /* Wait for all execution events */
        while (not this->getWorkflow()->isDone()) {
            this->waitForAndProcessNextEvent();
        }

        htcondor_cs->stop();
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
        for (auto const &task : job->getTasks()) {
            WRENCH_INFO("    - Task %s ran on host %s (started at time %.2lf and finished at time %.2lf)",
                        task->getID().c_str(),
                        task->getPhysicalExecutionHost().c_str(),
                        task->getStartDate(),
                        task->getEndDate());
        }
    }



}
