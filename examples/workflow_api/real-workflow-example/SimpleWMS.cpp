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
            ExecutionController(hostname,"simple"),
            workflow(workflow),
            batch_compute_service(batch_compute_service),
            cloud_compute_service(cloud_compute_service),
            storage_service(storage_service) {}

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
        this->core_utilization_map[vm1_cs] = 2;

        auto vm2 = this->cloud_compute_service->createVM(4, 0.0); // 4 cores, 0 RAM (RAM isn't used in this simulation)
        auto vm2_cs = this->cloud_compute_service->startVM(vm2);
        this->core_utilization_map[vm2_cs] = 4;


        while (true) {

            // If a pilot job is not running on the batch service, let's submit one that asks
            // for 3 cores on 2 compute nodes for 1 hour
            if (not pilot_job) {
                WRENCH_INFO("Creating and submitting a pilot job");
                pilot_job = job_manager->createPilotJob();
                job_manager->submitJob(pilot_job, this->batch_compute_service,
                                       {{"-N","2"}, {"-c","3"}, {"-t", "30"}});
            }

            // Construct the list of currently available bare-metal services (on VMs and perhaps within pilot job as well)
            std::set<std::shared_ptr<BareMetalComputeService>> available_compute_service = {vm1_cs, vm2_cs};
            if (this->pilot_job_is_running) {
                available_compute_service.insert(pilot_job->getComputeService());
            }

            scheduleReadyTasks(workflow->getReadyTasks(), job_manager, available_compute_service);

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

        WRENCH_INFO("WMS terminating");

        return 0;
    }

    /**
     * @brief Process a StandardJobFailedEvent
     *
     * @param event: a workflow execution event
     */
    void SimpleWMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        auto job = event->standard_job;
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);
        WRENCH_INFO("Task %s has failed", (*job->getTasks().begin())->getID().c_str());
        WRENCH_INFO("failure cause: %s", event->failure_cause->toString().c_str());
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

    }

    /**
    * @brief Process a StandardJobCompletedEvent
    *
    * @param event: a workflow execution event
    */
    void SimpleWMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        auto job = event->standard_job;
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("Task %s has COMPLETED (on service %s)",
                    (*job->getTasks().begin())->getID().c_str(),
                    job->getParentComputeService()->getName().c_str());
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        this->core_utilization_map[job->getParentComputeService()]++;
    }


    /**
    * @brief Process a PilotJobStartedEvent event
    *
    * @param event: a workflow execution event
    */
    void SimpleWMS::processEventPilotJobStart(std::shared_ptr<PilotJobStartedEvent> event) {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_BLUE);
        WRENCH_INFO("The pilot job has started (it exposes bare-metal compute service %s)",
                    event->pilot_job->getComputeService()->getName().c_str());
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
        this->pilot_job_is_running = true;
        this->core_utilization_map[this->pilot_job->getComputeService()] = event->pilot_job->getComputeService()->getTotalNumIdleCores();

    }

    /**
    * @brief Process a processEventPilotJobExpiration even
    *
    * @param event: a workflow execution event
    */
    void SimpleWMS::processEventPilotJobExpiration(std::shared_ptr<PilotJobExpiredEvent> event) {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_RED);
        WRENCH_INFO("The pilot job has expired (it was exposing bare-metal compute service %s)",
                    event->pilot_job->getComputeService()->getName().c_str());
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);

        this->pilot_job_is_running = false;
        this->core_utilization_map.erase(this->pilot_job->getComputeService());
        this->pilot_job = nullptr;
    }

    /**
     * @brief Helper method to schedule a task one available compute services. This is a very, very
     *        simple/naive scheduling approach, that greedily runs tasks on idle cores of whatever
     *        compute services are available right now, using 1 core per task. Obviously, much more
     *        sophisticated approaches/algorithms are possible. But this is sufficient for the sake
     *        of an example.
     *
     * @param ready_task: the ready tasks to schedule
     * @param job_manager: a job manager
     * @param compute_services: available compute services
     * @return
     */
    void SimpleWMS::scheduleReadyTasks(std::vector<std::shared_ptr<WorkflowTask>> ready_tasks,
                                       std::shared_ptr<JobManager> job_manager,
                                       std::set<std::shared_ptr<BareMetalComputeService>> compute_services) {

        if (ready_tasks.empty()) {
            return;
        }

        WRENCH_INFO("Trying to schedule %lu ready tasks", ready_tasks.size());

        unsigned long num_tasks_scheduled = 0;
        for (auto const &task : ready_tasks) {
            bool scheduled = false;
            for (auto const &cs : compute_services) {
                if (this->core_utilization_map[cs] > 0) {
                    // Specify that ALL files are read/written from the one storage service
                    std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> file_locations;
                    for (auto const &f : task->getInputFiles()) {
                        file_locations[f] = wrench::FileLocation::LOCATION(this->storage_service);
                    }
                    for (auto const &f : task->getOutputFiles()) {
                        file_locations[f] = wrench::FileLocation::LOCATION(this->storage_service);
                    }
                    try {
                        auto job = job_manager->createStandardJob(task, file_locations);WRENCH_INFO(
                                "Submitting task %s to compute service %s", task->getID().c_str(),
                                cs->getName().c_str());
                        job_manager->submitJob(job, cs);
                        this->core_utilization_map[cs]--;
                        num_tasks_scheduled++;
                        scheduled = true;
                    } catch (ExecutionException &e) {
                        WRENCH_INFO("WARNING: Was not able to submit task %s, likely due to the pilot job having expired "
                                    "(I should get a notification of its expiration soon)", task->getID().c_str());
                    }
                    break;
                }
            }
            if (not scheduled) break;
        }
        WRENCH_INFO("Was able to schedule %lu out of %lu ready tasks", num_tasks_scheduled, ready_tasks.size());
    }

}
