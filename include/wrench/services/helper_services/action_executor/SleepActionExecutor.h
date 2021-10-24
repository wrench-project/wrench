/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SLEEP_ACTION_EXECUTOR_H
#define WRENCH_SLEEP_ACTION_EXECUTOR_H

#include <set>

#include "wrench/services/helper_services/action_executor/ActionExecutor.h"
#include "wrench/action/Action.h"

namespace wrench {

    class Simulation;
    class Action;
    class SleepAction;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief An service that performs a Sleep Action
     */
    class SleepActionExecutor : public ActionExecutor {

    public:

        SleepActionExecutor(
                     std::string hostname,
                     std::string callback_mailbox,
                     std::shared_ptr<SleepAction> action);

        void kill(bool job_termination) override;
        void cleanup(bool has_returned_from_main, int return_value) override;


    private:
        int main() override;

    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_SLEEP_ACTION_EXECUTOR_H
