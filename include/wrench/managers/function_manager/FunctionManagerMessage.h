/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FUNCTIONMANAGERMESSAGE_H
#define WRENCH_FUNCTIONMANAGERMESSAGE_H

#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/compute/serverless/ServerlessComputeService.h"
#include "wrench/function/Function.h"
#include "wrench-dev.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a FunctionManager
     */
    class FunctionManagerMessage : public SimulationMessage {
    protected:
        explicit FunctionManagerMessage();
    };

    /**
     * @brief Message sent to the function manager to wake it up
     */
    class FunctionManagerWakeupMessage : public FunctionManagerMessage {
    public:
        FunctionManagerWakeupMessage();
    };

    class FunctionManagerFunctionInvocationRequestMessage : public FunctionManagerMessage {

    };

    class FunctionManagerFunctionInvocationAnswerMessage : public FunctionManagerMessage {
        
    };

    /**
     * @brief A message sent by the function manager when one waited-upon invocation completes
     * 
     */
    class FunctionManagerWaitOneMessage : public FunctionManagerMessage {
    public:
        FunctionManagerWaitOneMessage(S4U_CommPort *answer_commport, 
                                      std::shared_ptr<Invocation> invocation);

	/** @brief The commport to notify */
        S4U_CommPort *answer_commport;
	/** @brief The invocation */
        std::shared_ptr<Invocation> invocation;
    };

    /**
     * @brief A message sent by the function manager when multiple waited-upon invocation completes
     * 
     */
    class FunctionManagerWaitAllMessage : public FunctionManagerMessage {
    public:
        FunctionManagerWaitAllMessage(S4U_CommPort *answer_commport,
                                      std::vector<std::shared_ptr<Invocation>> invocations);

	/** @brief The commport to notify */
        S4U_CommPort *answer_commport;
	/** @brief The invocations */
        std::vector<std::shared_ptr<Invocation>> invocations;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#endif//WRENCH_FUNCTIONMANAGERMESSAGE_H
