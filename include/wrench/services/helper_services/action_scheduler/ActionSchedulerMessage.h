/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_ACTION_SCHEDULER_MESSAGE_H
#define WRENCH_ACTION_SCHEDULER_MESSAGE_H

#include <vector>

#include "wrench/services/helper_services/action_scheduler/ActionExecutionService.h"
#include "wrench/services/ServiceMessage.h"

namespace wrench {

    class Action;

    class ActionExecutor;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a ActionExecutionService
     */
    class ActionSchedulerMessage : public SimulationMessage {
    protected:
        ActionSchedulerMessage(std::string name, double payload);
    };

    /**
     * @brief A message sent to an ActionExecutionService to submit an Action
     */
    class ActionSchedulerSubmitActionRequestMessage : public ActionSchedulerMessage {
    public:
        ActionSchedulerSubmitActionRequestMessage(
                const std::string &reply_mailbox,
                std::shared_ptr<Action> action,
                double payload);

        /** @brief The action to be executed */
        std::shared_ptr<Action> action;
        /** @brief The reply mailbox */
        std::string reply_mailbox;
    };

    /**
     * @brief A message sent by an ActionExecutionService in answer to an Action submission
     */
    class ActionSchedulerSubmitActionAnswerMessage : public ActionSchedulerMessage {
    public:
        ActionSchedulerSubmitActionAnswerMessage(
                bool success,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief Whether the action submission was a success or not */
        bool success;
        /** @brief The failure cause, if any */
        std::shared_ptr<FailureCause> cause;
    };

    /**
     * @brief A message sent to an ActionExecutionService to terminate an Action
     */
    class ActionSchedulerTerminateActionRequestMessage : public ActionSchedulerMessage {
    public:
        ActionSchedulerTerminateActionRequestMessage(
                const std::string &reply_mailbox,
                std::shared_ptr<Action> action,
                double payload);

        /** @brief The reply mailbox */
        std::string reply_mailbox;
        /** @brief The action to terminate  */
        std::shared_ptr<Action> action;
    };

    /**
   * @brief A message sent by an ActionExecutionService in response to an action termination
   */
    class ActionSchedulerTerminateActionAnswerMessage : public ActionSchedulerMessage {
    public:
        ActionSchedulerTerminateActionAnswerMessage(
                bool success,
                std::shared_ptr<FailureCause> cause,
                double payload);

        /** @brief The success status */
        bool success;
        /** @brief The failure cause, if any  */
        std::shared_ptr<FailureCause> cause;
    };

    /**
     * @brief A message sent by an ActionExecutionService to notify of an action's failure
     */
    class ActionSchedulerActionFailedMessage : public ActionSchedulerMessage {
    public:
        ActionSchedulerActionFailedMessage(
                std::shared_ptr<Action> action,
                double payload);

        /** @brief The action that failed  */
        std::shared_ptr<Action> action;
    };

    /**
 * @brief A message sent by an ActionExecutionService to notify of an action's completion
 */
    class ActionSchedulerActionDoneMessage : public ActionSchedulerMessage {
    public:
        ActionSchedulerActionDoneMessage(
                std::shared_ptr<Action> action,
                double payload);

        /** @brief The action that completed  */
        std::shared_ptr<Action> action;
    };



    /***********************/
    /** \endcond           */
    /***********************/
};

#endif //WRENCH_ACTION_SCHEDULER_MESSAGE_H
