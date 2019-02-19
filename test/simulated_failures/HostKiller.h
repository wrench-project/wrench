/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HOSTKILLER_H
#define WRENCH_HOSTKILLER_H

#include <wrench/services/Service.h>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

class HostKiller : public Service {

    public:

//        void stop();
//        void kill();
        explicit HostKiller(std::string host_on_which_to_run, double sleep_time, std::string host_to_kill);

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        double sleep_time;
        std::string host_to_kill;
        int main();

    };

    /***********************/
    /** \endcond            */
    /***********************/


};






#endif //WRENCH_HOSTKILLER_H
