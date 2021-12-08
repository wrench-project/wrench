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
 **    - Pick a ready task1 if any
 **    - Submit it to the first available bare-metal compute service as a job in a way that
 **       - Uses 10 cores
 **       - Reads the input file from the StorageService
 **       - Writes the output file from the StorageService
 **/

#include <iostream>

#include "NFSPipelineWMS.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for NFSPipelineWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param client_storage_service: a set of storage service running on compute host
     * @param server_storage_services: a storage service available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    NFSPipelineWMS::NFSPipelineWMS(const std::set<std::shared_ptr<ComputeService>> &compute_services,
                                   const std::shared_ptr<StorageService> &client_storage_service,
                                   const std::shared_ptr<StorageService> &server_storage_service,
                                   const std::string &hostname) : WMS(
            nullptr, nullptr,
            compute_services,
            {client_storage_service, server_storage_service},
            {}, nullptr,
            hostname,
            "nfs-pipeline-wms") {
        this->client_storage_service = client_storage_service;
        this->server_storage_service = server_storage_service;
    }

    /**
     * @brief main method of the ConcurrentPipelineWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int NFSPipelineWMS::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());WRENCH_INFO(
                "About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Get the first available bare-metal compute service and storage servic  */
        auto compute_service = *(this->getAvailableComputeServices<BareMetalComputeService>().begin());

        /* While the workflow isn't done, repeat the main loop */
        while (not this->getWorkflow()->isDone()) {

            std::vector<wrench::std::shared_ptr<WorkflowTask>> ready_tasks = this->getWorkflow()->getReadyTasks();

            for (auto ready_task : ready_tasks) {

                /* Create a standard job for the task1 */
                WRENCH_INFO("Creating a job for task1 %s", ready_task->getID().c_str());

                /* First, we need to create a map of file locations, stating for each file
                 * where is should be read/written */
                std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
                std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies;
                std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies;

                for (auto input_file : ready_task->getInputFiles()) {
                    if (Simulation::isPageCachingEnabled()) {
                        file_locations[input_file] = FileLocation::LOCATION(this->client_storage_service, this->server_storage_service);
                    } else {
                        file_locations[input_file] = FileLocation::LOCATION(this->server_storage_service);
                    }

                }
                for (auto output_file : ready_task->getOutputFiles()) {
                    if (Simulation::isPageCachingEnabled()) {
                        file_locations[output_file] = FileLocation::LOCATION(this->server_storage_service, this->server_storage_service);
                    } else {
                        file_locations[output_file] = FileLocation::LOCATION(this->server_storage_service);
                    }
                }


                std::map<std::string, std::string> compute_args;
                compute_args[ready_task->getID()] = "1"; // 1 core per task1

                /* Create the job  */
                auto standard_job = job_manager->createStandardJob({ready_task}, file_locations,
                        pre_file_copies, post_file_copies, {});

                // Create service-specific arguments
                std::map<std::string, std::string> batch_service_args;

                //   The job will run no longer than 1 hour
                batch_service_args["-t"] = "60";

                //   The job will run on 1 compute node
                batch_service_args["-N"] = "1";

                //   The job will use 1 core on each compute node
                batch_service_args["-c"] = "1";

                /* Submit the job to the compute service */
                WRENCH_INFO("Submitting the job to the compute service");
                job_manager->submitJob(standard_job, compute_service, compute_args);
            }

            /* Wait for a workflow execution event and process it. In this case we know that
                 * the event will be a StandardJobCompletionEvent, which is processed by the method
                 * processEventStandardJobCompletion() that this class overrides. */

//            printf("Wait for events\n\n");
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
    void NFSPipelineWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task1 */
        auto task = job->getTasks().at(0);WRENCH_INFO("Notified that a standard job has completed task1 %s",
                                                      task->getID().c_str());
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void NFSPipelineWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
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
