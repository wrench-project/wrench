/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include <managers/data_movement_manager/DataMovementManager.h>

#include <logging/TerminalOutput.h>
#include <simgrid_S4U_util/S4U_Mailbox.h>
#include <wms/engine/SimpleWMS.h>
#include <simulation/Simulation.h>
#include <workflow_job/StandardJob.h>
#include <workflow_job/PilotJob.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms, "Log category for Simple WMS");

namespace wrench {

    /**
     * @brief Create a Simple WMS with a workflow instance and a scheduler implementation
     *
     * @param simulation: a pointer to a simulation object
     * @param workflow: a pointer to a workflow to execute
     * @param scheduler: a pointer to a scheduler implementation
     * @param hostname: the name of the host
     */
    SimpleWMS::SimpleWMS(Simulation *simulation,
                         Workflow *workflow,
                         std::unique_ptr<Scheduler> scheduler,
                         std::string hostname) : WMS(simulation,
                                                     workflow,
                                                     std::move(scheduler),
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

      TerminalOutput::setThisProcessLoggingColor(WRENCH_LOGGING_COLOR_GREEN);

      WRENCH_INFO("Starting on host %s listening on mailbox %s",
                  S4U_Simulation::getHostName().c_str(),
                  this->mailbox_name.c_str());
      WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

      // Create a job manager
      std::unique_ptr<JobManager> job_manager = std::unique_ptr<JobManager>(new JobManager(this->workflow));

      // Create a data movement manager
      std::unique_ptr<DataMovementManager> data_movement_manager = std::unique_ptr<DataMovementManager>(new DataMovementManager(this->workflow));

      // Perform static optimizations
      runStaticOptimizations();

      while (true) {

        // Take care of previously posted iput() that should be cleared
        S4U_Mailbox::clear_dputs();

        // Get the ready tasks
        std::map<std::string, std::vector<WorkflowTask *>> ready_tasks = this->workflow->getReadyTasks();

        // Get the available compute services
        std::set<ComputeService *> compute_services = this->simulation->getComputeServices();
        if (compute_services.size() == 0) {
          WRENCH_INFO("Aborting - No compute services available!");
          break;
        }

        // Submit pilot jobs
        WRENCH_INFO("Scheduling pilot jobs...");
        double flops = 10000.00; // bogus default
        if (ready_tasks.size() > 0) {
          // Heuristic: ask for something that can run 1.5 times the next ready tasks..
          flops = 1.5 * this->scheduler->getTotalFlops((*ready_tasks.begin()).second);
        }
        this->scheduler->schedulePilotJobs(job_manager.get(), this->workflow, flops,
                                           this->simulation->getComputeServices());

        // Perform dynamic optimizations
        runDynamicOptimizations();

        // Run ready tasks with defined scheduler implementation
        WRENCH_INFO("Scheduling tasks...");
        this->scheduler->scheduleTasks(job_manager.get(),
                                       ready_tasks,
                                       this->simulation->getComputeServices());

        // Wait for a workflow execution event
        std::unique_ptr<WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();

        switch (event->type) {
          case WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            StandardJob *job = (StandardJob *) (event->job);
            WRENCH_INFO("Notified that a %ld-task job has completed", job->getNumTasks());
            break;
          }
          case WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
            StandardJob *job = (StandardJob *) (event->job);
            job_manager->forgetJob(job);
            WRENCH_INFO("Notified that a standard job has failed (it's back in the ready state)");
            break;
          }
          case WorkflowExecutionEvent::PILOT_JOB_START: {
            WRENCH_INFO("Notified that a pilot job has started!");
            break;
          }
          case WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
            WRENCH_INFO("Notified that a pilot job has expired!");
            break;
          }
          case WorkflowExecutionEvent::UNSUPPORTED_JOB_TYPE: {
            WRENCH_INFO("Notified that job '%s' was submitted to a service that doesn't support its job type",
                        event->job->getName().c_str());
            break;
          }
          default: {
            throw std::runtime_error("Unknown workflow execution event type '" +
                                     std::to_string(event->type) + "'");
          }
        }

        if (workflow->isDone()) {
          break;
        }
      }

      S4U_Mailbox::clear_dputs();

      if (workflow->isDone()) {
        WRENCH_INFO("Workflow execution is complete!");
      } else {
        WRENCH_INFO("Workflow execution is incomplete, but there are no more compute services...");
      }

      WRENCH_INFO("Simple WMS Daemon is shutting down all Compute Services");
      this->simulation->shutdownAllComputeServices();

      WRENCH_INFO("Simple WMS Daemon is shutting down all Data Services");
      this->simulation->shutdownAllStorageServices();

      WRENCH_INFO("Simple WMS Daemon is shutting down the File Registry Service");
      this->simulation->getFileRegistryService()->stop();

      /***
       *** NO NEED TO stop/kill the Managers (will soon be out of scope, and
       *** destructor simply called kill() on their actors.
       ***/


      WRENCH_INFO("Simple WMS Daemon started on host %s terminating", S4U_Simulation::getHostName().c_str());

      return 0;
    }

}
