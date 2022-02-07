/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_ACTIONEXECUTORMESSAGE_H
#define WRENCH_ACTIONEXECUTORMESSAGE_H


#include "wrench/simulation/SimulationMessage.h"
#include "wrench-dev.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class ActionExecutor;

    /**
     * @brief Top-level class for messages received/sent by an ActionExecutor
     */
    class ActionExecutorMessage : public SimulationMessage {
    protected:
        explicit ActionExecutorMessage();
    };

    /**
     * @brief A message sent by an ActionExecutor when it's successfully completed an action
     */
    class ActionExecutorDoneMessage : public ActionExecutorMessage {
    public:
        explicit ActionExecutorDoneMessage(std::shared_ptr<ActionExecutor> action_executor);
        /** @brief The Action Executor */
        std::shared_ptr<ActionExecutor> action_executor;
    };

    /***********************/
    /** \endcond           */
    /***********************/

};

#endif //WRENCH_ACTIONEXECUTORMESSAGE_H
