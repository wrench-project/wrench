/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/action/Action.h>
#include <wrench/action/SleepAction.h>
#include <wrench/action/ComputeAction.h>
#include <wrench/action/FileReadAction.h>
#include <wrench/action/FileWriteAction.h>
#include <wrench/action/FileCopyAction.h>
#include <wrench/action/FileDeleteAction.h>
#include <wrench/action/CustomAction.h>
#include <wrench/action/FileRegistryAddEntryAction.h>
#include <wrench/action/FileRegistryDeleteEntryAction.h>

#include <utility>
#include <wrench/simgrid_S4U_util/S4U_VirtualMachine.h>

WRENCH_LOG_CATEGORY(wrench_action, "Log category for  Action");

namespace wrench {

    /**
     * @brief Constructor
     * @param name: the action's name (if empty, a unique name will be picked)
     * @param prefix: the action's name prefix (if name is empty)
     * @param job: the job that contains this action
     */
    Action::Action(const std::string& name, const std::string& prefix, std::shared_ptr<CompoundJob> job) {
        if (name.empty()) {
            this->name = prefix + std::to_string(Action::getNewUniqueNumber());
        } else {
            this->name = name;
        }
        this->job = std::move(job);

        this->execution_history.push(Action::ActionExecution());
        this->simulate_computation_as_sleep = false;
        this->thread_creation_overhead = 0.0;
        this->priority = 0.0;
    }

    /**
     * @brief Generate a unique action number
     * @return a unique number
     */
    unsigned long Action::getNewUniqueNumber() {
        static unsigned long sequence_number = 0;
        return (sequence_number++);
    }

    /**
     * @brief Returns the action's name
     * @return the name
     */
    const std::string &Action::getName() const {
        return this->name;
    }

    /**
     * @brief Returns the action's state
     * @return the state
     */
    Action::State Action::getState() const {
        return this->execution_history.top().state;
    }

    /**
     * @brief Convert an action state to a human-readable string
     * @param state: an action state
     * @return a string
     */
    std::string Action::stateToString(Action::State state) {
        switch(state) {
            case Action::State::NOT_READY:
                return "NOT READY";
            case Action::State::READY:
                return "READY";
            case Action::State::STARTED:
                return "STARTED";
            case Action::State::COMPLETED:
                return "COMPLETED";
            case Action::State::KILLED:
                return "KILLED";
            case Action::State::FAILED:
                return "FAILED";
            default:
                return "???";
        }
    }


    /**
    * @brief Returns the action's state as a human-readable string
    * @return a string
    */
    std::string Action::getStateAsString() const {
        return Action::stateToString(this->execution_history.top().state);
    }

    /**
     * @brief Returns the job this action belongs to
     * @return the job
     */
    std::shared_ptr<CompoundJob> Action::getJob() const {
        return this->job;
    }

    /**
     * @brief Sets the action's (visible) state
     * @param state: the state
     */
    void Action::setState(Action::State new_state) {
        auto old_state = this->execution_history.top().state;
        this->job->updateStateActionMap(this->shared_ptr_this, old_state, new_state);
//        std::cerr << "ACTION " << this->getName() << ": " << Action::stateToString(old_state) << "-->" << Action::stateToString(new_state) << "\n";
        this->execution_history.top().state = new_state;
    }

    /**
     * @brief Returns the action's failure cause
     * @return an internal state
     */
    std::shared_ptr<FailureCause> Action::getFailureCause() const {
        return this->execution_history.top().failure_cause;
    }

    /**
     * @brief Sets the action's start date
     * @param date: the date 
     */
    void Action::setStartDate(double date) {
        this->execution_history.top().start_date = date;
    }

    /**
     * @brief Sets the action's end date
     * @param date: the date
     */
    void Action::setEndDate(double date) {
        this->execution_history.top().end_date = date;
    }

    /**
     * @brief Returns ths action's started date (-1.0 if not started)
     * @return a data
     */
    double Action::getStartDate() const {
        return this->execution_history.top().start_date;
    }

    /**
     * @brief Returns this action's end date (-1.0 if not ended)
     * @return a data
     */
    double Action::getEndDate() const {
        return this->execution_history.top().end_date;
    }

    /**
     * @brief Sets the action's failure cause
     * @param failure_cause: the failure cause
     */
    void Action::setFailureCause(std::shared_ptr<FailureCause> failure_cause) {
        this->execution_history.top().failure_cause = std::move(failure_cause);
    }

    /**
    * @brief Sets the action's execution hosts (and the action's physical execution host)
    * @param host: a hostname
    */
    void Action::setExecutionHost(std::string host) {
        std::string physical_host;

        this->execution_history.top().execution_host = host;

        if (S4U_VirtualMachine::vm_to_pm_map.find(host) != S4U_VirtualMachine::vm_to_pm_map.end()) {
            physical_host = S4U_VirtualMachine::vm_to_pm_map[host];
        } else {
            physical_host = host;
        }
        this->execution_history.top().physical_execution_host = physical_host;
    }

