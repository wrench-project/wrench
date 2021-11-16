/**
 * Copyright (c) 2017-2021. The WRENCH Team.
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
#include <wrench/action/FileWriteAction.h>
#include <wrench/action/FileCopyAction.h>
#include <wrench/action/FileDeleteAction.h>
#include <wrench/action/FileRegistryAddEntryAction.h>
#include <wrench/action/FileRegistryDeleteEntryAction.h>
#include <wrench/action/CustomAction.h>

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
     * @brief Get the state of the standard job
     * @return the state
     */
    std::string CompoundJob::getStateAsString() {
        switch(this->state) {
            case NOT_SUBMITTED:
                return "NOT SUBMITTED";
            case SUBMITTED:
                return "SUBMITTED";
            case COMPLETED:
                return "COMPLETED";
            case DISCONTINUED:
                return "DISCONTINUED";
            default:
                return "???";
        }
    }


    /**
    * @brief Set the job's priority (the higher the priority value, the higher the priority)
    * @param priority: a priority
    */
    void CompoundJob::setPriority(double priority) {
        assertJobNotSubmitted();
        this->priority = priority;
    }

    /**
     * @brief Add a sleep action to the job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param sleep_time: the time to sleep, in seconds
     * @return a sleep action
     */
    std::shared_ptr<SleepAction> CompoundJob::addSleepAction(std::string name, double sleep_time) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<SleepAction>(new SleepAction(name, this->shared_this, sleep_time));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
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
                                                                 unsigned long min_num_cores,
                                                                 unsigned long max_num_cores,
                                                                 std::shared_ptr<ParallelModel> parallel_model) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<ComputeAction>(
                new ComputeAction(name, this->shared_this, flops, ram, min_num_cores, max_num_cores, parallel_model));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
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
                                                                   WorkflowFile *file,
                                                                   std::shared_ptr<FileLocation> file_location) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<FileReadAction>(
                new FileReadAction(name, this->shared_this, file, {std::move(file_location)}));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
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
                                                                   WorkflowFile *file,
                                                                   std::vector<std::shared_ptr<FileLocation>> file_locations) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<FileReadAction>(
                new FileReadAction(name, this->shared_this, file, std::move(file_locations)));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
        this->actions.insert(new_action);
        return new_action;
    }

    /**
    * @brief Add a file write action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file: the file
    * @param file_location: the file's location where it should be written
    * @return a file write action
    */
    std::shared_ptr<FileWriteAction> CompoundJob::addFileWriteAction(std::string name,
                                                                     WorkflowFile *file,
                                                                     std::shared_ptr<FileLocation> file_location) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<FileWriteAction>(
                new FileWriteAction(name, this->shared_this, file, {std::move(file_location)}));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
        this->actions.insert(new_action);
        return new_action;
    }

    /**
    * @brief Add a file copy action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file: the file
    * @param src_file_location: the file's location where it should be read
    * @param dst_file_location: the file's location where it should be written
    * @return a file copy action
    */
    std::shared_ptr<FileCopyAction> CompoundJob::addFileCopyAction(std::string name,
                                                                   WorkflowFile *file,
                                                                   std::shared_ptr<FileLocation> src_file_location,
                                                                   std::shared_ptr<FileLocation> dst_file_location) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<FileCopyAction>(
                new FileCopyAction(name, this->shared_this, file, std::move(src_file_location), std::move(dst_file_location)));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
        this->actions.insert(new_action);
        return new_action;
    }

    /**
    * @brief Add a file delete action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file: the file
    * @param file_location: the location from which to delete the file
    * @return a file delete action
    */
    std::shared_ptr<FileDeleteAction>
    CompoundJob::addFileDeleteAction(std::string name, WorkflowFile *file,
                                     std::shared_ptr<FileLocation> file_location) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<FileDeleteAction>(
                new FileDeleteAction(name, this->shared_this, file, std::move(file_location)));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
        this->actions.insert(new_action);
        return new_action;
    }

    /**
     * @brief Add a file registry add entry action
     * @param file_registry: the file registry
     * @param file: the file
     * @param file_location: the file location
     * @return
     */
    std::shared_ptr<FileRegistryAddEntryAction> CompoundJob::addFileRegistryAddEntryAction(
            std::shared_ptr<FileRegistryService> file_registry,
            WorkflowFile *file,
            std::shared_ptr<FileLocation> file_location) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<FileRegistryAddEntryAction>(
                new FileRegistryAddEntryAction(name, this->shared_this, std::move(file_registry), file, std::move(file_location)));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
        this->actions.insert(new_action);
        return new_action;
    }

    /**
     * @brief Add a file registry add entry action
     * @param file_registry: the file registry
     * @param file: the file
     * @param file_location: the file location
     * @return
     */
    std::shared_ptr<FileRegistryDeleteEntryAction> CompoundJob::addFileRegistryDeleteEntryAction(
            std::shared_ptr<FileRegistryService> file_registry,
            WorkflowFile *file,
            std::shared_ptr<FileLocation> file_location) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<FileRegistryDeleteEntryAction>(
                new FileRegistryDeleteEntryAction(name, this->shared_this, std::move(file_registry), file, std::move(file_location)));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
        this->actions.insert(new_action);
        return new_action;
    }

    /**
    * @brief Add a custom action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param lambda_execute: the action execution function
    * @param lambda_terminate: the action termination function
    * @return a custom action
    */
    std::shared_ptr<CustomAction> CompoundJob::addCustomAction(std::string name,
                                                               const std::function<void(std::shared_ptr<ActionExecutor>)> &lambda_execute,
                                                               const std::function<void(std::shared_ptr<ActionExecutor>)> &lambda_terminate) {
        assertJobNotSubmitted();
        auto new_action = std::shared_ptr<CustomAction>(
                new CustomAction(name, this->shared_this, lambda_execute, lambda_terminate));
        new_action->setSharedPtrThis(new_action);
        new_action->setState(Action::State::READY);
        this->actions.insert(new_action);
        return new_action;
    }


    /**
     * @brief Add a dependency between two actions (does nothing if dependency already exists)
     * @param parent: the parent action
     * @param child: the child action
     */
    void CompoundJob::addActionDependency(const std::shared_ptr<Action>& parent, const std::shared_ptr<Action>& child) {
        assertJobNotSubmitted();
        if ((parent == nullptr) or (child == nullptr)) {
            throw std::invalid_argument("CompoundJob::addDependency(): Arguments cannot be nullptr");
        }
        if (parent == child) {
            throw std::invalid_argument("CompoundJob::addDependency(): Cannot add a dependency between a task and itself");
        }
        if (parent->getJob() != this->shared_this or child->getJob() != this->shared_this) {
            throw std::invalid_argument("CompoundJob::addDependency(): Both actions must belong to this job");
        }
        if (pathExists(child, parent)) {
            throw std::invalid_argument("CompoundJob::addDependency(): Adding this dependency would create a cycle");
        }

        child->parents.insert(parent);
        parent->children.insert(child);
        child->updateState();
    }

    /**
     * @brief Add a parent job to this job (be careful not to add circular dependencies, which may lead to deadlocks)
     * @param parent: the parent job
     */
    void CompoundJob::addParentJob(std::shared_ptr<CompoundJob> parent) {
        assertJobNotSubmitted();
        if (parent == nullptr) {
            throw std::invalid_argument("CompoundJob::addParentJob: Cannot add a nullptr parent");
        }
        if (this->pathExists(this->shared_this, parent)) {
            throw std::invalid_argument("CompoundJob::addChildJob(): Adding this dependency would create a cycle");
        }
        this->parents.insert(parent);
        parent->children.insert(this->shared_this);
    }

    /**
     * @brief Add a child job to this job (be careful not to add circular dependencies, which may lead to deadlocks)
     * @param child: the child job
     */
    void CompoundJob::addChildJob(std::shared_ptr<CompoundJob> child) {
        assertJobNotSubmitted();
        if (child == nullptr) {
            throw std::invalid_argument("CompoundJob::addChildJob: Cannot add a nullptr child");
        }
        if (this->pathExists(child, this->shared_this)) {
            throw std::invalid_argument("CompoundJob::addChildJob(): Adding this dependency would create a cycle");
        }
        this->children.insert(child);
        child->parents.insert(this->shared_this);
    }

    /**
     * @brief Get the job's parents
     * @return the (possibly empty) set of parent jobs
     */
    std::set<std::shared_ptr<CompoundJob>> CompoundJob::getParentJobs() {
        return this->parents;
    }

    /**
     * @brief Get the job's children
     * @return the (possibly empty) set of children jobs
     */
    std::set<std::shared_ptr<CompoundJob>> CompoundJob::getChildrenJobs() {
        return this->children;
    }

    /**
     * @brief Get the readiness of the job
     *
     * @return true if ready, false otherwise
     */
    bool CompoundJob::isReady() {
        for (auto const &p : this->parents) {
            if (p->getState() != CompoundJob::State::COMPLETED) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Update the internal State-Action map
     * @param action: the action
     * @param old_state: the action's old state
     * @param new_state: the action's new state
     */
    void CompoundJob::updateStateActionMap(std::shared_ptr<Action> action, Action::State old_state, Action::State new_state) {
        if (old_state != new_state) {
            this->state_task_map[old_state].erase(action);
            this->state_task_map[new_state].insert(action);
        }
    }

    /**
     * @brief Assert that the job has not been submitted
     */
    void CompoundJob::assertJobNotSubmitted() {
        if (this->state != CompoundJob::State::NOT_SUBMITTED) {
            throw std::runtime_error("CompoundJob::assertJobNotSubmitted(): Cannot modify a CompoundJob onces it has been submitted");
        }
    }

    /**
     * @brief Return whether the job has terminated and has done so successfully
     * @return true or false
     */
    bool CompoundJob::hasSuccessfullyCompleted() {
        return (this->actions.size() == this->state_task_map[Action::State::COMPLETED].size());
    }

    /**
     * @brief Print the task map
     */
    void CompoundJob::printTaskMap() {
        std::cerr << "JOB TASK MAP:\n";
        std::cerr << "   TOTAL (" + std::to_string(this->actions.size()) + "): ";
        for (auto const &a : this->actions) {
            std::cerr << a->getName() << " ";
        }
        std::cerr << "\n";
        std::vector<Action::State> states = {Action::State::NOT_READY, Action::State::READY, Action::State::COMPLETED,
                                             Action::State::FAILED, Action::State::KILLED, Action::State::STARTED};
        for (auto const &s : states) {
            std::cerr << "   " << Action::stateToString(s) << " (" + std::to_string(this->state_task_map[s].size()) + ") ";
            for (auto const &a : this->state_task_map[s]) {
                std::cerr << a->getName();
                // std::cerr << "(" << a << ") ";
            }
            std::cerr << "\n";
        }
    }

    /**
     * @brief Return whether the job has terminated and has done so with some tasks having failed
     * @return true or false
     */
    bool CompoundJob::hasFailed() {
        return (this->actions.size() ==
                this->state_task_map[Action::State::NOT_READY].size() +
                        this->state_task_map[Action::State::COMPLETED].size() +
                        this->state_task_map[Action::State::KILLED].size() +
                        this->state_task_map[Action::State::FAILED].size());
    }

    /**
     * @brief Set all actions to FAILED for the same failure cause (e.g., a job-level failure)
     * @param cause: a failure cause
     */
    void CompoundJob::setAllActionsFailed(std::shared_ptr<FailureCause> cause) {
        for (auto const &action : this->actions) {
            action->setState(Action::State::FAILED);
            action->setFailureCause(cause);
        }

    }

    /**
     * Determine whether there is a path between two actions
     * @param a: an action
     * @param b: another action
     * @return
     */
    bool CompoundJob::pathExists(const std::shared_ptr<Action>& a, const std::shared_ptr<Action> &b) {
        auto children = a->getChildren();
        if (children.find(b) != children.end()) {
            return true;
        }
        bool path_exists = false;
        for (auto const &c : children) {
            path_exists = path_exists || this->pathExists(c, b);
        }
        return path_exists;
    }

    /**
     * Determine whether there is a path between two jobs
     * @param a: a job
     * @param b: another job
     * @return
     */
    bool CompoundJob::pathExists(const std::shared_ptr<CompoundJob>& a, const std::shared_ptr<CompoundJob> &b) {
        auto children = a->getChildren();
        if (children.find(b) != children.end()) {
            return true;
        }
        bool path_exists = false;
        for (auto const &c : children) {
            path_exists = path_exists || this->pathExists(c, b);
        }
        return path_exists;
    }

    /**
     * @brief Returns the job's child jobs, if any
     * @return a set of jobs
     */
    std::set<std::shared_ptr<CompoundJob>> &CompoundJob::getChildren() {
        return this->children;
    }

    /**
     * @brief Returns the job's parent jobs, if any
     * @return a set of jobs
     */
    std::set<std::shared_ptr<CompoundJob>> &CompoundJob::getParents() {
        return this->parents;
    }

    /**
     * @brief Remove an action from the job
     * @param action: the action to remove
     */
    void CompoundJob::removeAction(shared_ptr<Action> &action) {
        assertJobNotSubmitted();
        this->state_task_map[action->getState()].erase(action);
        for (auto const &parent : action->parents) {
            parent->children.erase(action);
        }
        for (auto const &child : action->children) {
            child->parents.erase(action);
            child->updateState();
        }
        this->actions.erase(action);
    }

    /**
     * @brief Print the list of actions with their children and parents
     */
    void CompoundJob::printActionDependencies() {
        for (auto const &action : this->actions) {
            std::cerr << "* " << Action::getActionTypeAsString(action) << "-" << action->getName() << " ("  << action->getStateAsString() << ")\n";
            if (not action->getChildren().empty()) {
                std::cerr << "  CHILDREN:\n";
                for (auto const &child: action->getChildren()) {
                    std::cerr << "    - " << Action::getActionTypeAsString(child) << "-" << child->getName() << "\n";
                }
            }
            if (not action->getParents().empty()) {
                std::cerr << "  PARENTS:\n";
                for (auto const &parent: action->getParents()) {
                    std::cerr << "    - " << Action::getActionTypeAsString(parent) << "-" << parent->getName() << "\n";
                }
            }
        }

    }

}
