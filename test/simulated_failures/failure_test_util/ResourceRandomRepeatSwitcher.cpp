/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ResourceRandomRepeatSwitcher.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>

WRENCH_LOG_CATEGORY(resource_random_repeat_switcher, "Log category for ResourceRandomRepeatSwitcher");


wrench::ResourceRandomRepeatSwitcher::ResourceRandomRepeatSwitcher(std::string host_on_which_to_run, double seed,
                                                                   double min_sleep_before_off_time, double max_sleep_before_off_time,
                                                                   double min_sleep_before_on_time, double max_sleep_before_on_time,
                                                                   std::string resource_to_switch,
                                                                   ResourceType resource_type) :

                                                                                                 Service(host_on_which_to_run, "resource_switcher") {
    this->seed = seed;
    this->min_sleep_before_off_time = min_sleep_before_off_time;
    this->max_sleep_before_off_time = max_sleep_before_off_time;
    this->min_sleep_before_on_time = min_sleep_before_on_time;
    this->max_sleep_before_on_time = max_sleep_before_on_time;
    this->resource_to_switch = resource_to_switch;
    this->resource_type = resource_type;
}

void wrench::ResourceRandomRepeatSwitcher::kill() {

    this->killActor();
}


int wrench::ResourceRandomRepeatSwitcher::main() {

    //Type of random number distribution
    std::uniform_real_distribution<double> dist_off(this->min_sleep_before_off_time, this->max_sleep_before_off_time);
    std::uniform_real_distribution<double> dist_on(this->min_sleep_before_on_time, this->max_sleep_before_on_time);
    //Mersenne Twister: Good quality random number generator
    std::mt19937 rng;

    rng.seed(this->seed);

    while (true) {
        double sleep_before_off_time = dist_off(rng);
        WRENCH_INFO("Sleeping for %.3lf seconds...", sleep_before_off_time);
        wrench::Simulation::sleep(sleep_before_off_time);
        switch (this->resource_type) {
            case ResourceType::HOST: {
                WRENCH_INFO("Turning OFF host %s", this->resource_to_switch.c_str());
                wrench::Simulation::turnOffHost(this->resource_to_switch);
                break;
            }
            case ResourceType::LINK: {
                WRENCH_INFO("Turning OFF link %s", this->resource_to_switch.c_str());
                wrench::Simulation::turnOffLink(this->resource_to_switch);
                break;
            }
        }
        double sleep_before_on_time = dist_on(rng);
        WRENCH_INFO("Sleeping for %.3lf seconds...", sleep_before_on_time);
        wrench::Simulation::sleep(sleep_before_on_time);
        switch (this->resource_type) {
            case ResourceType::HOST: {
                WRENCH_INFO("Turning ON host %s", this->resource_to_switch.c_str());
                wrench::Simulation::turnOnHost(this->resource_to_switch);
                break;
            }
            case ResourceType::LINK: {
                WRENCH_INFO("Turning ON link %s", this->resource_to_switch.c_str());
                wrench::Simulation::turnOnLink(this->resource_to_switch);
                break;
            }
        }
    }
    return 0;
}
