/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_RESOURCESWITCHER_H
#define WRENCH_RESOURCESWITCHER_H

#include "wrench/services/Service.h"
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class ResourceSwitcher : public Service {

    public:
        enum ResourceType {
            HOST,
            LINK
        };

        enum Action {
            TURN_ON,
            TURN_OFF
        };

        explicit ResourceSwitcher(std::string host_on_which_to_run, double sleep_time, std::string host_to_switch, Action action, ResourceType resource_type);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        double sleep_time;
        std::string resource_to_switch;
        Action action;
        ResourceType resource_type;
        int main();

    };

    /***********************/
    /** \endcond            */
    /***********************/


};






#endif //WRENCH_RESOURCESWITCHER_H
