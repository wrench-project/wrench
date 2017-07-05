/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "exceptions/WorkflowExecutionException.h"
#include "logging/TerminalOutput.h"
#include "wms/WMS.h"
#include "workflow_job/StandardJob.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(wms, "Log category for WMS");

namespace wrench {

    /**
     * @brief Create a WMS with a workflow instance and a scheduler implementation
     *
     * @param workflow: a workflow to execute
     * @param scheduler: a scheduler implementation
     * @param hostname: the name of the host on which to run the WMS
     * @param suffix: a string to append to the process name
     *
     * @throw std::invalid_argument
     */
    WMS::WMS(Workflow *workflow,
             std::unique_ptr<Scheduler> scheduler,
             std::string hostname,
             std::string suffix) :
            S4U_DaemonWithMailbox("wms_" + suffix, "wms_" + suffix),
            workflow(workflow),
            scheduler(std::move(scheduler)) {

      this->hostname = hostname;
    }

    /**
     * @brief Add a dynamic optimization to the list of optimizations. Optimizations are
     * executed in order of insertion
     *
     * @param optimization: a dynamic optimization implementation
     */
    void WMS::addDynamicOptimization(std::unique_ptr<DynamicOptimization> optimization) {
      this->dynamic_optimizations.push_back(std::move(optimization));
    }

    /**
     * @brief Add a static optimization to the list of optimizations. Optimizations are
     * executed in order of insertion
     *
     * @param optimization: a static optimization implementation
     */
    void WMS::addStaticOptimization(std::unique_ptr<StaticOptimization> optimization) {
      this->static_optimizations.push_back(std::move(optimization));
    }

    /**
     * @brief Set a pilot job scheduler strategy
     *
     * @param pilot_job_scheduler: a pilot job scheduler implementation
     */
    void WMS::setPilotJobScheduler(std::unique_ptr<PilotJobScheduler> pilot_job_scheduler) {
      this->pilot_job_scheduler = std::move(pilot_job_scheduler);
    }

    /**
     * @brief Perform dynamic optimizations. Optimizations are executed in order of insertion
     */
    void WMS::runDynamicOptimizations() {
      for (auto &opt : this->dynamic_optimizations) {
        opt.get()->process(this->workflow);
      }
    }

    /**
     * @brief Perform static optimizations. Optimizations are executed in order of insertion
     */
    void WMS::runStaticOptimizations() {
      for (auto &opt : this->static_optimizations) {
        opt.get()->process(this->workflow);
      }
    }

    /**
     * @brief Wait for a workflow execution event and then call the associated function to process
     *
     * @throw wrench::WorkflowExecutionException
     */
    void WMS::waitForAndProcessNextEvent() {
      std::unique_ptr<WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();

      switch (event->type) {
        case WorkflowExecutionEvent::UNSUPPORTED_JOB_TYPE: {
          processEventUnsupportedJobType(std::move(event));
          break;
        }
        case WorkflowExecutionEvent::STANDARD_JOB_COMPLETION: {
          processEventStandardJobCompletion(std::move(event));
          break;
        }
        case WorkflowExecutionEvent::STANDARD_JOB_FAILURE: {
          processEventStandardJobFailure(std::move(event));
          break;
        }
        case WorkflowExecutionEvent::PILOT_JOB_START: {
          processEventPilotJobStart(std::move(event));
          break;
        }
        case WorkflowExecutionEvent::PILOT_JOB_EXPIRATION: {
          processEventPilotJobExpiration(std::move(event));
          break;
        }
        case WorkflowExecutionEvent::FILE_COPY_COMPLETION: {
          processEventFileCopyCompletion(std::move(event));
          break;
        }
        case WorkflowExecutionEvent::FILE_COPY_FAILURE: {
          processEventFileCopyFailure(std::move(event));
          break;
        }
        default: {
          throw std::runtime_error("SimpleWMS::main(): Unknown workflow execution event type '" +
                                   std::to_string(event->type) + "'");
        }
      }
    }

    /**
     * @brief Process a WorkflowExecutionEvent::UNSUPPORTED_JOB_TYPE
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventUnsupportedJobType(std::unique_ptr<WorkflowExecutionEvent> event) {
      WRENCH_INFO("Notified that job '%s' was submitted to a service that does not support its job type",
                  event->job->getName().c_str());
    }

    /**
     * @brief Process a WorkflowExecutionEvent::STANDARD_JOB_COMPLETION
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventStandardJobCompletion(std::unique_ptr<WorkflowExecutionEvent> event) {
      StandardJob *job = (StandardJob *) (event->job);
      WRENCH_INFO("Notified that a %ld-task job has completed", job->getNumTasks());
    }

    /**
     * @brief Process a WorkflowExecutionEvent::STANDARD_JOB_FAILURE
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventStandardJobFailure(std::unique_ptr<WorkflowExecutionEvent> event) {
      WRENCH_INFO("Notified that a standard job has failed (all its tasks are back in the ready state)");
    }

    /**
     * @brief Process a WorkflowExecutionEvent::PILOT_JOB_START
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventPilotJobStart(std::unique_ptr<WorkflowExecutionEvent> event) {
      WRENCH_INFO("Notified that a pilot job has started!");
    }

    /**
     * @brief Process a WorkflowExecutionEvent::PILOT_JOB_EXPIRATION
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventPilotJobExpiration(std::unique_ptr<WorkflowExecutionEvent> event) {
      WRENCH_INFO("Notified that a pilot job has expired!");
    }

    /**
     * @brief Process a WorkflowExecutionEvent::FILE_COPY_COMPLETION
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventFileCopyCompletion(std::unique_ptr<WorkflowExecutionEvent> event) {
      WRENCH_INFO("Notified that a file copy is completed!");
    }

    /**
     * @brief Process a WorkflowExecutionEvent::FILE_COPY_FAILURE
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventFileCopyFailure(std::unique_ptr<WorkflowExecutionEvent> event) {
      WRENCH_INFO("Notified that a file copy has failed!");
    }

    /**
     * @brief Set the simulation
     *
     * @param simulation: the current simulation
     *
     * @throw std::invalid_argument
     */
    void WMS::setSimulation(Simulation *simulation) {
      this->simulation = simulation;

      // Start the daemon
      try {
        this->start(this->hostname);
      } catch (std::invalid_argument &e) {
        throw;
      }
    }

    /**
     * @brief Get the name of the host on which the WMS is running
     *
     * @return the hostname
     */
    std::string WMS::getHostname() {
      return this->hostname;
    }

};
