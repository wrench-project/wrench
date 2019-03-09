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

XBT_LOG_NEW_DEFAULT_CATEGORY(sleeper_victom, "Log category for Sleeper");


wrench::SleeperVictim::SleeperVictim(std::string host_on_which_to_run, double seconds_of_life, SimulationMessage *msg, std::string mailbox_to_notify)
        : Service(host_on_which_to_run, "victim", "victim") {
    this->seconds_of_life = seconds_of_life;
    this->msg = msg;
    this->mailbox_to_notify = mailbox_to_notify;
}


int wrench::SleeperVictim::main() {

    WRENCH_INFO("Starting  (%u)", this->num_starts);
    try {
        WRENCH_INFO("Sleeping for %.3lf seconds...", this->seconds_of_life);
        wrench::Simulation::sleep(this->seconds_of_life);
    } catch (std::shared_ptr<wrench::HostError> &e) {
        return 1;
    }
    S4U_Mailbox::dputMessage(this->mailbox_to_notify, this->msg);

    return 0;

}
