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
#include <wrench/workflow/Workflow.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/action/SleepAction.h>
#include <wrench/action/ComputeAction.h>
#include <wrench/action/FileReadAction.h>

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
     * @return a sleep action
     */
    std::shared_ptr<SleepAction> CompoundJob::addSleepAction(std::string name, double sleep_time) {
        auto new_action = std::shared_ptr<SleepAction>(new SleepAction(name, this->shared_this, sleep_time));
        this->actions.insert(new_action);
        return new_action;
    }

    /**
     * @brief Add a compute action to the job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param flops: the number of flops to perform
     * @param ram: the amount of RAM required
     * @param min_num_cores: the minimum number of cores needed
     * @param max_num_cores: the maximum number of cores allowed
     * @param parallel_model: the parallel speedup model
     * @return a compute action
     */
    std::shared_ptr<ComputeAction> CompoundJob::addComputeAction(std::string name,
                                                               double flops,
                                                               double ram,
                                                               int min_num_cores,
                                                               int max_num_cores,
                                                               std::shared_ptr<ParallelModel> parallel_model) {
        auto new_action = std::shared_ptr<ComputeAction>(
                new ComputeAction(name, this->shared_this, flops, ram, min_num_cores, max_num_cores, parallel_model));
        this->actions.insert(new_action);
        return new_action;

    }


    /**
     * @brief Add a file read action to the job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file: the file
     * @param file_location: the file's location
     * @return a file read action
     */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(std::string name,
                                                                std::shared_ptr<WorkflowFile> file,
                                                                std::shared_ptr<FileLocation> file_location) {
        auto new_action = std::shared_ptr<FileReadAction>(
                new FileReadAction(name, this->shared_this, std::move(file), {std::move(file_location)}));
        this->actions.insert(new_action);
        return new_action;
    }

    /**
    * @brief Add a file read action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file: the file
    * @param file_locations: the locations to read the file from (will be tried in order until one succeeds)
    * @return a file read action
    */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(std::string name,
                                                                   std::shared_ptr<WorkflowFile> file,
                                                                   std::vector<std::shared_ptr<FileLocation>> file_locations) {
        auto new_action = std::shared_ptr<FileReadAction>(
        new FileReadAction(name, this->shared_this, std::move(file), std::move(file_locations)));
        this->actions.insert(new_action);
        return new_action;
    }

}
