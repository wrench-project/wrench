/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ACTION_H
#define WRENCH_ACTION_H

#include <memory>
#include <string>

namespace wrench {

    class CompoundJob;
    class FailureCause;

    class Action {

    public:

        /** @brief Action states */
        enum State {
            /** @brief Not ready (not ready, because depends on other actions) */
            NOT_READY,
            /** @brief Ready (ready to execute) */
            READY,
            /** @brief Started (is being executed) */
            STARTED,
            /** @brief Completed (successfully completed) */
            COMPLETED,
            /** @brief Killed (due to user actions, service being terminated, etc.) */
            KILLED,
            /** @brief Failed (has failed) */
            FAILED
        };

        std::string getName() const;
        std::shared_ptr<CompoundJob> getJob() const;
        Action::State getState() const;
        std::string getStateAsString() const;
        double getStartDate() const;
        double getEndDate() const;
        std::shared_ptr<FailureCause> getFailureCause() const;


    protected:

        friend class SleepActionExecutor;
        friend class ComputeActionExecutor;

        void setState(Action::State state);
        virtual ~Action() = default;
        Action(std::string name, std::shared_ptr<CompoundJob> job);
        std::shared_ptr<CompoundJob> job;

        void setStartDate(double date);
        void setEndDate(double date);
        void setFailureCause(std::shared_ptr<FailureCause> failure_cause);

    private:

        std::string name;
        Action::State state;

        double start_date;
        double end_date;
        std::shared_ptr<FailureCause> failure_cause;

        static unsigned long getNewUniqueNumber();

    };
}


#endif //WRENCH_ACTION_H
