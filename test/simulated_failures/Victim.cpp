/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Victim.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(victim, "Log category for Victim");


wrench::Victim::Victim(std::string host_on_which_to_run, double seconds_of_life, SimulationMessage *msg, std::string mailbox_to_notify)
        : Service(host_on_which_to_run, "victim", "victim") {
    this->seconds_of_life = seconds_of_life;
    this->msg = msg;
    this->mailbox_to_notify = mailbox_to_notify;
}


int wrench::Victim::main() {

    WRENCH_INFO("Starting");
    wrench::Simulation::sleep(this->seconds_of_life);
    S4U_Mailbox::dputMessage(this->mailbox_to_notify, this->msg);

    return 0;

}
