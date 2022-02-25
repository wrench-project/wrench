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
 **    - Pick one ready task
 **    - Submit it as part of a single job to the one available bare-metal compute service so that:
 **       - The task uses 10 cores
 **    - Wait for the task's completion
 **/

#include <iostream>

#include "OneTaskAtATimeWMS.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for OneTaskAtATimeWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param workflow: the workflow to execute
     * @param bare_metal_compute_service: a bare-metal compute services available to run tasks
     * @param hostname: the name of the host on which to start the WMS
     */
    OneTaskAtATimeWMS::OneTaskAtATimeWMS(const std::shared_ptr<Workflow>& workflow,
                                         const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                                         const std::string &hostname) :
                                         ExecutionController(hostname,"one-task-at-a-time"),
                                         workflow(workflow), bare_metal_compute_service(bare_metal_compute_service) {}

    /**
     * @brief main method of the OneTaskAtATimeWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int OneTaskAtATimeWMS::main() {

        /* Set the logging output to GREEN */
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("WMS starting on host %s", Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* While the workflow isn't done, repeat the main loop */
        while (not this->workflow->isDone()) {

            /* Get the next ready task */
            auto ready_task = *(this->workflow->getReadyTasks().begin());

            WRENCH_INFO("Creating a job for task %s", ready_task->getID().c_str());

            /* Create the job  */
            auto standard_job = job_manager->createStandardJob(ready_task);

            /* No need to use service specific arguments to specify a number of cores for the task.
             * By default, the compute service will run the task with the largest possible number of
             * cores. */

            WRENCH_INFO("Submitting the job to the compute service");

            /* Submit the job to the compute service */
            job_manager->submitJob(standard_job, bare_metal_compute_service);

            /* Wait for a workflow execution event and process it. In this case we know that
             * the event will be a StandardJobCompletionEvent, which is processed by the method
             * processEventStandardJobCompletion() that this class overrides. */
            WRENCH_INFO("Waiting for next event");
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
    void OneTaskAtATimeWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        /* Retrieve the job's tasks */
        for (auto const &task : job->getTasks()) {
            WRENCH_INFO("Notified that a standard job has completed task %s",
                        task->getID().c_str());
        }
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: the event
     */
    void OneTaskAtATimeWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        /* Retrieve the job that this event is for */
        auto job = event->standard_job;
        WRENCH_INFO("Notified that a standard job has failed (failure cause: %s)",
                    event->failure_cause->toString().c_str());
        /* Retrieve the job's tasks */
        WRENCH_INFO("As a result, the following tasks have failed:");
        for (auto const &task : job->getTasks()) {
            WRENCH_INFO(" - %s", task->getID().c_str());
        }
        throw std::runtime_error("This should not happen in this example");
    }


}
