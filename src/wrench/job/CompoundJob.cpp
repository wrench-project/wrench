/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <set>
#include <utility>
#include <wrench-dev.h>
#include "wrench/workflow/Workflow.h"
#include "wrench/job/CompoundJob.h"
#include "wrench/action/SleepAction.h"


WRENCH_LOG_CATEGORY(wrench_core_compound_job, "Log category for CompoundJob");


namespace wrench {

    /**
     * @brief Constructor
     *
     * @param name: the job's name (if empty, a unique name will be chosen for you)
     * @param job_manager: the Job Manager in charge of this job
     *
     * @throw std::invalid_argument
     */
    CompoundJob::CompoundJob(std::string name, std::shared_ptr<JobManager> job_manager)
            :
            Job(std::move(name), job_manager),
            state(CompoundJob::State::NOT_SUBMITTED) {
    }

    /**
     * @brief Get the job's actions
     *
     * @return the set of actions in the job
     */
    std::set<std::shared_ptr<Action>> CompoundJob::getActions() {
        return this->actions;
    }

    /**
     * @brief Get the state of the standard job
     * @return the state
     */
    CompoundJob::State CompoundJob::getState() {
        return this->state;
    }

    /**
     * @brief Get the job's priority (which may be used by some services)
     * @return the priority
     */
     unsigned long CompoundJob::getPriority() {
         return this->priority;
     }

    /**
    * @brief Set the job's priority (which may be used by some services)
    * @param priority: the job's priority
    */
    void CompoundJob::setPriority(unsigned long priority) {
        this->priority = priority;
    }

    /**
     * @brief Add a sleep action to the job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param sleep_time: the time to sleep, in seconds
     * @return
     */
    std::shared_ptr<SleepAction> CompoundJob::addSleepAction(std::string name, double sleep_time) {
        if (name.empty()) name = "sleep_";
        auto new_action = std::shared_ptr<SleepAction>(new SleepAction(name, this->shared_this, sleep_time));
        this->actions.insert(new_action);
        return new_action;
    }

}
