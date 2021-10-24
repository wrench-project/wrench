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

#include "wrench/services/Service.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class SleeperVictim : public Service {

    public:

        explicit SleeperVictim(std::string host_on_which_to_run, double seconds_of_life, SimulationMessage *msg, std::string mailbox_to_notify);

        void cleanup(bool has_terminated_cleanly, int return_value) override;

    private:
        double seconds_of_life;
        SimulationMessage *msg;
        std::string mailbox_to_notify;
        int main() override;

    };


};


#endif //WRENCH_SLEEPER_H
