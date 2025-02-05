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
     * @brief Constructor
     * 
     * @param answer_commport: commport to which the answer message should be sent
     * @param function: the function to register
     * @param time_limit: time limit for execution
     * @param disk_space_limit_in_bytes: disk space limit for the function
     * @param ram_limit_in_bytes: RAM limit for the function
     * @param ingress_in_bytes: ingress data limit
     * @param egress_in_bytes: egress data limit
     */
    FunctionManagerFunctionRegisterRequestMessage::FunctionManagerFunctionRegisterRequestMessage(
            S4U_CommPort *answer_commport,
            std::shared_ptr<Function> function,
            double time_limit,
            sg_size_t disk_space_limit_in_bytes,
            sg_size_t ram_limit_in_bytes,
            sg_size_t ingress_in_bytes,
            sg_size_t egress_in_bytes)
        : FunctionManagerMessage() {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (function == nullptr)) {
            throw std::invalid_argument(
                    "ServerlessComputeServiceFunctionRegisterRequestMessage::ServerlessComputeServiceFunctionRegisterRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->function = std::move(function);
        this->time_limit_in_seconds = time_limit;
        this->disk_space_limit_in_bytes = disk_space_limit_in_bytes;
        this->ram_limit_in_bytes = ram_limit_in_bytes;
        this->ingress_in_bytes = ingress_in_bytes;
        this->egress_in_bytes = egress_in_bytes;
    }

    }

    /**
     * @brief Constructor 
     * @param function: the function that is invoked
     * @param sl_compute_service: the ServerlessComputeService on which it ran 
     */
    FunctionManagerFunctionRegisterAnswerMessage::FunctionManagerFunctionRegisterAnswerMessage(std::shared_ptr<Function> function,
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

    FunctionManagerFunctionInvocationRequestMessage::FunctionManageFunctionInvocationRequestMessage() : FunctionManagerMessage() {

    }

    /**
     * @brief Constructor 
     * @param function: the function that is invoked
     * @param sl_compute_service: the ServerlessComputeService on which it ran 
     */
    FunctionManagerFunctionInvocationAnswerMessage::FunctionManagerFunctionInvocationAnswerMessage(std::shared_ptr<Function> function,
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
     * @brief Constructor 
     * @param function: the function that is invoked
     * @param sl_compute_service: the ServerlessComputeService on which it ran 
     * @param cause: the cause of the failure
     */
    FunctionManagerFunctionInvocationCompletedMessage::FunctionManagerFunctionInvocationCompletedMessage(std::shared_ptr<Function> function,
                                                                               std::shared_ptr<ServerlessComputeService> sl_compute_service,
                                                                               std::shared_ptr<FailureCause> cause) 
                                                                               : FunctionManagerMessage() {
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
