/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simulation/Simulation.h>
#include <wrench/action/Action.h>
#include <wrench/action/SleepAction.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>


#include <utility>

namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param sleep_time: the time to sleep, in seconds
    */
    SleepAction::SleepAction(const std::string &name, double sleep_time) : Action(name, "sleep_"), sleep_time(sleep_time) {
        if (sleep_time < 0) {
            throw std::invalid_argument("SleepAction::SleepAction(): Invalid (negative) sleep time");
        }
    }

    /**
     * @brief Returns the action's sleep time
     * @return the sleep time (in sec)
     */
    double SleepAction::getSleepTime() const {
        return this->sleep_time;
    }

    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void SleepAction::execute(const std::shared_ptr<ActionExecutor> &action_executor) {
        // Thread creation overhead
        Simulation::sleep(action_executor->getThreadCreationOverhead());
        // Sleeping
        S4U_Simulation::sleep(this->sleep_time);
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void SleepAction::terminate(const std::shared_ptr<ActionExecutor> &action_executor) {
        // Nothing to do for a Sleep Action
    }


}// namespace wrench
