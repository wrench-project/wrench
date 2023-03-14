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
#include <wrench/action/MPIAction.h>
#include "wrench/services/storage/storage_helpers/FileLocation.h"
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
        : Job(std::move(name), std::move(job_manager)),
          state(CompoundJob::State::NOT_SUBMITTED), priority(0.0) {
        this->state_task_map[Action::State::NOT_READY] = {};
        this->state_task_map[Action::State::COMPLETED] = {};
        this->state_task_map[Action::State::KILLED] = {};
        this->state_task_map[Action::State::FAILED] = {};
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
        switch (this->state) {
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
    * @brief Set the job's priority (the higher the value, the higher the priority)
    * @param p: a priority
    */
    void CompoundJob::setPriority(double p) {
        assertJobNotSubmitted();
        this->priority = p;
    }

    /**
     * @brief Add a sleep action to the job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param sleep_time: the time to sleep, in seconds
     * @return a sleep action
     */
    std::shared_ptr<SleepAction> CompoundJob::addSleepAction(const std::string &name, double sleep_time) {
        auto new_action = std::shared_ptr<SleepAction>(new SleepAction(name, sleep_time));
        this->addAction(new_action);
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
    std::shared_ptr<ComputeAction> CompoundJob::addComputeAction(const std::string &name,
                                                                 double flops,
                                                                 double ram,
                                                                 unsigned long min_num_cores,
                                                                 unsigned long max_num_cores,
                                                                 const std::shared_ptr<ParallelModel> &parallel_model) {
        auto new_action = std::shared_ptr<ComputeAction>(
                new ComputeAction(name, flops, ram, min_num_cores, max_num_cores, parallel_model));
        this->addAction(new_action);
        return new_action;
    }

    /**
     * @brief Add a file read action to a job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file: the file to read
     * @param storage_service: the storage service to read the file from
     * @return a file read action
     */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(const std::string &name,
                                                                   const std::shared_ptr<DataFile> &file,
                                                                   const std::shared_ptr<StorageService> &storage_service) {
        return addFileReadAction(name, FileLocation::LOCATION(storage_service, file));
    }

    /**
     * @brief Add a file read action to a job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file: the file to read
     * @param storage_service: the storage service to read the file from
     * @param num_bytes_to_read: the number of bytes to read
     * @return a file read action
     */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(const std::string &name,
                                                                   const std::shared_ptr<DataFile> &file,
                                                                   const std::shared_ptr<StorageService> &storage_service,
                                                                   const double num_bytes_to_read) {
        return addFileReadAction(name, FileLocation::LOCATION(storage_service, file), num_bytes_to_read);
    }


    /**
     * @brief Add a file write action to a job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file: the file to write
     * @param storage_service: the storage service to write the file to
     * @return a file write action
     */
    std::shared_ptr<FileWriteAction> CompoundJob::addFileWriteAction(const std::string &name,
                                                                     const std::shared_ptr<DataFile> &file,
                                                                     const std::shared_ptr<StorageService> &storage_service) {
        return addFileWriteAction(name, FileLocation::LOCATION(storage_service, file));
    }

    /**
     * @brief Add a file copy action to a job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file: the file to copy
     * @param src_storage_service: the source storage service
     * @param dst_storage_service: the destination storage service
     * @return a file copy action
     */
    std::shared_ptr<FileCopyAction> CompoundJob::addFileCopyAction(const std::string &name,
                                                                   const std::shared_ptr<DataFile> &file,
                                                                   const std::shared_ptr<StorageService> &src_storage_service,
                                                                   const std::shared_ptr<StorageService> &dst_storage_service) {
        return addFileCopyAction(name, FileLocation::LOCATION(src_storage_service, file), FileLocation::LOCATION(dst_storage_service, file));
    }

    /**
     * @brief Add a file delete action to a job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file: the file to delete
     * @param storage_service: the storage service on which the file is
     * @return a file delete action
     */
    std::shared_ptr<FileDeleteAction> CompoundJob::addFileDeleteAction(const std::string &name,
                                                                       const std::shared_ptr<DataFile> &file,
                                                                       const std::shared_ptr<StorageService> &storage_service) {
        return addFileDeleteAction(name, FileLocation::LOCATION(storage_service, file));
    }
    /**
     * @brief Add a file read action to the job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file_location: the file's location
     * @return a file read action
     */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(const std::string &name,
                                                                   const std::shared_ptr<FileLocation> &file_location) {
        std::vector<std::shared_ptr<FileLocation>> v = {file_location};
        return this->addFileReadAction(name, v, -1.0);
    }

    /**
     * @brief Add a file read action to the job
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param file_location: the file's location
     * @param num_bytes_to_read: the number of bytes to read
     * @return a file read action
     */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(const std::string &name,
                                                                   const std::shared_ptr<FileLocation> &file_location,
                                                                   const double num_bytes_to_read) {
        std::vector<std::shared_ptr<FileLocation>> v = {file_location};
        return this->addFileReadAction(name, v, num_bytes_to_read);
    }


    /**
    * @brief Add a file read action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file_locations: the locations to read the file from (will be tried in order until one succeeds)
    * @return a file read action
    */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(const std::string &name,
                                                                   const std::vector<std::shared_ptr<FileLocation>> &file_locations) {
        return this->addFileReadAction(name, file_locations, -1.0);
    }

    /**
    * @brief Add a file read action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file_locations: the locations to read the file from (will be tried in order until one succeeds)
    * @param num_bytes_to_read: number of bytes to read
    * @return a file read action
    */
    std::shared_ptr<FileReadAction> CompoundJob::addFileReadAction(const std::string &name,
                                                                   const std::vector<std::shared_ptr<FileLocation>> &file_locations,
                                                                   const double num_bytes_to_read) {
        auto new_action = std::shared_ptr<FileReadAction>(
                new FileReadAction(name, file_locations, num_bytes_to_read));
        this->addAction(new_action);
        return new_action;
    }

    /**
    * @brief Add a file write action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file_location: the file's location where it should be written
    * @return a file write action
    */
    std::shared_ptr<FileWriteAction> CompoundJob::addFileWriteAction(const std::string &name,
                                                                     const std::shared_ptr<FileLocation> &file_location) {
        auto new_action = std::shared_ptr<FileWriteAction>(
                new FileWriteAction(name, {file_location}));
        this->addAction(new_action);
        return new_action;
    }

    /**
    * @brief Add a file copy action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param src_file_location: the file's location where it should be read
    * @param dst_file_location: the file's location where it should be written
    * @return a file copy action
    */
    std::shared_ptr<FileCopyAction> CompoundJob::addFileCopyAction(const std::string &name,
                                                                   const std::shared_ptr<FileLocation> &src_file_location,
                                                                   const std::shared_ptr<FileLocation> &dst_file_location) {
        auto new_action = std::shared_ptr<FileCopyAction>(
                new FileCopyAction(name, src_file_location, dst_file_location));
        this->addAction(new_action);
        return new_action;
    }

    /**
    * @brief Add a file delete action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file_location: the location from which to delete the file
    * @return a file delete action
    */
    std::shared_ptr<FileDeleteAction>
    CompoundJob::addFileDeleteAction(const std::string &name,
                                     const std::shared_ptr<FileLocation> &file_location) {
        auto new_action = std::shared_ptr<FileDeleteAction>(
                new FileDeleteAction(name, file_location->getFile(), file_location));
        this->addAction(new_action);
        return new_action;
    }

    /**
     * @brief Add a file registry add entry action
     * @param name: the action's name
     * @param file_registry: the file registry
     * @param file_location: the file location
     * @return
     */
    std::shared_ptr<FileRegistryAddEntryAction> CompoundJob::addFileRegistryAddEntryAction(
            const std::string &name,
            const std::shared_ptr<FileRegistryService> &file_registry,
            const std::shared_ptr<FileLocation> &file_location) {
        auto new_action = std::shared_ptr<FileRegistryAddEntryAction>(
                new FileRegistryAddEntryAction(name, file_registry, file_location));
        this->addAction(new_action);
        return new_action;
    }

    /**
     * @brief Add a file registry add entry action
     * @param name: the action's name
     * @param file_registry: the file registry
     * @param file_location: the file location
     * @return
     */
    std::shared_ptr<FileRegistryDeleteEntryAction> CompoundJob::addFileRegistryDeleteEntryAction(
            const std::string &name,
            const std::shared_ptr<FileRegistryService> &file_registry,
            const std::shared_ptr<FileLocation> &file_location) {
        auto new_action = std::shared_ptr<FileRegistryDeleteEntryAction>(
                new FileRegistryDeleteEntryAction(name, file_registry, file_location));
        this->addAction(new_action);
        return new_action;
    }

    /**
    * @brief Add a custom action to the job
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param ram: the action's RAM footprint
    * @param num_cores: the action's allocated number of cores
    * @param lambda_execute: the action execution function
    * @param lambda_terminate: the action termination function
    * @return a custom action
    */
    std::shared_ptr<CustomAction> CompoundJob::addCustomAction(const std::string &name,
                                                               double ram,
                                                               unsigned long num_cores,
                                                               const std::function<void(std::shared_ptr<ActionExecutor>)> &lambda_execute,
                                                               const std::function<void(std::shared_ptr<ActionExecutor>)> &lambda_terminate) {
        auto new_action = std::shared_ptr<CustomAction>(
                new CustomAction(name, ram, num_cores, lambda_execute, lambda_terminate));
        this->addAction(new_action);
        return new_action;
    }


    /**
     * @brief Add a custom action to the job
     * @param custom_action: a custom action
     * @return the custom action that was passed in
     */
    std::shared_ptr<CustomAction> CompoundJob::addCustomAction(std::shared_ptr<CustomAction> custom_action) {
        this->addAction(custom_action);
        return custom_action;
    }

    /**
     * @brief Add an MPI action to the job.  The intended
     *         use-case for an MPI action is that never runs concurrently with other actions
     *        within its job, and that that job is submitted to a BatchComputeService, so that it
     *        has a set of resources dedicated to it. If the job
     *        is submitted to a BareMetalComputeService, this action will use all of that service's
     *        resources, regardless of other running actions/jobs on that service.
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param mpi_code: a lambda/function that implements the MPI code that MPI processes should execute
     * @param num_processes: the number of MPI processes that will be started.
     * @param num_cores_per_process: the number of core that each MPI process should use. Note that this is not enforced by the runtime system.
*                  If the processes compute with more cores, then they will cause time-sharing on cores.
     * @return an MPI action
     */
    std::shared_ptr<MPIAction> CompoundJob::addMPIAction(const std::string &name,
                                                         const std::function<void(const std::shared_ptr<ActionExecutor> &action_executor)> &mpi_code,
                                                         unsigned long num_processes,
                                                         unsigned long num_cores_per_process) {
        auto new_action = std::shared_ptr<MPIAction>(
                new MPIAction(name, num_processes, num_cores_per_process, mpi_code));
        this->addAction(new_action);
        return new_action;
    }

    /**
     * @brief Helper method to add an action to the job
     * @param action: an action
     */
    void CompoundJob::addAction(const std::shared_ptr<Action> &action) {
        assertJobNotSubmitted();
        assertActionNameDoesNotAlreadyExist(action->getName());
        action->job = this->getSharedPtr();
        action->setState(Action::State::READY);
        this->actions.insert(action);
        this->name_map[action->getName()] = action;
    }

    /**
     * @brief Add a dependency between two actions (does nothing if dependency already exists)
     * @param parent: the parent action
     * @param child: the child action
     */
    void CompoundJob::addActionDependency(const std::shared_ptr<Action> &parent, const std::shared_ptr<Action> &child) {
        assertJobNotSubmitted();
        if ((parent == nullptr) or (child == nullptr)) {
            throw std::invalid_argument("CompoundJob::addDependency(): Arguments cannot be nullptr");
        }
        if (parent == child) {
            throw std::invalid_argument("CompoundJob::addDependency(): Cannot add a dependency between a task and itself");
        }
        if (parent->getJob() != this->getSharedPtr() or child->getJob() != this->getSharedPtr()) {
            throw std::invalid_argument("CompoundJob::addDependency(): Both actions must belong to this job");
        }
        if (pathExists(child, parent)) {
            throw std::invalid_argument("CompoundJob::addDependency(): Adding this dependency would create a cycle");
        }

        child->parents.insert(parent.get());
        parent->children.insert(child.get());
        child->updateState();
    }

    /**
     * @brief Add a parent job to this job (be careful not to add circular dependencies, which may lead to deadlocks)
     * @param parent: the parent job
     */
    void CompoundJob::addParentJob(const std::shared_ptr<CompoundJob> &parent) {
        assertJobNotSubmitted();
        if (parent == nullptr) {
            throw std::invalid_argument("CompoundJob::addParentJob: Cannot add a nullptr parent");
        }
        if (pathExists(this->getSharedPtr(), parent)) {
            throw std::invalid_argument("CompoundJob::addChildJob(): Adding this dependency would create a cycle");
        }
        this->parents.insert(parent);
        parent->children.insert(this->getSharedPtr());
    }

    /**
     * @brief Add a child job to this job (be careful not to add circular dependencies, which may lead to deadlocks)
     * @param child: the child job
     */
    void CompoundJob::addChildJob(const std::shared_ptr<CompoundJob> &child) {
        assertJobNotSubmitted();
        if (child == nullptr) {
            throw std::invalid_argument("CompoundJob::addChildJob: Cannot add a nullptr child");
        }
        if (pathExists(child, this->getSharedPtr())) {
            throw std::invalid_argument("CompoundJob::addChildJob(): Adding this dependency would create a cycle");
        }
        this->children.insert(child);
        child->parents.insert(this->getSharedPtr());
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

        return std::all_of(this->parents.begin(), this->parents.end(),
                           [](const std::shared_ptr<CompoundJob> &e) {
                               return e->getState() == CompoundJob::State::COMPLETED;
                           });

        //        for (auto const &p: this->parents) {
        //            if (p->getState() != CompoundJob::State::COMPLETED) {
        //                return false;
        //            }
        //        }
        //        return true;
    }

    /**
     * @brief Update the internal State-Action map
     * @param action: the action
     * @param old_state: the action's old state
     * @param new_state: the action's new state
     */
    void CompoundJob::updateStateActionMap(const std::shared_ptr<Action> &action, Action::State old_state, Action::State new_state) {
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
            throw std::runtime_error("CompoundJob::assertJobNotSubmitted(): Cannot modify a CompoundJob once it has been submitted");
        }
    }

    /**
     * @brief Assert that the job has not been submitted
     */
    void CompoundJob::assertActionNameDoesNotAlreadyExist(const std::string &name) {
        if (this->name_map.find(name) != this->name_map.end()) {
            throw std::runtime_error("CompoundJob::assertActionNameDoesNotAlreadyExist(): Task with name " + name + " already exists");
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
        for (auto const &a: this->actions) {
            std::cerr << a->getName() << " ";
        }
        std::cerr << "\n";
        std::vector<Action::State> states = {Action::State::NOT_READY, Action::State::READY, Action::State::COMPLETED,
                                             Action::State::FAILED, Action::State::KILLED, Action::State::STARTED};
        for (auto const &s: states) {
            std::cerr << "   " << Action::stateToString(s) << " (" + std::to_string(this->state_task_map[s].size()) + ") ";
            for (auto const &a: this->state_task_map[s]) {
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
        auto not_ready = this->state_task_map[Action::State::NOT_READY].size();
        auto completed = this->state_task_map[Action::State::COMPLETED].size();
        auto killed = this->state_task_map[Action::State::KILLED].size();
        auto failed = this->state_task_map[Action::State::FAILED].size();

        return (this->actions.size() == not_ready + completed + killed + failed) && (this->actions.size() != completed);
    }

    /**
     * @brief Set all actions to FAILED for the same failure cause (e.g., a job-level failure)
     * @param cause: a failure cause
     */
    void CompoundJob::setAllActionsFailed(const std::shared_ptr<FailureCause> &cause) {
        for (auto const &action: this->actions) {
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
    bool CompoundJob::pathExists(const std::shared_ptr<Action> &a, const std::shared_ptr<Action> &b) {
        auto current_children = a->children;
        if (current_children.find(b.get()) != current_children.end()) {
            return true;
        }
        bool path_exists = false;
        for (auto const &c: current_children) {
            path_exists = path_exists || this->pathExists(c->getSharedPtr(), b);
        }
        return path_exists;
    }

    /**
     * Determine whether there is a path between two jobs
     * @param a: a job
     * @param b: another job
     * @return
     */
    bool CompoundJob::pathExists(const std::shared_ptr<CompoundJob> &a, const std::shared_ptr<CompoundJob> &b) {
        auto current_children = a->getChildren();
        if (current_children.find(b) != current_children.end()) {
            return true;
        }
        bool path_exists = false;
        for (auto const &c: current_children) {
            path_exists = path_exists || CompoundJob::pathExists(c, b);
        }
        return path_exists;
    }

    /**
     * @brief Returns the job's child jobs, if any
     * @return a set of jobs (a reference)
     */
    std::set<std::shared_ptr<CompoundJob>> &CompoundJob::getChildren() {
        return this->children;
    }

    //    /**
    //     * @brief Returns a reference to the job's parent jobs, if any (use with caution)
    //     * @return a set of jobs (a reference)
    //     */
    //    std::set<std::shared_ptr<CompoundJob>> &CompoundJob::getParents() {
    //        return this->parents;
    //    }

    /**
     * @brief Remove an action from the job
     * @param action: the action to remove
     */
    void CompoundJob::removeAction(shared_ptr<Action> &action) {
        assertJobNotSubmitted();
        this->state_task_map[action->getState()].erase(action);
        for (auto const &parent: action->parents) {
            parent->children.erase(action.get());
        }
        for (auto const &child: action->children) {
            child->parents.erase(action.get());
            child->updateState();
        }
        this->actions.erase(action);
        this->name_map.erase(action->getName());
    }

    /**
     * @brief Print the list of actions with their children and parents
     */
    void CompoundJob::printActionDependencies() {
        for (auto const &action: this->actions) {
            std::cerr << "* " << Action::getActionTypeAsString(action) << "-" << action->getName() << " (" << action->getStateAsString() << ")\n";
            if (not action->children.empty()) {
                std::cerr << "  CHILDREN:\n";
                for (auto const &child: action->children) {
                    std::cerr << "    - " << Action::getActionTypeAsString(child->getSharedPtr()) << "-" << child->getName() << "\n";
                }
            }
            if (not action->parents.empty()) {
                std::cerr << "  PARENTS:\n";
                for (auto const &parent: action->parents) {
                    std::cerr << "    - " << Action::getActionTypeAsString(parent->getSharedPtr()) << "-" << parent->getName() << "\n";
                }
            }
        }
    }

    /**
     * @brief Determine whether an action with a given name exists in job
     * @param name: action name
     * @return true or false
     */
    bool CompoundJob::hasAction(const std::string &name) {
        return (this->name_map.find(name) != this->name_map.end());
    }

    /**
     * @brief Get the minimum required num cores to run the job
     * @return a number of cores
     */
    unsigned long CompoundJob::getMinimumRequiredNumCores() {
        unsigned long min_num_cores = 0;
        for (auto const &action: this->actions) {
            min_num_cores = (min_num_cores < action->getMinNumCores() ? action->getMinNumCores() : min_num_cores);
        }
        return min_num_cores;
    }

    /**
   * @brief Get the minimum required amount of memory to run the job
   * @return a number of bytes
   */
    double CompoundJob::getMinimumRequiredMemory() {
        double min_ram = 0;
        for (auto const &action: this->actions) {
            min_ram = (min_ram < action->getMinRAMFootprint() ? action->getMinRAMFootprint() : min_ram);
        }
        return min_ram;
    }

    /**
     * @brief Determine whether the job uses scratch
     * @return true if the job uses scratch, false otherwise
     */
    bool CompoundJob::usesScratch() {
        for (auto const &a: this->actions) {
            if (a->usesScratch()) {
                return true;
            }
        }
        return false;
    }


}// namespace wrench
