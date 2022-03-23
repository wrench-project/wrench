/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_RESOURCERANDOMREPEATSWITCHER_H
#define WRENCH_RESOURCERANDOMREPEATSWITCHER_H

#include "wrench/services/Service.h"
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class ResourceRandomRepeatSwitcher : public Service {

    public:
        enum ResourceType {
            HOST,
            LINK
        };


        explicit ResourceRandomRepeatSwitcher(std::string host_on_which_to_run, double seed,
                                              double min_sleep_before_off_time, double max_sleep_before_off_time,
                                              double min_sleep_before_on_time, double max_sleep_before_on_time,
                                              std::string resource_to_switch,
                                              ResourceType resource_type);

        void kill();

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        double seed;
        double min_sleep_before_off_time;
        double max_sleep_before_off_time;
        double min_sleep_before_on_time;
        double max_sleep_before_on_time;
        std::string resource_to_switch;
        ResourceType resource_type;
        int main();
    };

    /***********************/
    /** \endcond            */
    /***********************/


};// namespace wrench


#endif//WRENCH_RESOURCERANDOMREPEATSWITCHER_H
