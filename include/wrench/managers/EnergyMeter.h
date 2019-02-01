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

    class EnergyMeter : public Service {

    public:

        void stop();
        void kill();

    protected:

        friend class WMS;

        EnergyMeter(WMS *wms, const std::map<std::string, double> &measurement_periods);
        EnergyMeter(WMS *wms, const std::vector<std::string> &hostnames, double period);


    private:
        int main();
        bool processNextMessage(double timeout);


            // Relevant WMS
        WMS *wms;

        std::map<std::string, double> measurement_periods;
        std::map<std::string, double> time_to_next_measurement;

    };

};


#endif //WRENCH_ENERGYMETER_H
