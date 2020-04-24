/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/EnergyMeterService.h"
#include <wrench/wms/WMS.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench-dev.h>

#define EPSILON 0.0001

WRENCH_LOG_CATEGORY(wrench_core_energy_meter, "Log category for Energy Meter");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the service shoudl start
     * @param measurement_periods: the measurement period for each metered host
     */
    EnergyMeterService::EnergyMeterService(const std::string hostname, const std::map<std::string, double> &measurement_periods) :
            Service(hostname, "energy_meter", "energy_meter") {

        if (measurement_periods.empty()) {
            throw std::invalid_argument("EnergyMeter::EnergyMeter(): no host to meter!");
        }

        for (auto &h : measurement_periods) {
            if (not S4U_Simulation::hostExists(h.first)) {
                throw std::invalid_argument("EnergyMeter::EnergyMeter(): unknown host " + h.first);
            }
            if (h.second < 1.0) {
                throw std::invalid_argument(
                        "EnergyMeter::EnergyMeter(): measurement period must be at least 1 second (host " + h.first +
                        ")");
            }
            this->measurement_periods[h.first] = h.second;
            this->time_to_next_measurement[h.first] = 0.0;  // We begin by taking a measurement
        }
    }

    /**
     * @brief Constructor
     *
     * @parame hostname: the name of the host on which this service is running
     * @param hostnames: the list of metered hosts, as hostnames
     * @param measurement_period: the measurement period
     */
    EnergyMeterService::EnergyMeterService(const std::string hostname, const std::vector<std::string> &hostnames,
                                           double measurement_period) :
            Service(hostname, "energy_meter", "energy_meter") {

        if (hostnames.empty()) {
            throw std::invalid_argument("EnergyMeter::EnergyMeter(): no host to meter!");
        }
        if (measurement_period < 1) {
            throw std::invalid_argument("EnergyMeter::EnergyMeter(): measurement period must be at least 1 second");
        }

        for (auto const &h : hostnames) {
            if (not S4U_Simulation::hostExists(h)) {
                throw std::invalid_argument("EnergyMeter::EnergyMeter(): unknown host " + h);
            }
            this->measurement_periods[h] = measurement_period;
            this->time_to_next_measurement[h] = 0.0;  // We begin by taking a measurement
        }
    }

    /**
     * @brief Kill the energy meter (brutally terminate the daemon)
     */
    void EnergyMeterService::kill() {
        this->killActor();
    }

    /**
     * @brief Stop the energy meter
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void EnergyMeterService::stop() {
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }
    }

    /**
     * @brief Main method of the daemon that implements the EnergyMeter
     * @return 0 on success
     */
    int EnergyMeterService::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Energy Meter Manager starting (%s)", this->mailbox_name.c_str());

        /** Main loop **/
        while (true) {
            S4U_Simulation::computeZeroFlop();

            // Find the minimum of the times to next measurements
            auto min_el = std::min_element(
                    this->time_to_next_measurement.begin(),
                    this->time_to_next_measurement.end(),
                    [](decltype(this->time_to_next_measurement)::value_type &lhs,
                       decltype(this->time_to_next_measurement)::value_type &rhs) {
                        return lhs.second < rhs.second;
                    });

            double time_to_next_measure = min_el->second;
            double before = Simulation::getCurrentSimulatedDate();

            if (time_to_next_measure > 0) {
                if (not processNextMessage(time_to_next_measure)) {
                    break;
                }
            }

            // Update time-to-next-measurement for all hosts
            for (auto &h : this->time_to_next_measurement) {
                h.second = std::max<double>(0, h.second - (Simulation::getCurrentSimulatedDate() - before));
            }

            // Take measurements
            for (auto &h : this->time_to_next_measurement) {
                if (h.second < EPSILON) {
                    this->simulation->getEnergyConsumed(h.first, true);
                    this->time_to_next_measurement[h.first] = this->measurement_periods[h.first];
                }
            }
        }

        WRENCH_INFO("Energy Meter Manager terminating");

        return 0;
    }

    /**
     * @brief Process the next message
     * @return true if the daemon should continue, false otherwise
     *
     * @throw std::runtime_error
     */
    bool EnergyMeterService::processNextMessage(double timeout) {

        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name, timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        WRENCH_INFO("Energy Meter got a %s message", message->getName().c_str());

        if (std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // There shouldn't be any need to clean any state up
            return false;
        }

        throw std::runtime_error("EnergyMeter::waitForNextMessage(): Unexpected [" + message->getName() + "] message");

    }

};
