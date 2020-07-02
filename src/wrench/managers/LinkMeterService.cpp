/**
 * Copyright (c) 2017-2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/LinkMeterService.h"
#include <wrench/wms/WMS.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench-dev.h>

WRENCH_LOG_CATEGORY(wrench_core_link_meter, "Log category for Link Bandwidth Meter");

namespace wrench {
    /**
     * @brief Constructor
     *
     * @param hostname: the hostname on which the service should start
     * @param measurement_periods: the measurement period for each monitored link
     */
    LinkMeterService::LinkMeterService(const std::string hostname, const std::map<std::string, double> &measurement_periods) :
            Service(hostname, "link_meter", "link_meter") {
 
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

     
}