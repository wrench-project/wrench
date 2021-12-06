/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SLEEP_ACTION_H
#define WRENCH_SLEEP_ACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {

    /**
     * @brief A class that implements a sleep action
     */
    class SleepAction : public Action {

    public:
        double getSleepTime();

    protected:
        friend class CompoundJob;

        SleepAction(const std::string& name, std::shared_ptr<CompoundJob> job, double sleep_time);

        void execute(std::shared_ptr<ActionExecutor> action_executor) override;
        void terminate(std::shared_ptr<ActionExecutor> action_executor) override;


    private:
        double sleep_time;

    };
}

#endif //WRENCH_SLEEP_ACTION_H
