/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** A Workflow Management System (WMS) implementation that operates on a workflow
 ** with a single task that has two input files and two output files as follows:
 **  - Run the workflow has a single job that:
 **    - Copies the first input file from the first storage service to the second one
 **    - Runs the task so that it produces its output files on the second storage service
 **    - Copies the task's first output file to the first storage service
 **    - Deletes the task's second output file on the second storage service
 **/

#include <iostream>

#include "ComplexJobWMS.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(complex_job_wms, "Log category for ComplexJobWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    ComplexJobWMS::ComplexJobWMS(const std::set<std::shared_ptr<ComputeService>> &compute_services,
                                 const std::set<std::shared_ptr<StorageService>> &storage_services,
                                 const std::string &hostname) : WMS(
            nullptr, nullptr,
            compute_services,
            storage_services,
            {}, nullptr,
            hostname,
            "complex-job") {}

    /**
     * @brief main method of the ComplexJobWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int ComplexJobWMS::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Get the first available bare-metal compute service  */
        auto compute_service = *(this->getAvailableComputeServices<BareMetalComputeService>().begin());

        /* Get the first and second available storage services */
        auto storage_services  = this->getAvailableStorageServices();
        std::shared_ptr<StorageService> storage_service1, storage_service2;
        if ((*(this->getAvailableStorageServices().begin()))->getHostname() == "StorageHost1") {
            storage_service1 = *(this->getAvailableStorageServices().begin());
            storage_service2 = *(this->getAvailableStorageServices().begin()++);
        } else {
            storage_service2 = *(this->getAvailableStorageServices().begin());
            storage_service1 = *(this->getAvailableStorageServices().begin()++);
        }

        /* Get references to the task and files */
        auto task = this->getWorkflow()->getTaskByID("task");
        auto infile_1 = this->getWorkflow()->getFileByID("infile_1");
        auto infile_2 = this->getWorkflow()->getFileByID("infile_2");
        auto outfile_1 = this->getWorkflow()->getFileByID("outfile_1");
        auto outfile_2 = this->getWorkflow()->getFileByID("outfile_2");

        /* Now let's create a map of file locations, stating for each file
         * where is should be read/written while the task executes */
        std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations;

        file_locations[infile_1] = FileLocation::LOCATION(storage_service2);
        file_locations[infile_2] = FileLocation::LOCATION(storage_service1);
        file_locations[outfile_1] = FileLocation::LOCATION(storage_service2);
        file_locations[outfile_2] = FileLocation::LOCATION(storage_service2);

        /* Let's create a set of "pre" file copy operations to be performed
         * BEFORE the task can run */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation> , std::shared_ptr<FileLocation>  >> pre_file_copies;
        pre_file_copies.emplace_back(infile_1, FileLocation::LOCATION(storage_service1), FileLocation::LOCATION(storage_service2));

        /* Let's create a set of "post" file copy operations to be performed
        * AFTER the task can run */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation> , std::shared_ptr<FileLocation>  >> post_file_copies;
        pre_file_copies.emplace_back(outfile_1, FileLocation::LOCATION(storage_service2), FileLocation::LOCATION(storage_service1));

        /* Let's create a set of file deletion operations to be performed
        * AFTER the "post" file copies have been performed */
        std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions;
        cleanup_file_deletions.push_back(std::make_tuple(outfile_2, FileLocation::LOCATION(storage_service2)));

        /* Create the standard job */
        auto job = job_manager->createStandardJob({task}, file_locations, pre_file_copies, post_file_copies, cleanup_file_deletions);

        /* Submit the job to the compute service */
        job_manager->submitJob(job, compute_service);

        /* Wait for a workflow execution event and process it. In this case we know that
         * the event will be a StandardJobCompletionEvent, which is processed by the method
         * processEventStandardJobCompletion() that this class overrides. */
        this->waitForAndProcessNextEvent();

        WRENCH_INFO("Workflow execution complete");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void ComplexJobWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
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
    void ComplexJobWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
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