    /**
     * @brief Sets the action's allocated number of cores
     * @param num_cores: a number of cores
     */
    void Action::setNumCoresAllocated(unsigned long num_cores) {
        this->execution_history.top().num_cores_allocated = num_cores;
    }

    /**
     * @brief Sets the action's allocated RAM
     * @param ram: a number of bytes
     */
    void Action::setRAMAllocated(double ram) {
        this->execution_history.top().ram_allocated = ram;
    }

    /**
     * @brief Set whether simulation should be simulated as sleep (default = false)
     * @param simulate_computation_as_sleep: true or false
     */
    void Action::setSimulateComputationAsSleep(bool simulate_computation_as_sleep) {
        this->simulate_computation_as_sleep = simulate_computation_as_sleep;
    }

    /**
     * @brief Creat a new execution data structure (e.g., after a restart)
     */
    void Action::newExecution(Action::State state) {
        Action::State old_state;
        if (!this->execution_history.empty()) {
            old_state = this->execution_history.top().state;
        } else {
            old_state = Action::State::READY;
        }
        this->execution_history.push(Action::ActionExecution());
        this->execution_history.top().state = old_state;
        this->setState(state);
    }

    /**
     * @brief Set the thread creation overhead (default = zero)
     * @param overhead_in_seconds: overhead in seconds
     */
    void Action::setThreadCreationOverhead(double overhead_in_seconds) {
        this->thread_creation_overhead = overhead_in_seconds;
    }

    /**
     * @brief Update the action's state
     */
    void Action::updateState() {
        // Do nothing if task state is neither ready nor not ready
        if (this->execution_history.top().state != Action::State::NOT_READY and this->execution_history.top().state != Action::State::READY) {
            return;
        }
        // Ready?
        bool ready = true;
        for (auto const &p : this->parents) {
            if (p->getState() != Action::State::COMPLETED) {
                ready = false;
            }
        }
        if (ready) {
            this->setState(Action::State::READY);
        } else {
            this->setState(Action::State::NOT_READY);
        }
    }

    /**
     * @brief Get the minimum number of cores required to execute the action
     * @return a number of cores
     */
    unsigned long Action::getMinNumCores() const {
        return 0;
    }

    /**
     * @brief Get the maximum number of cores that can be used to execute the action
     * @return a number of cores
     */
    unsigned long Action::getMaxNumCores() const {
        return 0;
    }

    /**
     * @brief Get the minimum required amount of RAM to execute the action
     * @return a number of bytes
     */
    double Action::getMinRAMFootprint() const {
        return 0.0;
    }

    /**
     * @brief Set the shared_ptr to this
     * @param
     */
    void Action::setSharedPtrThis(std::shared_ptr<Action> shared_ptr) {
        this->shared_ptr_this = std::move(shared_ptr);
    }

    /**
     * @brief Retrieve the execution history
     * @return the execution history
     */
    std::stack<Action::ActionExecution> &Action::getExecutionHistory() {
        return this->execution_history;
    }


    /**
     * @brief Get the action's children
     * @return a set of children
     */
    std::set<std::shared_ptr<Action>> &Action::getChildren() {
        return this->children;
    }

    /**
     * @brief Get the action's parents
     * @return a set of parents
     */
    std::set<std::shared_ptr<Action>> &Action::getParents() {
        return this->parents;
    }

    /**
     * @brief Set the action's priority
     * @param priority: a priority
     */
    void Action::setPriority(double priority) {
        this->priority = priority;
    }

    /**
   * @brief Get the action's priority
   * @return a priority
   */
    double Action::getPriority() const {
        return this->priority;
    }

    /**
    * @brief Returns an action's type as a human-readable string
    * @param action: the action
    * @return the type as a string
    */
    std::string Action::getActionTypeAsString(const std::shared_ptr<Action>& action) {
        if (std::dynamic_pointer_cast<SleepAction>(action)) {
            return "SLEEP-";
        } else if (std::dynamic_pointer_cast<ComputeAction>(action)) {
            return "COMPUTE-";
        } else if (std::dynamic_pointer_cast<FileReadAction>(action)) {
            return "FILEREAD-";
        } else if (std::dynamic_pointer_cast<FileWriteAction>(action)) {
            return "FILEWRITE-";
        } else if (std::dynamic_pointer_cast<FileCopyAction>(action)) {
            return "FILECOPY-";
        } else if (std::dynamic_pointer_cast<FileDeleteAction>(action)) {
            return "FILEDELETE-";
        } else if (std::dynamic_pointer_cast<CustomAction>(action)) {
            return "CUSTOM-";
        } else if (std::dynamic_pointer_cast<FileRegistryAddEntryAction>(action)) {
            return "FILEREGISTRY_ADD-";
        } else if (std::dynamic_pointer_cast<FileRegistryDeleteEntryAction>(action)) {
            return "FILEREGISTRY_DELETE-";
        } else {
            return "????-";
        }
    }

    /**
     * @brief Determine whether the action uses scratch
     * @return true if the action uses scratch, false otherwise
     */
    bool Action::usesScratch() const {
        return false;
    }


}