/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_ENERGYMETER_H
#define WRENCH_ENERGYMETER_H

#include "wrench/services/Service.h"

namespace wrench {

    class WMS;

    /**
     * @brief A service that measures and records energy consumption on a set of hosts at regular time intervals
     */
    class EnergyMeter : public Service {

    public:

        void stop() override;
        void kill();

    protected:

        friend class WMS;

        EnergyMeter(std::shared_ptr<WMS> wms, const std::map<std::string, double> &measurement_periods);
        EnergyMeter(std::shared_ptr<WMS> wms, const std::vector<std::string> &hostnames, double period);


    private:
        int main() override;
        bool processNextMessage(double timeout);


            // Relevant WMS
        std::shared_ptr<WMS> wms;

        std::map<std::string, double> measurement_periods;
        std::map<std::string, double> time_to_next_measurement;

    };

};


#endif //WRENCH_ENERGYMETER_H
