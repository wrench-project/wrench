/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ACTION_EXECUTOR_H
#define WRENCH_ACTION_EXECUTOR_H

#include <set>

#include "wrench/services/Service.h"
#include "wrench/action/Action.h"

namespace wrench {

    class Simulation;
    class Action;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An service that performs an Action
     */
    class ActionExecutor : public Service {

    public:

        ActionExecutor(
                     std::string hostname,
                     std::string callback_mailbox,
                     std::shared_ptr<Action> action);

        virtual int main() = 0;
        virtual void kill(bool job_termination) = 0;
        void cleanup(bool has_returned_from_main, int return_value) = 0;

        std::shared_ptr<Action> getAction();

    protected:
        std::shared_ptr<Action> action;
        std::string callback_mailbox;
        bool killed_on_purpose;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_ACTION_EXECUTOR_H