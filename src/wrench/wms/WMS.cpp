/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/wms/WMS.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/helper_services/alarm/Alarm.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/Simulation.h>
#include "wms/WMSMessage.h"
#include <wrench/managers/JobManager.h>
#include <wrench/managers/DataMovementManager.h>
#include <wrench/services/compute/ComputeService.h>
#include <wrench/services/compute/bare_metal/BareMetalComputeService.h>
#include <wrench/services/compute/batch/BatchComputeService.h>
#include <wrench/failure_causes/NetworkError.h>

WRENCH_LOG_CATEGORY(wrench_core_wms, "Log category for WMS");

namespace wrench {

    /**
     * @brief Constructor:  a WMS with a workflow instance, a scheduler implementation, and a list of compute services
     *
     * @param standard_job_scheduler: a standard job scheduler implementation (if nullptr then none is used)
     * @param pilot_job_scheduler: a pilot job scheduler implementation (if nullptr then none is used)
     * @param compute_services: a set of compute services available to run jobs (if {} then none is available)
     * @param storage_services: a set of storage services available to the WMS (if {} then none is available)
     * @param network_proximity_services: a set of network proximity services available to the WMS (if {} then none is available)
     * @param file_registry_service: a file registry services available to the WMS (if nullptr then none is available)
     * @param hostname: the name of the host on which to run the WMS
     * @param suffix: a string to append to the WMS process name (useful for debug output)
     *
     * @throw std::invalid_argument
     */
    WMS::WMS(std::unique_ptr<StandardJobScheduler> standard_job_scheduler,
             std::unique_ptr<PilotJobScheduler> pilot_job_scheduler,
             const std::set<std::shared_ptr<ComputeService>> &compute_services,
             const std::set<std::shared_ptr<StorageService>> &storage_services,
             const std::set<std::shared_ptr<NetworkProximityService>> &network_proximity_services,
             std::shared_ptr<FileRegistryService> file_registry_service,
             const std::string &hostname,
             const std::string suffix) :
            Service(hostname, "wms_" + suffix, "wms_" + suffix),
            compute_services(compute_services),
            storage_services(storage_services),
            network_proximity_services(network_proximity_services),
            file_registry_service(file_registry_service),
            standard_job_scheduler(std::move(standard_job_scheduler)),
            pilot_job_scheduler(std::move(pilot_job_scheduler)) {
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
     * @brief Check whether the WMS has a deferred start simulation time (likely the
     *        first call in the main() routine of any WMS
     *
     * @throw std::runtime_error
     */
    void WMS::checkDeferredStart() {
        if (S4U_Simulation::getClock() < this->start_time) {

            Alarm::createAndStartAlarm(this->simulation, this->start_time, this->hostname, this->mailbox_name,
                                       new AlarmWMSDeferredStartMessage(0), "wms_start");

            // Wait for a message
            std::unique_ptr<SimulationMessage> message = nullptr;

            try {
                message = S4U_Mailbox::getMessage(this->mailbox_name);
            } catch (std::shared_ptr<NetworkError> &cause) {
                throw std::runtime_error(cause->toString());
            }

            if (message == nullptr) {
                std::runtime_error("Got a NULL message... Likely this means the WMS cannot be started. Aborting!");
            }

            if (auto msg = dynamic_cast<AlarmWMSDeferredStartMessage*>(message.get())) {
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
    * @brief Obtain the list of storage services available to the WMS
    *
    * @return a set of storage services
    */
    std::set<std::shared_ptr<StorageService>> WMS::getAvailableStorageServices() {
        return this->storage_services;
    }

    /**
    * @brief Obtain the list of network proximity services available to the WMS
    *
    * @return a set of network proximity services
    */
    std::set<std::shared_ptr<NetworkProximityService>> WMS::getAvailableNetworkProximityServices() {
        return this->network_proximity_services;
    }

    /**
    * @brief Obtain the file registry service available to the WMS
    *
    * @return a file registry services
    */
    std::shared_ptr<FileRegistryService> WMS::getAvailableFileRegistryService() {
        return this->file_registry_service;
    }

    /**
     * @brief Wait for a workflow execution event and then call the associated function to process that event
     */
    void WMS::waitForAndProcessNextEvent() {
        this->waitForAndProcessNextEvent(-1.0);
    }

    /**
     * @brief Wait for a workflow execution event and then call the associated function to process that event
     * @param timeout: a timeout value in seconds
     *
     * @return false if a timeout occurred (in which case no event was received/processed)
     * @throw wrench::ExecutionException
     */
    bool WMS::waitForAndProcessNextEvent(double timeout) {

        std::shared_ptr<ExecutionEvent> event = this->waitForNextEvent(timeout);
        if (event == nullptr) {
            return false;
        }

        if (auto real_event = std::dynamic_pointer_cast<StandardJobCompletedEvent>(event)) {
            processEventStandardJobCompletion(real_event);
        } else if (auto real_event = std::dynamic_pointer_cast<StandardJobFailedEvent>(event)) {
            processEventStandardJobFailure(real_event);
        } else if (auto real_event = std::dynamic_pointer_cast<PilotJobStartedEvent>(event)) {
            processEventPilotJobStart(real_event);
        } else if (auto real_event = std::dynamic_pointer_cast<PilotJobExpiredEvent>(event)) {
            processEventPilotJobExpiration(real_event);
        } else if (auto real_event = std::dynamic_pointer_cast<FileCopyCompletedEvent>(event)) {
            processEventFileCopyCompletion(real_event);
        } else if (auto real_event = std::dynamic_pointer_cast<FileCopyFailedEvent>(event)) {
            processEventFileCopyFailure(real_event);
        } else if (auto real_event = std::dynamic_pointer_cast<TimerEvent>(event)) {
            processEventTimer(real_event);
        } else {
            throw std::runtime_error("SimpleWMS::main(): Unknown workflow execution event: " + event->toString());
        }

        return true;
    }

    /**
     * @brief  Wait for a workflow execution event
     * @param timeout: a timeout value in seconds
     * @return the event
     */
    std::shared_ptr<ExecutionEvent> WMS::waitForNextEvent(double timeout) {
        return ExecutionEvent::waitForNextExecutionEvent(this->mailbox_name, timeout);
    }

    /**
     * @brief  Wait for a workflow execution event
     * @return the event
     */
    std::shared_ptr<ExecutionEvent> WMS::waitForNextEvent() {
        return ExecutionEvent::waitForNextExecutionEvent(this->mailbox_name, -1.0);
    }

    /**
     * @brief Process a standard job completion event
     *
     * @param event: a StandardJobCompletedEvent
     */
    void WMS::processEventStandardJobCompletion(std::shared_ptr<StandardJobCompletedEvent> event) {
        auto standard_job = event->standard_job;
        WRENCH_INFO("Notified that a %ld-task job has completed", standard_job->getNumTasks());
    }

    /**
     * @brief Process a standard job failure event
     *
     * @param event: a StandardJobFailedEvent
     */
    void WMS::processEventStandardJobFailure(std::shared_ptr<StandardJobFailedEvent> event) {
        WRENCH_INFO("Notified that a standard job has failed (all its tasks are back in the ready state)");
    }

    /**
     * @brief Process a pilot job start event
     *
     * @param event: a PilotJobStartedEvent
     */
    void WMS::processEventPilotJobStart(std::shared_ptr<PilotJobStartedEvent> event) {
        WRENCH_INFO("Notified that a pilot job has started!");
    }

    /**
     * @brief Process a pilot job expiration event
     *
     * @param event: a PilotJobExpiredEvent
     */
    void WMS::processEventPilotJobExpiration(std::shared_ptr<PilotJobExpiredEvent> event) {
        WRENCH_INFO("Notified that a pilot job has expired!");
    }

    /**
     * @brief Process a file copy completion event
     *
     * @param event: a FileCopyCompletedEvent
     */
    void WMS::processEventFileCopyCompletion(std::shared_ptr<FileCopyCompletedEvent> event) {
        WRENCH_INFO("Notified that a file copy is completed!");
    }

    /**
     * @brief Process a file copy failure event
     *
     * @param event: a FileCopyFailedEvent
     */
    void WMS::processEventFileCopyFailure(std::shared_ptr<FileCopyFailedEvent> event) {
        WRENCH_INFO("Notified that a file copy has failed!");
    }

    /**
     * @brief Process a timer event
     *
     * @param event: a TimerEvent
     */
    void WMS::processEventTimer(std::shared_ptr<TimerEvent> event) {
        WRENCH_INFO("Notified that a time has gone off!");
    }

    /**
     * @brief Assign a workflow to the WMS
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

            /**
             * Set the simulation pointer member variable of the workflow so that the
             * workflow tasks have access to it for creating SimulationTimestamps
             */
            this->workflow->simulation = this->simulation;
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
     * @brief Instantiate and start a job manager
     * @return a job manager
     */
    std::shared_ptr<JobManager> WMS::createJobManager() {
        std::shared_ptr<JobManager> job_manager = std::shared_ptr<JobManager>(
                new JobManager(this->hostname, this->mailbox_name));
        job_manager->simulation = this->simulation;
        job_manager->start(job_manager, true, false); // Always daemonize, no auto-restart

        // Let my schedulers know who the job manager is
        if (this->standard_job_scheduler) {
            this->standard_job_scheduler->setJobManager(job_manager->getSharedPtr<JobManager>());
        }
        if (this->pilot_job_scheduler) {
            this->pilot_job_scheduler->setJobManager(job_manager->getSharedPtr<JobManager>());
        }

        return job_manager;
    }

    /**
     * @brief Instantiate and start a data movement manager
     * @return a data movement manager
     */
    std::shared_ptr<DataMovementManager> WMS::createDataMovementManager() {
        auto data_movement_manager = std::shared_ptr<DataMovementManager>(
                new DataMovementManager(this->hostname, this->mailbox_name));
        data_movement_manager->simulation = this->simulation;
        data_movement_manager->start(data_movement_manager, true, false); // Always daemonize, no auto-restart

        // Let my schedulers know who the data movement manager is
        if (this->standard_job_scheduler) {
            this->standard_job_scheduler->setDataMovementManager(
                    data_movement_manager->getSharedPtr<DataMovementManager>());
        }
        if (this->pilot_job_scheduler) {
            this->pilot_job_scheduler->setDataMovementManager(
                    data_movement_manager->getSharedPtr<DataMovementManager>());
        }

        return data_movement_manager;
    }

    /**
     * @brief Instantiate and start an energy meter
     *
     * @param measurement_periods: the measurement period for each metered host
     *
     * @return an energy meter
     */
    std::shared_ptr<EnergyMeterService> WMS::createEnergyMeter(const std::map<std::string, double> &measurement_periods) {
        auto energy_meter_raw_ptr = new EnergyMeterService(this->hostname, measurement_periods);
        std::shared_ptr<EnergyMeterService> energy_meter = std::shared_ptr<EnergyMeterService>(energy_meter_raw_ptr);
        energy_meter->setSimulation(this->simulation);
        energy_meter->start(energy_meter, true, false); // Always daemonize, no auto-restart
        return energy_meter;
    }

    /**
     * @brief Instantiate and start an energy meter
     * @param hostnames: the list of metered hosts, as hostnames
     * @param measurement_period: the measurement period
     * @return an energy meter
     */
    std::shared_ptr<EnergyMeterService>
    WMS::createEnergyMeter(const std::vector<std::string> &hostnames, double measurement_period) {
        auto energy_meter_raw_ptr = new EnergyMeterService(this->hostname, hostnames, measurement_period);
        std::shared_ptr<EnergyMeterService> energy_meter = std::shared_ptr<EnergyMeterService>(energy_meter_raw_ptr);
        energy_meter->setSimulation(this->simulation);
        energy_meter->start(energy_meter, true, false); // Always daemonize, no auto-restart
        return energy_meter;
    }

    /**
     * @brief Instantiate and start a bandwidth meter
     *
     * @param measurement_periods: the measurement period for each metered link
     *
     * @return a link meter
     */
    std::shared_ptr<BandwidthMeterService> WMS::createBandwidthMeter(const std::map<std::string, double> &measurement_periods) {
        auto bandwidth_meter_raw_ptr = new BandwidthMeterService(this->hostname, measurement_periods);
        std::shared_ptr<BandwidthMeterService> bandwidth_meter = std::shared_ptr<BandwidthMeterService>(bandwidth_meter_raw_ptr);
        bandwidth_meter->setSimulation(this->simulation);
        bandwidth_meter->start(bandwidth_meter, true, false); // Always daemonize, no auto-restart
        return bandwidth_meter;
    }

    /**
     * @brief Instantiate and start a bandwidth meter
     * @param linknames: the list of metered links, as linknames
     * @param measurement_period: the measurement period
     * @return a link meter
     */
    std::shared_ptr<BandwidthMeterService>
    WMS::createBandwidthMeter(const std::vector<std::string> &linknames, double measurement_period) {
        auto bandwidth_meter_raw_ptr = new BandwidthMeterService(this->hostname, linknames, measurement_period);
        std::shared_ptr<BandwidthMeterService> bandwidth_meter = std::shared_ptr<BandwidthMeterService>(bandwidth_meter_raw_ptr);
        bandwidth_meter->setSimulation(this->simulation);
        bandwidth_meter->start(bandwidth_meter, true, false); // Always daemonize, no auto-restart
        return bandwidth_meter;
    }


    /**
     * @brief Get the WMS's pilot scheduler
     * 
     * @return the pilot scheduler, or nullptr if none
     */
    PilotJobScheduler *WMS::getPilotJobScheduler() {
        return (this->pilot_job_scheduler).get();
    }

    /**
     * @brief Get the WMS's pilot scheduler
     * 
     * @return the pilot scheduler, or nullptr if none
     */
    StandardJobScheduler *WMS::getStandardJobScheduler() {
        return (this->standard_job_scheduler).get();
    }

    /**
     * @brief Sets a timer (which, when it goes off, will generate a TimerEvent)
     * @param date: the date at which the timer should go off
     * @param message: a string message that will be in the generated TimerEvent
     */
    void WMS::setTimer(double date, std::string message) {
        Alarm::createAndStartAlarm(this->simulation, date, this->hostname, this->getWorkflow()->callback_mailbox,
                                   new AlarmWMSTimerMessage(message, 0), "wms_timer");
    }

};
