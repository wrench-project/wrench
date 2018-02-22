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

XBT_LOG_NEW_DEFAULT_CATEGORY(wms, "Log category for WMS");

namespace wrench {

    /**
     * @brief Create a WMS with a workflow instance, a scheduler implementation, and a list of compute services
     *
     * @param scheduler: a scheduler implementation
     * @param compute_services: a set of compute services available to run jobs
     * @param storage_services: a set of storage services available to the WMS
     * @param hostname: the name of the host on which to run the WMS
     * @param suffix: a string to append to the process name

     *
     * @throw std::invalid_argument
     */
    WMS::WMS(std::unique_ptr<Scheduler> scheduler,
             const std::set<ComputeService *> &compute_services,
             const std::set<StorageService *> &storage_services,
             const std::string &hostname,
             const std::string suffix) :
            S4U_Daemon("wms_" + suffix, "wms_" + suffix),
            compute_services(std::move(compute_services)),
            storage_services(std::move(storage_services)),
            scheduler(std::move(scheduler)) {

      this->hostname = hostname;
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
     * @brief Set a pilot job scheduler strategy
     *
     * @param pilot_job_scheduler: a pilot job scheduler implementation
     */
    void WMS::setPilotJobScheduler(std::unique_ptr<PilotJobScheduler> pilot_job_scheduler) {
      this->pilot_job_scheduler = std::move(pilot_job_scheduler);
    }

    /**
     * @brief Check whether the WMS has a deferred start simulation time
     *
     * @throw std::runtime_error
     */
    void WMS::checkDeferredStart() {
      if (S4U_Simulation::getClock() < this->start_time) {
        new Alarm(this->start_time, this->hostname, this->mailbox_name,
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

        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto *msg = dynamic_cast<AlarmWMSDeferredStartMessage *>(message.get())) {
          // The WMS can be started
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
     * @brief Obtain the list of compute services
     *
     * @return a set of compute services
     */
    std::set<ComputeService *> WMS::getRunningComputeServices() {
      std::set<ComputeService *> set = {};
      for (auto compute_service : this->compute_services) {
        if (compute_service->isUp()) {
          set.insert(compute_service);
        }
      }
      return set;
    }

    /**
     * @brief Shutdown all services
     */
    void WMS::shutdownAllServices() {
      WRENCH_INFO("WMS %s Daemon is shutting down all Compute Services", this->getName().c_str());
      this->simulation->getTerminator()->shutdownComputeService(this->compute_services);

      WRENCH_INFO("WMS %s Daemon is shutting down all Storage Services", this->getName().c_str());
      this->simulation->getTerminator()->shutdownStorageService(this->storage_services);

      WRENCH_INFO("WMS %s Daemon is shutting down the File Registry Service", this->getName().c_str());
      this->simulation->getTerminator()->shutdownFileRegistryService(this->simulation->getFileRegistryService());

      WRENCH_INFO("WMS %s Daemon is shutting down the Network Proximity Service", this->getName().c_str());
      this->simulation->getTerminator()->shutdownNetworkProximityService(
              this->simulation->getNetworkProximityService());
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
     * @brief Set the simulation
     *
     * @param simulation: the current simulation
     *
     * @throw std::invalid_argument
     */
    void WMS::setSimulation(Simulation *simulation) {
      this->simulation = simulation;
    }

    /**
     * @brief Start the WMS daemon
     *
     * @throw std::runtime_error
     */
    void WMS::start() {
      // Start the daemon
      try {
        this->start_daemon(this->hostname);
      } catch (std::invalid_argument &e) {
        throw std::runtime_error("WMS:start(): " + std::string(e.what()));
      }
      // register services into terminator (the storage service is obtained from the compute service)
      this->simulation->getTerminator()->registerComputeService(this->compute_services);
      this->simulation->getTerminator()->registerStorageService(this->storage_services);
      this->simulation->getTerminator()->registerFileRegistryService(this->simulation->getFileRegistryService());
      this->simulation->getTerminator()->registerNetworkProximityService(
              this->simulation->getNetworkProximityService());
    }

    /**
     * @brief Get the name of the host on which the WMS is running
     *
     * @return the hostname
     */
    std::string WMS::getHostname() {
      return this->hostname;
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


};
