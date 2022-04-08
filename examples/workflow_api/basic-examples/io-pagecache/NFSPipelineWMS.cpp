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
     * @param workflow: the workflow to execute
     * @param bare_metal_compute_service: a bare-metal compute service available to run tasks
     * @param client_storage_service: a storage service available to store files
     * @param server_storage_service: a storage service available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    NFSPipelineWMS::NFSPipelineWMS(const std::shared_ptr<Workflow> &workflow,
                                   const std::shared_ptr<ComputeService> &bare_metal_compute_service,
                                   const std::shared_ptr<StorageService> &client_storage_service,
                                   const std::shared_ptr<StorageService> &server_storage_service,
                                   const std::string &hostname) : ExecutionController(hostname, "nfs-pipeline-wms"),
                                                                  workflow(workflow), bare_metal_compute_service(bare_metal_compute_service),
                                                                  client_storage_service(client_storage_service), server_storage_service(server_storage_service) {
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

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO(
                "About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* While the workflow isn't done, repeat the main loop */
        while (not this->workflow->isDone()) {

            std::vector<std::shared_ptr<wrench::WorkflowTask>> ready_tasks = this->workflow->getReadyTasks();

            for (const auto &ready_task: ready_tasks) {

                /* Create a standard job for the task */
                WRENCH_INFO("Creating a job for task %s", ready_task->getID().c_str());

                /* First, we need to create a map of file locations, stating for each file
                 * where is should be read/written */
                std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
                std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> pre_file_copies;
                std::vector<std::tuple<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>>> post_file_copies;

                for (const auto &input_file: ready_task->getInputFiles()) {
                    if (Simulation::isPageCachingEnabled()) {
                        file_locations[input_file] = FileLocation::LOCATION(this->client_storage_service, this->server_storage_service);
                    } else {
                        file_locations[input_file] = FileLocation::LOCATION(this->server_storage_service);
                    }
                }
                for (const auto &output_file: ready_task->getOutputFiles()) {
                    if (Simulation::isPageCachingEnabled()) {
                        file_locations[output_file] = FileLocation::LOCATION(this->server_storage_service, this->server_storage_service);
                    } else {
                        file_locations[output_file] = FileLocation::LOCATION(this->server_storage_service);
                    }
                }


                std::map<std::string, std::string> compute_args;
                compute_args[ready_task->getID()] = "1";// 1 core per task

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
                job_manager->submitJob(standard_job, bare_metal_compute_service, compute_args);
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
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        WRENCH_INFO("Notified that a standard job has completed task %s",
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
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        /* Print some error message */
        WRENCH_INFO("Notified that a standard job has failed for task %s with error %s",
                    task->getID().c_str(),
                    event->failure_cause->toString().c_str());
        throw std::runtime_error("ABORTING DUE TO JOB FAILURE: " + event->failure_cause->toString());
    }


}// namespace wrench
