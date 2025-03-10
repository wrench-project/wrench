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
 **  - While the workflow is not done, repeat:
 **    - Pick a ready task if any
 **    - Submit it to the first available bare-metal compute service as a job in a way that
 **       - Uses 5 cores
 **       - Reads the input file from the StorageService
 **       - Writes the output file from the StorageService
 **/

#include <iostream>

#include "WorkflowAsAsingleJobWMS.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for WorkflowAsAsingleJobWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param workflow: the workflow to execute
     * @param compute_service: a bare-metal compute service available to run tasks
     * @param storage_service: a storage service available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    WorkflowAsAsingleJobWMS::WorkflowAsAsingleJobWMS(const std::shared_ptr<Workflow> &workflow,
                                                     const std::shared_ptr<BareMetalComputeService> &compute_service,
                                                     const std::shared_ptr<SimpleStorageService> &storage_service,
                                                     const std::string &hostname) : ExecutionController(hostname, "one-task-at-a-time"),
                                                                                    workflow(workflow), bare_metal_compute_service(compute_service), storage_service(storage_service) {}

    /**
     * @brief main method of the OneTaskAtATimeWMS daemon
     *
     * @return 0 on completion
     *
     */
    int WorkflowAsAsingleJobWMS::main() {
        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* We want to execute the workflow tasks so that intermediate files are
         * kept in the compute service's scratch place, and only the final output
         * file is written back to the storage service at host WMSHost. However,
         * files stored in the compute service's scratch space are erased
         * after the job that created them has completed. So we have to run
         * the entire workflow as a single multi-task job! */

        /* First, we need to create a map of file locations, stating for each file
         * where it should be read/written */
        std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
        // Set each file's location to the compute service's scratch space
        for (auto const &file: this->workflow->getFileMap()) {
            file_locations[file.second] = FileLocation::SCRATCH(file.second);
        }
        // For the workflow input files, in fact, set the location to the storage service
        for (auto const &f: this->workflow->getInputFiles()) {
            file_locations[f] = FileLocation::LOCATION(storage_service, f);
        }
        // For the workflow output files, in fact, set the location to the storage service
        for (auto const &f: this->workflow->getOutputFiles()) {
            file_locations[f] = FileLocation::LOCATION(storage_service, f);
        }

        /* Second, we create a job */
        WRENCH_INFO("Creating a job for the entire workflow");
        auto job = job_manager->createStandardJob(this->workflow->getTasks(), file_locations);

        /* Submit the job to the compute service */
        WRENCH_INFO("Submitting the job to the compute service");
        job_manager->submitJob(job, bare_metal_compute_service);

        /* Wait for a workflow execution event and process it. In this case we know that
         * the event will be a StandardJobCompletionEvent, which is processed by the method
         * processEventStandardJobCompletion() that this class overrides. */
        WRENCH_INFO("Waiting for next event");
        this->waitForAndProcessNextEvent();

        WRENCH_INFO("Workflow execution complete");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void WorkflowAsAsingleJobWMS::processEventStandardJobCompletion(const std::shared_ptr<StandardJobCompletedEvent> &event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        for (auto const &task: job->getTasks()) {
            WRENCH_INFO("Notified that a standard job has completed task %s", task->getID().c_str());
        }
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void WorkflowAsAsingleJobWMS::processEventStandardJobFailure(const std::shared_ptr<StandardJobFailedEvent> &event) {
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
