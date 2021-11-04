/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/services/helper_services/action_execution_service/ActionExecutionServiceMessage.h>

#include <utility>

namespace wrench {


    /**
    * @brief Constructor
    *
    * @param name: the message name
    * @param payload: the message size in bytes
    */
    ActionExecutionServiceMessage::ActionExecutionServiceMessage(std::string name, double payload) :
            SimulationMessage("ActionExecutionServiceMessage::" + name, payload) {
    }


    /**
     * @brief Constructor
     * @param action: the action to perform
     * @param payload: the message size in bytes
     */
    ActionExecutionServiceSubmitActionRequestMessage::ActionExecutionServiceSubmitActionRequestMessage(
            const std::string &reply_mailbox,
            std::shared_ptr<Action> action,
            double payload) :
            ActionExecutionServiceMessage("ACTION_EXECUTION_SERVICE_SUBMIT_ACTION_REQUEST", payload) {
        this->action = std::move(action);
        this->reply_mailbox = reply_mailbox;
    }

    /**
     * @brief Constructor
     * @param success: whether the action submission was a success
     * @param cause: the cause of the failure, if any
     * @param payload: the message size in bytes
     */
    ActionExecutionServiceSubmitActionAnswerMessage::ActionExecutionServiceSubmitActionAnswerMessage(
            bool success,
            std::shared_ptr<FailureCause> cause,
            double payload) :
            ActionExecutionServiceMessage("ACTION_EXECUTION_SERVICE_SUBMIT_ACTION_ANSWER", payload) {
        this->success = success;
        this->cause = std::move(cause);
    }

    /**
     * @brief Constructor
     * @param action: the action to terminate
     * @param payload: the message size in bytes
     */
    ActionExecutionServiceTerminateActionRequestMessage::ActionExecutionServiceTerminateActionRequestMessage(
            const std::string &reply_mailbox,
            std::shared_ptr<Action> action, double payload) :
            ActionExecutionServiceMessage("ACTION_EXECUTION_SERVICE_TERMINATE_ACTION_REQUEST", payload) {
        this->reply_mailbox = reply_mailbox;
        this->action = std::move(action);
    }

    /**
     * @brief Constructor
     * @param success: the success status
     * @param cause: the failure cause, if any
     * @param payload: the message size in bytes
     */
    ActionExecutionServiceTerminateActionAnswerMessage::ActionExecutionServiceTerminateActionAnswerMessage(
            bool success,
            std::shared_ptr<FailureCause> cause, double payload) :
            ActionExecutionServiceMessage("ACTION_EXECUTION_SERVICE_TERMINATE_ACTION_ANSWER", payload) {
        this->success = success;
        this->cause = std::move(cause);
    }

    /**
  * @brief Constructor
  * @param action: the action that completed
  * @param payload: the message size in bytes
  */
    ActionExecutionServiceActionDoneMessage::ActionExecutionServiceActionDoneMessage(
            std::shared_ptr<Action> action, double payload) :
            ActionExecutionServiceMessage("ACTION_EXECUTION_SERVICE_ACTION_DONE", payload){
        this->action = std::move(action);
        this->payload = payload;
    }

};
