/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/**
 ** An execution controller to execute a workflow
 **/

#include <iostream>

#include "OneTaskAtATimeWMS.h"

WRENCH_LOG_CATEGORY(controller, "Log category for Controller");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param workflow: the workflow to execute
     * @param bare_metal_compute_service: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    Controller::Controller(std::shared_ptr<Workflow> workflow,
                           const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                           const std::shared_ptr<SimpleStorageService> &storage_service,
                           const std::string &hostname) :
            ExecutionController(hostname,"controller"),
            workflow(workflow), bare_metal_compute_service(bare_metal_compute_service), storage_service(storage_service) {}

    /**
     * @brief main method of the OneTaskAtATimeWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int Controller::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Controller starting");
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* While the workflow isn't done, repeat the main loop */
        while (not this->workflow->isDone()) {

            /* Get one ready task */
            auto ready_task = this->workflow->getReadyTasks().at(0);

            /* Create a standard job for the task */
            WRENCH_INFO("Creating a job for task %s", ready_task->getID().c_str());

            /* Create a map of file locations, stating for each file (could be none)
             * where is should be read/written */
            std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
            for (auto const &f : ready_task->getInputFile()) {
                file_locations[f] = FileLocation::LOCATION(storage_service);
            }
            for (auto const &f : ready_task->getOutputFile()) {
                file_locations[f] = FileLocation::LOCATION(storage_service);
            }

            /* Create the job  */
            auto standard_job = job_manager->createStandardJob(ready_task, file_locations);

            /* Submit the job to the compute service */
            WRENCH_INFO("Submitting the job to the compute service");
            job_manager->submitJob(standard_job, bare_metal_compute_service);

            /* Wait for a workflow execution event and process it */
            this->waitForAndProcessNextEvent();
        }

        WRENCH_INFO("Workflow execution complete!");
        return 0;
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: the event
     */
    void OneTaskAtATimeWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's first (and in our case only) task */
        auto task = job->getTasks().at(0);
        WRENCH_INFO("Notified that a standard job has completed task %s", task->getID().c_str());
    }
}
