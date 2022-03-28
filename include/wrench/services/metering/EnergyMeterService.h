/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_ENERGYMETERSERVICE_H
#define WRENCH_ENERGYMETERSERVICE_H

#include "wrench/services/Service.h"

namespace wrench {

    class WMS;

    /**
     * @brief A service that measures and records energy consumption on a set of hosts at regular time intervals
     */
    class EnergyMeterService : public Service {

    public:
        EnergyMeterService(std::string hostname, const std::vector<std::string> &hostnames, double period);
        EnergyMeterService(std::string hostname, const std::map<std::string, double> &measurement_periods);

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

};// namespace wrench


#endif//WRENCH_ENERGYMETERSERVICE_H
