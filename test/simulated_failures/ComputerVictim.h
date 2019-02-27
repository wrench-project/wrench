/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SLEEPER_H
#define WRENCH_SLEEPER_H

#include <wrench/services/Service.h>
#include <wrench/simulation/SimulationMessage.h>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class ComputerVictim
            : public Service {

    public:

        explicit ComputerVictim(std::string host_on_which_to_run, double flops, SimulationMessage *msg, std::string mailbox_to_notify);

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        double flops;
        SimulationMessage *msg;
        std::string mailbox_to_notify;
        int main() override;

    };

    /***********************/
    /** \endcond            */
    /***********************/


};


#endif //WRENCH_SLEEPER_H
