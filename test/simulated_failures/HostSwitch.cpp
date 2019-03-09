/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "HostSwitch.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(host_killer, "Log category for HostSwitch");


wrench::HostSwitch::HostSwitch(std::string host_on_which_to_run, double sleep_time, std::string host_to_switch, Action action) :
        Service(host_on_which_to_run, "host_switcher", "host_switcher"){
    this->sleep_time = sleep_time;
    this->host_to_switch = host_to_switch;
    this->action = action;
}

int wrench::HostSwitch::main() {

    WRENCH_INFO("Starting and sleeping for %.3lf seconds...", sleep_time);
    wrench::Simulation::sleep(sleep_time);
    if (this->action == HostSwitch::Action::TURN_OFF) {
        WRENCH_INFO("Turning OFF host %s", this->host_to_switch.c_str());
        simgrid::s4u::Host::by_name(host_to_switch)->turn_off();
    } else {
        WRENCH_INFO("Turning ON host %s", this->host_to_switch.c_str());
        simgrid::s4u::Host::by_name(host_to_switch)->turn_on();
    }

    return 0;
}
