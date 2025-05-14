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
     * @param function: the function that is invoked
     * @param sl_compute_service: the ServerlessComputeService on which it ran 
     */
    FunctionManagerFunctionCompletedMessage::FunctionManagerFunctionCompletedMessage(std::shared_ptr<Function> function,
                                                                                     std::shared_ptr<ServerlessComputeService> sl_compute_service)
                                                                                     : FunctionManagerMessage() {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((function == nullptr) || (sl_compute_service == nullptr)) {
            throw std::invalid_argument("FunctionManagerFunctionCompletedMessage::FunctionManagerFunctionCompletedMessage(): Invalid arguments");
        }
#endif
        this->function = std::move(function);
        this->sl_compute_service = std::move(sl_compute_service);
    }

    /**
     * @brief Construct a new Function Manager Wait One Message:: Function Manager Wait One Message object
     * 
     * @param answer_commport 
     * @param invocation 
     */
    FunctionManagerWaitOneMessage::FunctionManagerWaitOneMessage(S4U_CommPort *answer_commport,
                                                                 std::shared_ptr<Invocation> invocation) 
                                                                 : FunctionManagerMessage() {
        this->answer_commport = answer_commport;
        this->invocation = std::move(invocation);
    }

    FunctionManagerWaitAllMessage::FunctionManagerWaitAllMessage(S4U_CommPort *answer_commport,
                                                                 std::vector<std::shared_ptr<Invocation>> invocations) 
                                                                 : FunctionManagerMessage() {
        this->answer_commport = answer_commport;
        this->invocations = std::move(invocations);
    }

}// namespace wrench
