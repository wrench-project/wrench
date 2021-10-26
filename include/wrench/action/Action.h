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
#include <set>

namespace wrench {

    class CompoundJob;
    class FailureCause;
    class ActionExecutor;

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

        void setSimulateComputationAsSleep(bool simulate_computation_as_sleep);
        void setThreadCreationOverhead(double overhead_in_seconds);

    protected:

        friend class CompoundJob;
        friend class ActionExecutor;

        virtual ~Action() = default;
        Action(const std::string& name, const std::string& prefix, std::shared_ptr<CompoundJob> job);

        virtual void execute(std::shared_ptr<ActionExecutor> action_executor, unsigned long num_threads, double ram_footprint) = 0;
        virtual void terminate(std::shared_ptr<ActionExecutor> action_executor) = 0;


        void setState(Action::State state);
        void setStartDate(double date);
        void setEndDate(double date);
        void setFailureCause(std::shared_ptr<FailureCause> failure_cause);

        bool simulate_computation_as_sleep;
        double thread_creation_overhead;

        std::set<std::shared_ptr<Action>> parents;
        std::set<std::shared_ptr<Action>> children;

        void updateReadiness();

    private:
        std::string name;
        std::shared_ptr<CompoundJob> job;
        Action::State state;

        double start_date;
        double end_date;
        std::shared_ptr<FailureCause> failure_cause;


        static unsigned long getNewUniqueNumber();

    };
}


#endif //WRENCH_ACTION_H
