/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_CUSTOM_ACTION_H
#define WRENCH_CUSTOM_ACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {

    class CustomAction : public Action {

    public:

    protected:
        friend class CompoundJob;

        CustomAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                    const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_execute,
                    const std::function<void (std::shared_ptr<ActionExecutor> action_executor)> &lambda_terminate);

        void execute(std::shared_ptr<ActionExecutor> action_executor) override;
        void terminate(std::shared_ptr<ActionExecutor> action_executor) override;

    private:
        std::function<void (std::shared_ptr<ActionExecutor> action_executor)> lambda_execute;
        std::function<void (std::shared_ptr<ActionExecutor> action_executor)> lambda_terminate;
    };
}

#endif //WRENCH_CUSTOM_ACTION_H
