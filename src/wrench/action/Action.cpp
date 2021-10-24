/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/action/Action.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     * @param name: the action's name (if empty, a unique name will be picked for you)
     * @param job: the job that contains this action
     */
    Action::Action(std::string name, std::shared_ptr<CompoundJob> job) {
        if (name.empty()) {
            this->name = "action_" + std::to_string(Action::getNewUniqueNumber());
        } else {
            this->name = name;
        }
        this->job = std::move(job);
        this->state = Action::State::READY;
        this->start_date = -1.0;
        this->end_date = -1.0;
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
    void Action::setState(Action::State state) {
        this->state = state;
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

}