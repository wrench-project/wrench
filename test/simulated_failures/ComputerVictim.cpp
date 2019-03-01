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

XBT_LOG_NEW_DEFAULT_CATEGORY(computer_victim, "Log category for Computer");


wrench::ComputerVictim::ComputerVictim(std::string host_on_which_to_run, double flops, SimulationMessage *msg, std::string mailbox_to_notify)
        : Service(host_on_which_to_run, "victim", "victim") {
    this->flops = flops;
    this->msg = msg;
    this->mailbox_to_notify = mailbox_to_notify;
}


int wrench::ComputerVictim::main() {

    WRENCH_INFO("Starting  (%u)", this->num_starts);

    try {
        WRENCH_INFO("Computing %.3lf flops...", this->flops);
        wrench::Simulation::compute(this->flops);
    } catch (std::shared_ptr<wrench::HostError> &e) {
        /** ON MAC I GET IN THIS!! ON LINUX I DON'T!!! SINCE THE INTEND BEHAVIOR IN
         * SIMGRID WILL LIKELY BE TO NEVER BE ABLE TO CATCH THIS EXCEPTION, LET JUST
         * DIRECTLY TO TO THE ON_EXIT()
         **/
       simgrid::s4u::this_actor::exit();
    }
    S4U_Mailbox::dputMessage(this->mailbox_to_notify, this->msg);

    return 0;

}
