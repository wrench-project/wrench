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
     * @param standard_job_scheduler: a standard job scheduler implementation (if nullptr none is used)
     * @param pilot_job_scheduler: a pilot job scheduler implementation if nullptr none is used)
     * @param compute_services: a set of compute services available to run jobs
     * @param storage_services: a set of storage services available to the WMS
     * @param hostname: the name of the host on which to start the WMS
     */
    SimpleWMS::SimpleWMS(std::unique_ptr<StandardJobScheduler> standard_job_scheduler,
                         std::unique_ptr<PilotJobScheduler> pilot_job_scheduler,
                         const std::set<std::shared_ptr<ComputeService>> &compute_services,
                         const std::set<std::shared_ptr<StorageService>> &storage_services,
                         const std::string &hostname) : WMS(
            std::move(standard_job_scheduler),
            std::move(pilot_job_scheduler),
            compute_services,
            storage_services,
            {}, nullptr,
            hostname,
            "simple") {}

    /**
     * @brief main method of the SimpleWMS daemon
     *
     * @return 0 on completion
     *
     * @throw std::runtime_error
     */
    int SimpleWMS::main() {

      TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_GREEN);
      
      // Check whether the WMS has a deferred start time
      checkDeferredStart();

      WRENCH_INFO("Starting on host %s", S4U_Simulation::getHostName().c_str());
      WRENCH_INFO("About to execute a workflow with %lu tasks", this->getWorkflow()->getNumberOfTasks());

      // Create a job manager
      this->job_manager = this->createJobManager();

      // Create a data movement manager
      std::shared_ptr<DataMovementManager> data_movement_manager = this->createDataMovementManager();

      // Perform static optimizations
      runStaticOptimizations();

      while (true) {

        // Get the ready tasks
        std::vector<std::shared_ptr<WorkflowTask>> ready_tasks = this->getWorkflow()->getReadyTasks();

        // Get the available compute services
        auto compute_services = this->getAvailableComputeServices<ComputeService>();

        if (compute_services.empty()) {
          WRENCH_INFO("Aborting - No compute services available!");
          break;
        }

        // Submit pilot jobs
        if (this->getPilotJobScheduler()) {
          WRENCH_INFO("Scheduling pilot jobs...");
          this->getPilotJobScheduler()->schedulePilotJobs(this->getAvailableComputeServices<ComputeService>());
        }

        // Perform dynamic optimizations
        runDynamicOptimizations();

        // Run ready tasks with defined scheduler implementation
        WRENCH_INFO("Scheduling tasks...");
        this->getStandardJobScheduler()->scheduleTasks(this->getAvailableComputeServices<ComputeService>(), ready_tasks);

        // Wait for a workflow execution event, and process it
        try {
          this->waitForAndProcessNextEvent();
        } catch (ExecutionException &e) {
          WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again",
                      (e.getCause()->toString().c_str()));
          continue;
        }
        if (this->abort || this->getWorkflow()->isDone()) {
          break;
        }
      }

      S4U_Simulation::sleep(10);

      WRENCH_INFO("--------------------------------------------------------");
      if (this->getWorkflow()->isDone()) {
        WRENCH_INFO("Workflow execution is complete!");
      } else {
        WRENCH_INFO("Workflow execution is incomplete!");
      }

      WRENCH_INFO("Simple WMS Daemon started on host %s terminating", S4U_Simulation::getHostName().c_str());

      this->job_manager.reset();

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

}
