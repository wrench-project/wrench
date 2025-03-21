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
#include <wrench/action/CustomAction.h>

#include <utility>

namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param ram: memory required
    * @param num_cores: number of cores required
    * @param lambda_execute: a lambda that implements the action's execution
    * @param lambda_terminate: a lambda that implements the action's termination (typically a no-op)
    */
    CustomAction::CustomAction(const std::string &name,
                               sg_size_t ram,
                               unsigned long num_cores,
                               std::function<void(std::shared_ptr<ActionExecutor>)> lambda_execute,
                               std::function<void(std::shared_ptr<ActionExecutor>)> lambda_terminate) : Action(name, "custom_"), ram(ram), num_cores(num_cores), lambda_execute(std::move(lambda_execute)), lambda_terminate(std::move(lambda_terminate)) {
    }

    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void CustomAction::execute(const std::shared_ptr<ActionExecutor> &action_executor) {
        this->lambda_execute(action_executor);
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void CustomAction::terminate(const std::shared_ptr<ActionExecutor> &action_executor) {
        this->lambda_terminate(action_executor);
    }

    /**
     * @brief Returns the action's minimum number of required cores
     * @return a number of cores
     */
    unsigned long CustomAction::getMinNumCores() const {
        return this->num_cores;
    }

    /**
     * @brief Returns the action's maximum number of required cores
     * @return a number of cores
     */
    unsigned long CustomAction::getMaxNumCores() const {
        return this->num_cores;
    }

    /**
     * @brief Returns the action's minimum required memory footprint
     * @return a number of bytes
     */
    sg_size_t CustomAction::getMinRAMFootprint() const {
        return this->ram;
    }

}// namespace wrench
