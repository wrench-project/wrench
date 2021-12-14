/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>

#include "SimpleWMS.h"

WRENCH_LOG_CATEGORY(simple_wms, "Log category for Simple WMS");

namespace wrench {

    /**
     * @brief Constructor that creates a Simple WMS with
     *        a scheduler implementation, and a list of compute services
     *
     * @param workflow: a workflow to execute
     * @param batch_compute_service: a batch compute service available to run jobs
     * @param cloud_compute_service: a cloud compute service available to run jobs
     * @param storage_service: a storage service available to store files
     * @param hostname: the name of the host on which to start the WMS
     */
    SimpleWMS::SimpleWMS(const std::shared_ptr<Workflow> &workflow,
                         const std::shared_ptr<BatchComputeService> &batch_compute_service,
                         const std::shared_ptr<CloudComputeService> &cloud_compute_service,
                         const std::shared_ptr<StorageService> &storage_service,
                         const std::string &hostname) :
            ExecutionController(hostname,"simple") {}

    /**
     * @brief main method of the SimpleWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int SimpleWMS::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        WRENCH_INFO("Starting on host %s", S4U_Simulation::getHostName().c_str());
        WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

        // Create a job manager
        auto job_manager = this->createJobManager();

        // Create a data movement manager
        auto data_movement_manager = this->createDataMovementManager();

        // Create and start two VMs on the cloud service to use for the whole execution
        auto vm1 = this->cloud_compute_service->createVM(2, 0.0); // 2 cores, 0 RAM (RAM isn't used in this simulation)
        auto vm1_cs = this->cloud_compute_service->startVM(vm1);

        auto vm2 = this->cloud_compute_service->createVM(4, 0.0); // 4 cores, 0 RAM (RAM isn't used in this simulation)
        auto vm2_cs = this->cloud_compute_service->startVM(vm1);

        // A pilot job to be submitted to the batch compute service
        std::shared_ptr<PilotJob> pilot_job = nullptr;
        bool pilot_job_running = false;

        while (true) {

            // If a pilot job is not running on the batch service, let's submit one that asks
            // for 3 cores on 2 compute nodes for 1 hour
            if (not pilot_job) {
                pilot_job = job_manager->createPilotJob();
                job_manager->submitJob(pilot_job, this->batch_compute_service,
                                       {{"-N","2"}, {"-c","3"}, {"-t", "60"}});
            }

            // Construct the list of currently available bare-metal services (on VMs and perhaps within pilot job as well)
            std::set<std::shared_ptr<BareMetalComputeService>> available_compute_service = {vm1_cs, vm2_cs};
            if (pilot_job_running) {
                available_compute_service.insert(pilot_job->getComputeService());
            }

            // Schedule ready tasks
            for (auto const &task : this->workflow->getReadyTasks()) {
                // If the task cannot be scheduled, then we're out of resources
                // and break out of this loop
                if (not scheduleTask(job_manager, task, available_compute_service)) {
                    break;
                }
            }

            // Wait for a workflow execution event, and process it
            try {
                this->waitForAndProcessNextEvent();
            } catch (ExecutionException &e) {
                WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again",
                            (e.getCause()->toString().c_str()));
                continue;
            }
            if (this->abort || this->workflow->isDone()) {
                break;
            }
        }

        S4U_Simulation::sleep(10);

        WRENCH_INFO("--------------------------------------------------------");
        if (this->workflow->isDone()) {
            WRENCH_INFO("Workflow execution is complete!");
        } else {
            WRENCH_INFO("Workflow execution is incomplete!");
        }

        WRENCH_INFO("Simple WMS Daemon started on host %s terminating", S4U_Simulation::getHostName().c_str());

        return 0;
    }

    /**
     * @brief Process a ExecutionEvent::STANDARD_JOB_FAILURE
     *
     * @param event: a workflow execution event
     */
    void SimpleWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        auto job = event->standard_job;
        WRENCH_INFO("Notified that a standard job has failed (all its tasks are back in the ready state)");
        WRENCH_INFO("CauseType: %s", event->failure_cause->toString().c_str());
        WRENCH_INFO("As a SimpleWMS, I abort as soon as there is a failure");
        this->abort = true;
    }

    /**
     * @brief Helper method to schedule a task one available compute services. This is a very, very
     *        simple/naive scheduling approach, that greedily runs tasks on idle cores of whatever
     *        compute services are available right now, using 1 core per task. Obviously, much more
     *        sophisticated approaches/algorithms are possible. But this is sufficient for the sake
     *        of an example.
     *
     * @param job_manager: a job manager
     * @param task: the task to schedule
     * @param compute_services: available compute services
     * @return
     */
    bool SimpleWMS::scheduleTask(std::shared_ptr<JobManager> job_manager,
                                 std::shared_ptr<WorkflowTask> task,
                                 std::set<std::shared_ptr<BareMetalComputeService>> compute_services) {
        for (auto const &cs : compute_services) {
            if (cs->getTotalNumIdleCores() > 0) {
                auto job = job_manager->createStandardJob(task);
                job_manager->submitJob(job, cs);
                return true;
            }
        }
        return false;
    }

}
