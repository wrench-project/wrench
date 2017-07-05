/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>

#include "exceptions/WorkflowExecutionException.h"
#include "logging/TerminalOutput.h"
#include "managers/data_movement_manager/DataMovementManager.h"
#include "simgrid_S4U_util/S4U_Mailbox.h"
#include "simulation/Simulation.h"
#include "wms/engine/SimpleWMS.h"
#include "workflow_execution_events/WorkflowExecutionFailureCause.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simple_wms, "Log category for Simple WMS");

namespace wrench {

    /**
     * @brief Create a Simple WMS with a workflow instance and a scheduler implementation
     *
     * @param workflow: a workflow to execute
     * @param scheduler: a scheduler implementation
     * @param hostname: the name of the host on which to start the WMS
     */
    SimpleWMS::SimpleWMS(Workflow *workflow,
                         std::unique_ptr<Scheduler> scheduler,
                         std::string hostname) : WMS(workflow,
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
      std::unique_ptr<DataMovementManager> data_movement_manager = std::unique_ptr<DataMovementManager>(
              new DataMovementManager(this->workflow));

      // Perform static optimizations
      runStaticOptimizations();

      bool abort = false;

      while (true) {

        // Take care of previously posted iput() that should be cleared
//        S4U_Mailbox::clear_dputs();

        // Get the ready tasks
        std::map<std::string, std::vector<WorkflowTask *>> ready_tasks = this->workflow->getReadyTasks();

        // Get the available compute services
        std::set<ComputeService *> compute_services = this->simulation->getRunningComputeServices();

        if (compute_services.size() == 0) {
          WRENCH_INFO("Aborting - No compute services available!");
          break;
        }

        // Submit pilot jobs
        if (this->pilot_job_scheduler) {
          WRENCH_INFO("Scheduling pilot jobs...");
          this->pilot_job_scheduler.get()->schedule(this->scheduler.get(), this->workflow, job_manager.get(),
                                                    this->simulation->getRunningComputeServices());
        }

        // Perform dynamic optimizations
        runDynamicOptimizations();

        // Run ready tasks with defined scheduler implementation
        WRENCH_INFO("Scheduling tasks...");
        this->scheduler->scheduleTasks(job_manager.get(),
                                       ready_tasks,
                                       this->simulation->getRunningComputeServices());

        // Wait for a workflow execution event
        std::unique_ptr<WorkflowExecutionEvent> event;
        try {
          event = workflow->waitForNextExecutionEvent();
        } catch (WorkflowExecutionException &e) {
          WRENCH_INFO("Error while getting next execution event (%s)... ignoring and trying again",
                      (e.getCause()->toString().c_str()));
          continue;
        }




        switch (event->type) {
          case WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
            StandardJob *job = (StandardJob *) (event->job);
            WRENCH_INFO("Notified that a %ld-task job has completed", job->getNumTasks());
            break;
          }
          case WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
            StandardJob *job = (StandardJob *) (event->job);
            WRENCH_INFO("Notified that a standard job has failed (all its tasks are back in the ready state)");
            WRENCH_INFO("CauseType: %s", event->failure_cause->toString().c_str());
            job_manager->forgetJob(job);
            WRENCH_INFO("As a SimpleWMS, I abort as soon as there is a failure");
            abort = true;
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
            throw std::runtime_error("SimpleWMS::main(): Unknown workflow execution event type '" +
                                     std::to_string(event->type) + "'");
          }
        }

        if (abort || workflow->isDone()) {
          break;
        }
      }

//      S4U_Mailbox::clear_dputs();

      WRENCH_INFO("--------------------------------------------------------");
      if (workflow->isDone()) {
        WRENCH_INFO("Workflow execution is complete!");
      } else {
        WRENCH_INFO("Workflow execution is incomplete!");
      }

//      // kill all processes
//      // TODO: This could work, but there is a problem with killAll() in S4U
//      simgrid::s4u::Actor::killAll();
//      return 0;

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
