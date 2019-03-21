/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "HostRandomRepeatSwitch.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(host_random_repeat_switch, "Log category for HostRandomRepeatSwitch");


wrench::HostRandomRepeatSwitch::HostRandomRepeatSwitch(std::string host_on_which_to_run, double seed, double min_sleep_time, double max_sleep_time, std::string host_to_switch) :
        Service(host_on_which_to_run, "host_switcher", "host_switcher"){
    this->seed = seed;
    this->min_sleep_time = min_sleep_time;
    this->max_sleep_time = max_sleep_time;
    this->host_to_switch = host_to_switch;
}

void wrench::HostRandomRepeatSwitch::kill() {

    this->killActor();
}


int wrench::HostRandomRepeatSwitch::main() {

    //Type of random number distribution
    std::uniform_real_distribution<double> dist(this->min_sleep_time, this->max_sleep_time);
    //Mersenne Twister: Good quality random number generator
    std::mt19937 rng;

    rng.seed(this->seed);

    double action = true;
    while (true) {
        double sleep_time = dist(rng);
        WRENCH_INFO("Sleeping for %.3lf seconds...", sleep_time);
        wrench::Simulation::sleep(sleep_time);
        if (action) {
            WRENCH_INFO("Turning OFF host %s", this->host_to_switch.c_str());
            simgrid::s4u::Host::by_name(host_to_switch)->turn_off();
        } else {
            WRENCH_INFO("Turning ON host %s", this->host_to_switch.c_str());
            simgrid::s4u::Host::by_name(host_to_switch)->turn_on();
        }
        action = not action;
    }

    return 0;
}
