/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HOSTRANDOMREPEATSWITCH_H
#define WRENCH_HOSTRANDOMREPEATSWITCH_H

#include <wrench/services/Service.h>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

class HostRandomRepeatSwitch : public Service {

    public:


        explicit HostRandomRepeatSwitch(std::string host_on_which_to_run, double seed, double min_sleep_time, double max_sleep_time, std::string host_to_switch);

        void kill();

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        double seed;
        double min_sleep_time;
        double max_sleep_time;
        std::string host_to_switch;
        int main();

    };

    /***********************/
    /** \endcond            */
    /***********************/


};


#endif //WRENCH_HOSTRANDOMREPEATSWITCH_H
