/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/execution_controller/ExecutionController.h>
#include <wrench/execution_controller/ExecutionControllerMessage.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/managers/DataMovementManager.h>
#include <wrench/managers/JobManager.h>
#include <wrench/services/helper_services/alarm/Alarm.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/managers/JobManager.h>
#include <wrench/failure_causes/NetworkError.h>

WRENCH_LOG_CATEGORY(wrench_core_execution_controller, "Log category for Execution Controller");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which to run the controller
     * @param suffix: a string to append to the controller process name (useful for debug output)
     *
     * @throw std::invalid_argument
     */
    ExecutionController::ExecutionController(
             const std::string &hostname,
             const std::string suffix) :
            Service(hostname, "controller_" + suffix, "controller_" + suffix) {
    }

    /**
     * @brief  Wait for an execution event
     * @param timeout: a timeout value in seconds
     * @return the event
     */
    std::shared_ptr<ExecutionEvent> ExecutionController::waitForNextEvent(double timeout) {
        return ExecutionEvent::waitForNextExecutionEvent(this->mailbox_name, timeout);
    }

    /**
     * @brief  Wait for an execution event
     * @return the event
     */
    std::shared_ptr<ExecutionEvent> ExecutionController::waitForNextEvent() {
        return ExecutionEvent::waitForNextExecutionEvent(this->mailbox_name, -1.0);
    }

    /**
     * @brief Instantiate and start a job manager
     * @return a job manager
     */
    std::shared_ptr<JobManager> ExecutionController::createJobManager() {
        std::shared_ptr<JobManager> job_manager = std::shared_ptr<JobManager>(
                new JobManager(this->hostname, this->mailbox_name));
        job_manager->simulation = this->simulation;
        job_manager->start(job_manager, true, false); // Always daemonize, no auto-restart

        return job_manager;
    }

    /**
     * @brief Instantiate and start a data movement manager
     * @return a data movement manager
     */
    std::shared_ptr<DataMovementManager> ExecutionController::createDataMovementManager() {
        auto data_movement_manager = std::shared_ptr<DataMovementManager>(
                new DataMovementManager(this->hostname, this->mailbox_name));
        data_movement_manager->simulation = this->simulation;
        data_movement_manager->start(data_movement_manager, true, false); // Always daemonize, no auto-restart

        return data_movement_manager;
    }

    /**
     * @brief Instantiate and start an energy meter
     *
     * @param measurement_periods: the measurement period for each metered host
     *
     * @return an energy meter
     */
    std::shared_ptr<EnergyMeterService> ExecutionController::createEnergyMeter(const std::map<std::string, double> &measurement_periods) {
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
    ExecutionController::createEnergyMeter(const std::vector<std::string> &hostnames, double measurement_period) {
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
    std::shared_ptr<BandwidthMeterService> ExecutionController::createBandwidthMeter(const std::map<std::string, double> &measurement_periods) {
        auto bandwidth_meter_raw_ptr = new BandwidthMeterService(this->hostname, measurement_periods);
        std::shared_ptr<BandwidthMeterService> bandwidth_meter = std::shared_ptr<BandwidthMeterService>(bandwidth_meter_raw_ptr);
        bandwidth_meter->setSimulation(this->simulation);
        bandwidth_meter->start(bandwidth_meter, true, false); // Always daemonize, no auto-restart
        return bandwidth_meter;
    }

    /**
     * @brief Instantiate and start a bandwidth meter
     * @param linknames: the list of metered links
     * @param measurement_period: the measurement period
     * @return a link meter
     */
    std::shared_ptr<BandwidthMeterService>
    ExecutionController::createBandwidthMeter(const std::vector<std::string> &linknames, double measurement_period) {
        auto bandwidth_meter_raw_ptr = new BandwidthMeterService(this->hostname, linknames, measurement_period);
        std::shared_ptr<BandwidthMeterService> bandwidth_meter = std::shared_ptr<BandwidthMeterService>(bandwidth_meter_raw_ptr);
        bandwidth_meter->setSimulation(this->simulation);
        bandwidth_meter->start(bandwidth_meter, true, false); // Always daemonize, no auto-restart
        return bandwidth_meter;
    }




    /**
     * @brief Sets a timer (which, when it goes off, will generate a TimerEvent)
     * @param date: the date at which the timer should go off
     * @param message: a string message that will be in the generated TimerEvent
     */
    void ExecutionController::setTimer(double date, std::string message) {
        Alarm::createAndStartAlarm(this->simulation, date, this->hostname, this->mailbox_name,
                                   new ExecutionControllerAlarmTimerMessage(message, 0), "wms_timer");
    }

};
