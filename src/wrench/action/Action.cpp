/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/job/CompoundJob.h>
#include <wrench/action/Action.h>

#include <utility>

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
        this->state = Action::State::READY;
        this->start_date = -1.0;
        this->end_date = -1.0;

        this->simulate_computation_as_sleep = false;
        this->thread_creation_overhead = 0.0;
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
    std::string Action::getName() const {
        return this->name;
    }

    /**
     * @brief Returns the action's state
     * @return the state
     */
    Action::State Action::getState() const {
        return this->state;
    }

    /**
    * @brief Returns the action's state as a string
    * @return the state
    */
    std::string Action::getStateAsString() const {
        switch(this->state) {
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
        auto old_state = this->state;
        this->job->updateStateActionMap(this->shared_ptr_this, old_state, new_state);
        this->state = new_state;
    }

    /**
     * @brief Returns the action's failure cause
     * @return an internal state
     */
    std::shared_ptr<FailureCause> Action::getFailureCause() const {
        return this->failure_cause;
    }

    /**
     * @brief Sets the action's start date
     * @param date: the date 
     */
    void Action::setStartDate(double date) {
        this->start_date = date;
    }

    /**
     * @brief Sets the action's end date
     * @param date: the date
     */
    void Action::setEndDate(double date) {
        this->end_date = date;
    }

    /**
     * @brief Returns ths action's started date (-1.0 if not started)
     * @return a data
     */
    double Action::getStartDate() const {
        return this->start_date;
    }

    /**
     * @brief Returns ths action's killed date (-1.0 if not ended)
     * @return a data
     */
    double Action::getEndDate() const {
        return this->end_date;
    }

    /**
     * @brief Sets the action's failure cause
     * @param failure_cause: the failure cause
     */
    void Action::setFailureCause(std::shared_ptr<FailureCause> failure_cause) {
        this->failure_cause = failure_cause;
    }

    /**
     * @brief Set whether simulation should be simulated as sleep (default = false)
     * @param simulate_computation_as_sleep: true or false
     */
    void Action::setSimulateComputationAsSleep(bool simulate_computation_as_sleep) {
        this->simulate_computation_as_sleep = simulate_computation_as_sleep;
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
        if (this->state != Action::State::NOT_READY and this->state != Action::State::READY) {
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
            this->state = Action::State::READY;
        } else {
            this->state = Action::State::NOT_READY;
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
        return ULONG_MAX;
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
    void Action::setSharedPtrThis(std::shared_ptr<Action> shared_ptr_this) {
        this->shared_ptr_this = shared_ptr_this;
    }

}