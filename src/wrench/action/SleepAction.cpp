/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/simulation/Simulation.h"
#include "wrench/action/Action.h"
#include "wrench/action/SleepAction.h"

namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param job: the job this action belongs to
    * @param sleep: the time to sleep, in seconds
    */
    SleepAction::SleepAction(std::string name, std::shared_ptr<CompoundJob> job, double sleep_time) : Action(name, job), sleep_time(sleep_time) {
    }

    /**
     * Returns the action's sleep time
     * @return the sleep time (in sec)
     */
    double SleepAction::getSleepTime() {
        return this->sleep_time;
    }

}
