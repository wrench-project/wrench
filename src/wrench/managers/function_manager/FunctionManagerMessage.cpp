/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/function_manager/FunctionManagerMessage.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     */
    FunctionManagerMessage::FunctionManagerMessage() : SimulationMessage(0) {
    }

    /**
     * @brief Message sent to the job manager to wake it up
     */
    FunctionManagerWakeupMessage::FunctionManagerWakeupMessage() : FunctionManagerMessage() {
    }

    /**
     * @brief Constructor
     * 
     * @param answer_commport the commport to notify
     * @param invocation the invocation that finished
     */
    FunctionManagerWaitOneMessage::FunctionManagerWaitOneMessage(S4U_CommPort *answer_commport,
                                                                 std::shared_ptr<Invocation> invocation) 
                                                                 : FunctionManagerMessage() {
        this->answer_commport = answer_commport;
        this->invocation = std::move(invocation);
    }

    /**
     * @brief Constructor
     * @param answer_commport he commport to notify
     * @param invocations the invocations that finished
     */
    FunctionManagerWaitAllMessage::FunctionManagerWaitAllMessage(S4U_CommPort *answer_commport,
                                                                 std::vector<std::shared_ptr<Invocation>> invocations) 
                                                                 : FunctionManagerMessage() {
        this->answer_commport = answer_commport;
        this->invocations = std::move(invocations);
    }

}// namespace wrench
