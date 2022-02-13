/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ComputerVictim.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

WRENCH_LOG_CATEGORY(computer_victim, "Log category for Computer");


wrench::ComputerVictim::ComputerVictim(std::string host_on_which_to_run, double flops, SimulationMessage *msg, simgrid::s4u::Mailbox *mailbox_to_notify)
        : Service(host_on_which_to_run, "victim") {
    this->flops = flops;
    this->msg = msg;
    this->mailbox_to_notify = mailbox_to_notify;
}


int wrench::ComputerVictim::main() {

    WRENCH_INFO("Starting  (%u)", this->num_starts);
    WRENCH_INFO("Computing %.3lf flops...", this->flops);
    wrench::Simulation::compute(this->flops);
    S4U_Mailbox::putMessage(this->mailbox_to_notify, this->msg);
    return 0;

}

void wrench::ComputerVictim::cleanup(bool has_terminated_cleanly, int return_value) {
    // Do nothing (ignore failures)
}
