/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SleeperVictim.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

WRENCH_LOG_CATEGORY(sleeper_victom, "Log category for Sleeper");


wrench::SleeperVictim::SleeperVictim(std::string host_on_which_to_run, double seconds_of_life, SimulationMessage *msg, S4U_CommPort *commport_to_notify)
    : Service(host_on_which_to_run, "victim") {
    this->seconds_of_life = seconds_of_life;
    this->msg = msg;
    this->commport_to_notify = commport_to_notify;
}


int wrench::SleeperVictim::main() {

    WRENCH_INFO("Starting  (%u)", this->num_starts);
    WRENCH_INFO("Sleeping for %.3lf seconds...", this->seconds_of_life);
    wrench::Simulation::sleep(this->seconds_of_life);
    this->commport_to_notify->putMessage(this->msg);
    return 0;
}

void wrench::SleeperVictim::cleanup(bool has_terminated_cleanly, int return_value) {
    // Do nothing (ignore failures)
}
