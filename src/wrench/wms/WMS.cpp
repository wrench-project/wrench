/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/helpers/Alarm.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simulation/Simulation.h"
#include "wms/WMSMessage.h"
#include "wrench/managers/JobManager.h"
#include "wrench/managers/DataMovementManager.h"
#include "wrench/services/compute/ComputeService.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(wms, "Log category for WMS");

namespace wrench {

    /**
     * @brief Constructor:  a WMS with a workflow instance, a scheduler implementation, and a list of compute services
     *
     * @param scheduler: a standard job scheduler implementation (if nullptr then none is used)
     * @param scheduler: a pilot job scheduler implementation (if nullptr then none is used)
     * @param compute_services: a set of compute services available to run jobs (if {} then none is available)
     * @param storage_services: a set of storage services available to the WMS (if {} then none is available)
     * @param network_proximity_services: a set of network proximity services available to the WMS (if {} then none is available)
     * @param file_registry_service: the file registry services available to the WMS (if nullptr then none is available)
     * @param hostname: the name of the host on which to run the WMS
     * @param suffix: a string to append to the WMS process name (useful for debug output)

     *
     * @throw std::invalid_argument
     */
    WMS::WMS(std::unique_ptr<StandardJobScheduler> standard_job_scheduler,
             std::unique_ptr<PilotJobScheduler> pilot_job_scheduler,
             const std::set<ComputeService *> &compute_services,
             const std::set<StorageService *> &storage_services,
             const std::set<NetworkProximityService *> &network_proximity_services,
             FileRegistryService *file_registry_service,
             const std::string &hostname,
             const std::string suffix) :
            Service(hostname, "wms_" + suffix, "wms_" + suffix),
            compute_services(compute_services),
            storage_services(storage_services),
            network_proximity_services(network_proximity_services),
            file_registry_service(file_registry_service),
            standard_job_scheduler(std::move(standard_job_scheduler)),
            pilot_job_scheduler(std::move(pilot_job_scheduler))
    {

      this->workflow = nullptr;
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
     * @brief Check whether the WMS has a deferred start simulation time
     *
     * @throw std::runtime_error
     */
    void WMS::checkDeferredStart() {
      if (S4U_Simulation::getClock() < this->start_time) {

        Alarm::createAndStartAlarm(this->start_time, this->hostname, this->mailbox_name,
                                   new AlarmWMSDeferredStartMessage(this->mailbox_name, this->start_time, 0), "wms_start");

        // Wait for a message
        std::unique_ptr<SimulationMessage> message = nullptr;

        try {
          message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
          throw std::runtime_error(cause->toString());
        } catch (std::shared_ptr<NetworkTimeout> &cause) {
          throw std::runtime_error(cause->toString());
        }

        if (message == nullptr) {
          std::runtime_error("Got a NULL message... Likely this means the WMS cannot be started. Aborting!");
        }

        if (auto msg = dynamic_cast<AlarmWMSDeferredStartMessage *>(message.get())) {
          // The WMS can be started
        } else {
          throw std::runtime_error("WMS::checkDeferredStart(): Unexpected " + message->getName() + " message");
        }
      }
    }

    /**
     * @brief Perform dynamic optimizations. Optimizations are executed in order of insertion
     */
    void WMS::runDynamicOptimizations() {
      for (auto &opt : this->dynamic_optimizations) {
        opt->process(this->workflow);
      }
    }

    /**
     * @brief Perform static optimizations. Optimizations are executed in order of insertion
     */
    void WMS::runStaticOptimizations() {
      for (auto &opt : this->static_optimizations) {
        opt->process(this->workflow);
      }
    }

    /**
     * @brief Obtain the list of compute services available to the WMS
     *
     * @return a set of compute services
     */
    std::set<ComputeService *> WMS::getAvailableComputeServices() {
      return this->compute_services;
    }

    /**
    * @brief Obtain the list of storage services available to the WMS
    *
    * @return a set of storage services
    */
    std::set<StorageService *> WMS::getAvailableStorageServices() {
      return this->storage_services;
    }

    /**
    * @brief Obtain the list of network proximity services available to the WMS
    *
    * @return a set of network proximity services
    */
    std::set<NetworkProximityService *> WMS::getAvailableNetworkProximityServices() {
      return this->network_proximity_services;
    }

    /**
    * @brief Obtain the file registry service available to the WMS
    *
    * @return a file registry services
    */
    FileRegistryService * WMS::getAvailableFileRegistryService() {
      return this->file_registry_service;
    }

    /**
     * @brief Wait for a workflow execution event and then call the associated function to process
     *
     * @throw wrench::WorkflowExecutionException
     */
    void WMS::waitForAndProcessNextEvent() {
      std::unique_ptr<WorkflowExecutionEvent> event = workflow->waitForNextExecutionEvent();

      switch (event->type) {
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
     * @brief Process a WorkflowExecutionEvent::STANDARD_JOB_COMPLETION
     *
     * @param event: a workflow execution event
     */
    void WMS::processEventStandardJobCompletion(std::unique_ptr<WorkflowExecutionEvent> event) {
      auto job = (StandardJob *) (event->job);
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
     * @brief Set the workflow to be executed by the WMS
     * @param workflow: a workflow to execute
     * @param start_time: the simulated time when the WMS should start executed the workflow (0 if not specified)
     *
     * @throw std::invalid_argument
     */
    void WMS::addWorkflow(Workflow *workflow, double start_time) {

      if ((workflow == nullptr) || (start_time < 0.0)) {
        throw std::invalid_argument("WMS::addWorkflow(): Invalid arguments");
      }

      if (this->workflow) {
        throw std::invalid_argument("WMS::addWorkflow(): The WMS has already been given a workflow");
      } else {
        this->workflow = workflow;
      }
      this->start_time = start_time;
    }

    /**
     * @brief Get the workflow that was assigned to the WMS
     *
     * @return a workflow
     */
    Workflow *WMS::getWorkflow() {
      return this->workflow;
    }

    /**
     * @brief Instantiate a job manager
     * @return a job manager
     */
    std::shared_ptr<JobManager> WMS::createJobManager() {
      auto job_manager_raw_ptr = new JobManager(this);
      std::shared_ptr<JobManager> job_manager = std::shared_ptr<JobManager>(job_manager_raw_ptr);
      job_manager->start(job_manager, true); // Always daemonize
      return job_manager;
    }

    /**
     * @brief Instantiate a data movement manager
     * @return a data movement manager
     */
    std::shared_ptr<DataMovementManager> WMS::createDataMovementManager() {
      auto data_movement_manager_raw_ptr = new DataMovementManager(this);
      std::shared_ptr<DataMovementManager> data_movement_manager = std::shared_ptr<DataMovementManager>(data_movement_manager_raw_ptr);
      data_movement_manager->start(data_movement_manager, true); // Always daemonize
      return data_movement_manager;
    }

};
