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
    * @param payload: the message size in bytes
    */
    ActionExecutionServiceMessage::ActionExecutionServiceMessage(sg_size_t payload) : SimulationMessage(payload) {
    }


    /**
     * @brief Constructor
     * @param reply_commport: the reply commport
     * @param action: the action to perform
     * @param payload: the message size in bytes
     */
    ActionExecutionServiceSubmitActionRequestMessage::ActionExecutionServiceSubmitActionRequestMessage(
            S4U_CommPort *reply_commport,
            std::shared_ptr<Action> action,
            sg_size_t payload) : ActionExecutionServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((reply_commport == nullptr) || (action == nullptr)) {
            throw std::invalid_argument("ActionExecutionServiceSubmitActionRequestMessage::ActionExecutionServiceSubmitActionRequestMessage(): invalid argument");
        }
#endif
        this->action = std::move(action);
        this->reply_commport = reply_commport;
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
            sg_size_t payload) : ActionExecutionServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
#endif
        this->success = success;
        this->cause = std::move(cause);
    }

    /**
     * @brief Constructor
     * @param reply_commport: the reply commport
     * @param action: the action to terminate
     * @param termination_cause: the termination cause
     * @param payload: the message size in bytes
     */
    ActionExecutionServiceTerminateActionRequestMessage::ActionExecutionServiceTerminateActionRequestMessage(
            S4U_CommPort *reply_commport,
            std::shared_ptr<Action> action,
            ComputeService::TerminationCause termination_cause,
            sg_size_t payload) : ActionExecutionServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((reply_commport == nullptr) || (action == nullptr)) {
            throw std::invalid_argument("ActionExecutionServiceTerminateActionRequestMessage::ActionExecutionServiceTerminateActionRequestMessage(): invalid argument");
        }
#endif
        this->reply_commport = reply_commport;
        this->action = std::move(action);
        this->termination_cause = termination_cause;
    }

    /**
     * @brief Constructor
     * @param success: the success status
     * @param cause: the failure cause, if any
     * @param payload: the message size in bytes
     */
    ActionExecutionServiceTerminateActionAnswerMessage::ActionExecutionServiceTerminateActionAnswerMessage(
            bool success,
            std::shared_ptr<FailureCause> cause, sg_size_t payload) : ActionExecutionServiceMessage(payload) {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
#endif
        this->success = success;
        this->cause = std::move(cause);
    }

    /**
  * @brief Constructor
  * @param action: the action that completed
  * @param payload: the message size in bytes
  */
    ActionExecutionServiceActionDoneMessage::ActionExecutionServiceActionDoneMessage(
            std::shared_ptr<Action> action, sg_size_t payload) : ActionExecutionServiceMessage(payload) {

#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if (action == nullptr) {
            throw std::invalid_argument("ActionExecutionServiceActionDoneMessage::ActionExecutionServiceActionDoneMessage(): invalid argument");
        }
#endif
        this->action = std::move(action);
    }

}// namespace wrench
