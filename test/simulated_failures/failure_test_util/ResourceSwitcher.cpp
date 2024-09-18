/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ResourceSwitcher.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

WRENCH_LOG_CATEGORY(host_switcher, "Log category for HostSwitcher");


wrench::ResourceSwitcher::ResourceSwitcher(std::string host_on_which_to_run, double sleep_time, std::string resource_to_switch,
                                           Action action, ResourceType resource_type) : Service(host_on_which_to_run, "host_switcher") {
    this->sleep_time = sleep_time;
    this->resource_to_switch = resource_to_switch;
    this->action = action;
    this->resource_type = resource_type;
}

int wrench::ResourceSwitcher::main() {

    WRENCH_INFO("Starting and sleeping for %.3lf seconds...", sleep_time);
    wrench::Simulation::sleep(sleep_time);
    if (this->action == ResourceSwitcher::Action::TURN_OFF) {
        if (this->resource_type == ResourceType::HOST) {
            WRENCH_INFO("Turning OFF host %s", this->resource_to_switch.c_str());
            wrench::Simulation::turnOffHost(resource_to_switch);
        } else if (this->resource_type == ResourceType::LINK) {
            WRENCH_INFO("Turning OFF link %s", this->resource_to_switch.c_str());
            wrench::Simulation::turnOffLink(resource_to_switch);
        }
    } else {
        if (this->resource_type == ResourceType::HOST) {
            WRENCH_INFO("Turning ON host %s", this->resource_to_switch.c_str());
            wrench::Simulation::turnOnHost(resource_to_switch);
        } else if (this->resource_type == ResourceType::LINK) {
            WRENCH_INFO("Turning OFF link %s", this->resource_to_switch.c_str());
            wrench::Simulation::turnOnLink(resource_to_switch);
        }
    }

    return 0;
}
