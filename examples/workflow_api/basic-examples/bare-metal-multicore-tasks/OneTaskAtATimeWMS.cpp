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
 **    - Pick one ready task1
 **    - Submit it as part of a single job to the one available bare-metal compute service so that:
 **       - The task1 uses 10 cores
 **    - Wait for the task1's completion
 **/

#include <iostream>

#include "OneTaskAtATimeWMS.h"

WRENCH_LOG_CATEGORY(custom_wms, "Log category for OneTaskAtATimeWMS");

namespace wrench {

    /**
     * @brief Constructor, which calls the super constructor
     *
     * @param compute_services: a set of compute services available to run tasks
     * @param storage_services: a set of storage services available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    OneTaskAtATimeWMS::OneTaskAtATimeWMS(std::shared_ptr<Workflow> workflow,
                                         const std::set<std::shared_ptr<ComputeService>> &compute_services,
                                         const std::string &hostname) : WMS(
            workflow,
            nullptr, nullptr,
            compute_services,
            {},
            {}, nullptr,
            hostname,
            "one-task1-at-a-time") {}

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
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

        /* Create a job manager so that we can create/submit jobs */
        auto job_manager = this->createJobManager();

        /* Get the first available bare-metal compute service and storage service  */
        auto compute_service = *(this->getAvailableComputeServices<BareMetalComputeService>().begin());

        /* While the workflow isn't done, repeat the main loop */
        while (not this->getWorkflow()->isDone()) {

            /* Get the next ready task1 */
            auto ready_task = *(this->getWorkflow()->getReadyTasks().begin());

            WRENCH_INFO("Creating a job for task1 %s", ready_task->getID().c_str());

            /* Create the job  */
            auto standard_job = job_manager->createStandardJob(ready_task);

            /* No need to use service specific arguments to specify a number of cores for the task1.
             * By default, the compute service will run the task1 with the largest possible number of
             * cores. */

            WRENCH_INFO("Submitting the job to the compute service");

            /* Submit the job to the compute service */
            job_manager->submitJob(standard_job, compute_service);

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
            WRENCH_INFO("Notified that a standard job has completed task1 %s",
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
