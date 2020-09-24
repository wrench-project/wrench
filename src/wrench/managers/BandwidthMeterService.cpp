/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/BandwidthMeterService.h"
#include <wrench/wms/WMS.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench-dev.h>

#define EPSILON 0.0001

WRENCH_LOG_CATEGORY(wrench_core_bandwidth_meter, "Log category for Link Bandwidth Meter");

namespace wrench {
    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the service should start
     * @param measurement_periods: the measurement period for each monitored link
     */
    BandwidthMeterService::BandwidthMeterService(const std::string hostname, const std::map<std::string, double> &measurement_periods) :
            Service(hostname, "bandwidth_meter", "bandwidth_meter") {
        if (measurement_periods.empty()) {
            throw std::invalid_argument("BandwidthMeter::BandwidthMeter(): no host to meter!");
        }

        for (auto &h : measurement_periods) {
            if (not S4U_Simulation::hostExists(h.first)) {
                throw std::invalid_argument("BandwidthMeter::BandwidthMeter(): unknown host " + h.first);
            }
            if (h.second < 0.01) {
                throw std::invalid_argument(
                        "BandwidthMeter::BandwidthMeter(): measurement period must be at least 0.01 second (host " + h.first +
                        ")");
            }
            this->measurement_periods[h.first] = h.second;
            this->time_to_next_measurement[h.first] = 0.0;  // We begin by taking a measurement
        }
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which this service is running
     * @param linknames: the list of metered links, as link ids
     * @param measurement_period: the measurement period
     */
    BandwidthMeterService::BandwidthMeterService(const std::string hostname, const std::vector<std::string> &linknames,
                                           double measurement_period) :
            Service(hostname, "bandwidth_meter", "bandwidth_meter") {
        if (linknames.empty()) {
            throw std::invalid_argument("BandwidthMeter::BandwidthMeter(): no host to meter!");
        }
        if (measurement_period < 0.01) {
            throw std::invalid_argument("BandwidthMeter::BandwidthMeter(): measurement period must be at least 0.01 second");
        }

        for (auto const &l : linknames) {
            if (not S4U_Simulation::linkExists(l)) {
                throw std::invalid_argument("BandwidthMeter::BandwidthMeter(): unknown link " + l);
            }
            this->measurement_periods[l] = measurement_period;
            this->time_to_next_measurement[l] = 0.0;  // We begin by taking a measurement
        }
    }

    /**
     * @brief Kill the bandwidth meter (brutally terminate the daemon)
     */
    void BandwidthMeterService::kill() {
        this->killActor();
    }

    /**
     * @brief Stop the bandwidth meter
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void BandwidthMeterService::stop() {
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }
    }

    /**
     * @brief Main method of the daemon that implements the BandwidthMeter
     * @return 0 on success
     */
    int BandwidthMeterService::main() {
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Bandwidth Meter Manager starting (%s) and monitoring links", this->mailbox_name.c_str());
        for (auto const &l : this->measurement_periods) {
            WRENCH_INFO("  - %s", l.first.c_str());
        }

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

            // Update time-to-next-measurement for all links
            for (auto &l : this->time_to_next_measurement) {
                l.second = std::max<double>(0, l.second - (Simulation::getCurrentSimulatedDate() - before));
            }

            // Take measurements
            for (auto &l : this->time_to_next_measurement) {
                if (l.second < EPSILON) {
                    this->simulation->getLinkUsage(l.first, true);
                    this->time_to_next_measurement[l.first] = this->measurement_periods[l.first];
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
    bool BandwidthMeterService::processNextMessage(double timeout) {
        std::shared_ptr<SimulationMessage> message = nullptr;

        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name, timeout);
        } catch (std::shared_ptr<NetworkError> &cause) {
            return true;
        }

        WRENCH_INFO("Bandwidth Meter got a %s message", message->getName().c_str());

        if (std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // There shouldn't be any need to clean any state up
            return false;
        }

        throw std::runtime_error("BandwidthMeter::waitForNextMessage(): Unexpected [" + message->getName() + "] message");
    }

     
};