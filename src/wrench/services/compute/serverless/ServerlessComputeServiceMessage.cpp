/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/services/compute/serverless/ServerlessComputeServiceMessage.h"

#include <utility>

namespace wrench
{

    /**
     * @brief Constructor for ServerlessComputeServiceMessage
     * @param payload: message size in bytes
     */
    ServerlessComputeServiceMessage::ServerlessComputeServiceMessage(const sg_size_t payload)
        : ComputeServiceMessage(payload)
    {
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
     * @param payload: message size in bytes
     */
    ServerlessComputeServiceFunctionRegisterRequestMessage::ServerlessComputeServiceFunctionRegisterRequestMessage(
        S4U_CommPort *answer_commport,
        std::shared_ptr<Function> function,
        const double time_limit,
        const sg_size_t disk_space_limit_in_bytes,
        const sg_size_t ram_limit_in_bytes,
        const sg_size_t ingress_in_bytes,
        const sg_size_t egress_in_bytes,
        const sg_size_t payload)
        : ServerlessComputeServiceMessage(payload)
    {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (function == nullptr))
        {
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

    /**
     * @brief Constructor
     *
     * @param success: whether the registration was successful or not
     * @param registered_function: the registered function (or nullptr on failure)
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    ServerlessComputeServiceFunctionRegisterAnswerMessage::ServerlessComputeServiceFunctionRegisterAnswerMessage(
        bool success,
        std::shared_ptr<RegisteredFunction> registered_function,
        std::shared_ptr<FailureCause> failure_cause,
        sg_size_t payload)
        : ServerlessComputeServiceMessage(payload), success(success), registered_function(std::move(registered_function)), failure_cause(std::move(failure_cause)) {}

    /**
     * @brief Constructor
     *
     * @param answer_commport: commport to which the answer message should be sent
     * @param registered_function: the (registered) function to invoke
     * @param function_input: input arguments passed to the function
     * @param notify_commport: commport to notify
     * @param payload: message size in bytes
     */
    ServerlessComputeServiceFunctionInvocationRequestMessage::ServerlessComputeServiceFunctionInvocationRequestMessage(
        S4U_CommPort *answer_commport,
        const std::shared_ptr<RegisteredFunction>& registered_function,
        const std::shared_ptr<FunctionInput>& function_input,
        S4U_CommPort *notify_commport,
        sg_size_t payload)
        : ServerlessComputeServiceMessage(payload)
    {
#ifdef WRENCH_INTERNAL_EXCEPTIONS
        if ((answer_commport == nullptr) || (function == nullptr))
        {
            throw std::invalid_argument(
                "ServerlessComputeServiceFunctionRegisterRequestMessage::ServerlessComputeServiceFunctionRegisterRequestMessage(): Invalid arguments");
        }
#endif
        this->answer_commport = answer_commport;
        this->registered_function = registered_function;
        this->function_input = function_input;
        this->notify_commport = notify_commport;
    }

    /**
     * @brief Constructor
     *
     * @param success: whether the invocation was successful or not
     * @param invocation: the invocation object
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    ServerlessComputeServiceFunctionInvocationAnswerMessage::ServerlessComputeServiceFunctionInvocationAnswerMessage(
        bool success,
        std::shared_ptr<Invocation> invocation,
        std::shared_ptr<FailureCause> failure_cause,
        sg_size_t payload)
        : ServerlessComputeServiceMessage(payload), success(success), invocation(std::move(invocation)), failure_cause(std::move(failure_cause)) {}

    /**
     * @brief Constructor
     *
     * @param success: whether the invocation was successful or not
     * @param invocation: the invocation object
     * @param failure_cause: a failure cause (or nullptr if success)
     * @param payload: the message size in bytes
     */
    ServerlessComputeServiceFunctionInvocationCompleteMessage::ServerlessComputeServiceFunctionInvocationCompleteMessage(
        bool success,
        std::shared_ptr<Invocation> invocation,
        std::shared_ptr<FailureCause> failure_cause,
        sg_size_t payload)
        : ServerlessComputeServiceMessage(payload), success(success), invocation(std::move(invocation)), failure_cause(std::move(failure_cause)) {}

    ServerlessComputeServiceDownloadCompleteMessage::ServerlessComputeServiceDownloadCompleteMessage(
        std::shared_ptr<CustomAction> action, std::shared_ptr<DataFile> image_file, sg_size_t payload) : ServerlessComputeServiceMessage(payload), _action(std::move(action)), _image_file(std::move(image_file)) {}

    ServerlessComputeServiceInvocationExecutionCompleteMessage::ServerlessComputeServiceInvocationExecutionCompleteMessage(
        std::shared_ptr<CustomAction> action, std::shared_ptr<Invocation> invocation, sg_size_t payload) : ServerlessComputeServiceMessage(payload), _action(std::move(action)), _invocation(std::move(invocation)) {}

    ServerlessComputeServiceNodeCopyCompleteMessage::ServerlessComputeServiceNodeCopyCompleteMessage(
        std::shared_ptr<CustomAction> action, std::shared_ptr<DataFile> image_file, std::string compute_host, sg_size_t payload) : ServerlessComputeServiceMessage(payload), _action(std::move(action)), _image_file(std::move(image_file)), _compute_host(std::move(compute_host)) {}

    ServerlessComputeServiceNodeLoadCompleteMessage::ServerlessComputeServiceNodeLoadCompleteMessage(
        std::shared_ptr<CustomAction> action, std::shared_ptr<DataFile> image_file, std::string compute_host, sg_size_t payload) : ServerlessComputeServiceMessage(payload), _action(std::move(action)), _image_file(std::move(image_file)), _compute_host(std::move(compute_host)) {}

} // namespace wrench
