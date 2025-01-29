/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FUNCTIONMANAGERMESSAGE_H
#define WRENCH_FUNCTIONMANAGERMESSAGE_H

#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/compute/ServerlessComputeService.h"
#include "wrench/job/Function.h"
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
     * @brief A message sent by the FunctionManager to notify some submitter that a Function has completed
     */
    class FunctionManagerFunctionCompletedMessage : public FunctionManagerMessage {
    public:
        FunctionManagerFunctionCompletedMessage(std::shared_ptr<Function> function, std::shared_ptr<ServerlessComputeService> sl_compute_service);

        /** @brief The function that is invoked */
        std::shared_ptr<Function> function;
        /** @brief The ServerlessComputeService on which the function ran */
        std::shared_ptr<ServerlessComputeService> sl_compute_service;
    };

    /**
     * @brief A message sent by the FunctionManager to notify some submitter that a Function has failed
     */
    class FunctionManagerFunctionFailedMessage : public FunctionManagerMessage {
    public:
        FunctionManagerFunctionFailedMessage(std::shared_ptr<Function> function,
                                             std::shared_ptr<ServerlessComputeService> sl_compute_service,
                                             std::shared_ptr<FailureCause> cause);

        /** @brief The function that is invoked */
        std::shared_ptr<Function> function;
        /** @brief The ServerlessComputeService on which the function ran */
        std::shared_ptr<ServerlessComputeService> sl_compute_service;
        /** @brief The cause of the failure */
        std::shared_ptr<FailureCause> cause;
    };

    /**
     * @brief Message sent to the function manager to wake it up
     */
    class FunctionManagerWakeupMessage : public FunctionManagerMessage {
    public:
        FunctionManagerWakeupMessage();
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#endif//WRENCH_FUNCTIONMANAGERMESSAGE_H
