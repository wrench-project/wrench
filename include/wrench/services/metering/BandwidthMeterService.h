/**
 * Copyright (c) 2020. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_BANDWIDTHMETERSERVICE_H
#define WRENCH_BANDWIDTHMETERSERVICE_H

#include "wrench/services/Service.h"

namespace wrench {

    class WMS;

    /**
     * @brief A service that measures and records bandwidth usage on a set of links at regular time intervals
     */
    class BandwidthMeterService : public Service {
    public:
        BandwidthMeterService(std::string hostname, const std::vector<std::string> &linknames, double period);
        BandwidthMeterService(std::string hostname, const std::map<std::string, double> &measurement_periods);

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        void stop() override;
        void kill();
        /***********************/
        /** \endcond           */
        /***********************/


    private:
        int main() override;
        bool processNextMessage(double timeout);

        std::map<std::string, double> measurement_periods;
        std::map<std::string, double> time_to_next_measurement;
    };
}// namespace wrench

#endif//WRENCH_BANDWIDTHMETERSERVICE_H
