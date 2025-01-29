/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/job_manager/FunctionManagerMessage.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     */
    FunctionManagerMessage::FunctionManagerMessage() : SimulationMessage(0) {
    }

    /**
     * @brief Constructor 
     * @param function: the function that is invoked
     * @param sl_compute_service: the ServerlessComputeService on which it ran 
     */
    FunctionManagerFunctionCompletedMessage::FunctionManagerFunctionCompletedMessage(std::shared_ptr<Function> function,
                                                                                     std::shared_ptr<ServerlessComputeService> sl_compute_service
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
     * @brief Constructor 
     * @param function: the function that is invoked
     * @param sl_compute_service: the ServerlessComputeService on which it ran 
     * @param cause: the cause of the failure
     */
    FunctionManagerFunctionFailedMessage::FunctionManagerFunctionFailedMessage(std::shared_ptr<Function> function,
                                                                               std::shared_ptr<ServerlessComputeService> sl_compute_service,
                                                                               std::shared_ptr<FailureCause> cause) : FunctionManagerMessage() {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((function == nullptr) || (sl_compute_service == nullptr) || (cause == nullptr)) {
            throw std::invalid_argument("FunctionManagerFunctionFailedMessage::FunctionManagerFunctionFailedMessage(): Invalid arguments");
        }
#endif
        this->function = std::move(function);
        this->sl_compute_service = std::move(sl_compute_service);
        this->cause = std::move(cause);
    }

    /**
     * @brief Message sent to the job manager to wake it up
     */
    FunctionManagerWakeupMessage::FunctionManagerWakeupMessage() : FunctionManagerMessage() {
    }


}// namespace wrench
