/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_ACTION_ACTION_EXECUTION_SERVICE_MESSAGE_H
#define WRENCH_ACTION_ACTION_EXECUTION_SERVICE_MESSAGE_H

#include <vector>

#include "wrench/services/helper_services/action_execution_service/ActionExecutionService.h"
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
    class ActionExecutionServiceMessage : public SimulationMessage {
    protected:
        ActionExecutionServiceMessage(sg_size_t payload);
    };

    /**
     * @brief A message sent to an ActionExecutionService to submit an Action
     */
    class ActionExecutionServiceSubmitActionRequestMessage : public ActionExecutionServiceMessage {
    public:
        ActionExecutionServiceSubmitActionRequestMessage(
                S4U_CommPort *reply_commport,
                std::shared_ptr<Action> action,
                sg_size_t payload);

        /** @brief The action to be executed */
        std::shared_ptr<Action> action;
        /** @brief The reply commport_name */
        S4U_CommPort *reply_commport;
    };

    /**
     * @brief A message sent by an ActionExecutionService in answer to an Action submission
     */
    class ActionExecutionServiceSubmitActionAnswerMessage : public ActionExecutionServiceMessage {
    public:
        ActionExecutionServiceSubmitActionAnswerMessage(
                bool success,
                std::shared_ptr<FailureCause> cause,
                sg_size_t payload);

        /** @brief Whether the action submission was a success or not */
        bool success;
        /** @brief The failure cause, if any */
        std::shared_ptr<FailureCause> cause;
    };

    /**
     * @brief A message sent to an ActionExecutionService to terminate an Action
     */
    class ActionExecutionServiceTerminateActionRequestMessage : public ActionExecutionServiceMessage {
    public:
        ActionExecutionServiceTerminateActionRequestMessage(
                S4U_CommPort *reply_commport,
                std::shared_ptr<Action> action,
                ComputeService::TerminationCause termination_cause,
                sg_size_t payload);

        /** @brief The reply commport_name */
        S4U_CommPort *reply_commport;
        /** @brief The action to terminate  */
        std::shared_ptr<Action> action;
        /** @brief The termination cause */
        ComputeService::TerminationCause termination_cause;
    };

    /**
   * @brief A message sent by an ActionExecutionService in response to an action termination
   */
    class ActionExecutionServiceTerminateActionAnswerMessage : public ActionExecutionServiceMessage {
    public:
        ActionExecutionServiceTerminateActionAnswerMessage(
                bool success,
                std::shared_ptr<FailureCause> cause,
                sg_size_t payload);

        /** @brief The success status */
        bool success;
        /** @brief The failure cause, if any  */
        std::shared_ptr<FailureCause> cause;
    };

    /**
 * @brief A message sent by an ActionExecutionService to notify of an action's completion
 */
    class ActionExecutionServiceActionDoneMessage : public ActionExecutionServiceMessage {
    public:
        ActionExecutionServiceActionDoneMessage(
                std::shared_ptr<Action> action,
                sg_size_t payload);

        /** @brief The action that completed  */
        std::shared_ptr<Action> action;
    };


    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench

#endif//WRENCH_ACTION_ACTION_EXECUTION_SERVICE_MESSAGE_H
