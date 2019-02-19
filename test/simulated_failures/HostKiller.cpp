/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "HostKiller.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(host_killer, "Log category for HostKiller");


wrench::HostKiller::HostKiller(std::string host_on_which_to_run, double sleep_time, std::string host_to_kill) :
        Service(host_on_which_to_run, "host_killer", "host_killer"){
    this->sleep_time = sleep_time;
    this->host_to_kill = host_to_kill;
}

int wrench::HostKiller::main() {

    WRENCH_INFO("Starting");
    wrench::Simulation::sleep(sleep_time);
    WRENCH_INFO("Turning off Host %s", this->host_to_kill.c_str());
    simgrid::s4u::Host::by_name(host_to_kill)->turn_off();

    return 0;
}
