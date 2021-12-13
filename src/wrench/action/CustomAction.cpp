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
    * @param job: the job this action belongs to
    * @param lambda_execute: a lambda that implements the action's execution
    * @param lambda_terminate: a lambda that implements the action's termination (typically a no-op)
    */
    CustomAction::CustomAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                               const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_execute,
                               const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_terminate) :
            Action(name, "custom_", std::move(job)), lambda_execute(lambda_execute), lambda_terminate(lambda_terminate) {
    }

    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void CustomAction::execute(std::shared_ptr<ActionExecutor> action_executor) {
        this->lambda_execute(action_executor);
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void CustomAction::terminate(std::shared_ptr<ActionExecutor> action_executor) {
        this->lambda_terminate(action_executor);
    }

}
