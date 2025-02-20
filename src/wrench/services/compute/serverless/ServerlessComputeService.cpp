/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/services/compute/serverless/ServerlessComputeServiceMessage.h>
#include <wrench/services/compute/serverless/ServerlessComputeServiceMessagePayload.h>
#include <wrench/services/compute/serverless/Invocation.h>
#include <wrench/managers/function_manager/Function.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/failure_causes/FunctionNotFound.h>

#include "wrench/services/ServiceMessage.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");

namespace wrench
{
    ServerlessComputeService::ServerlessComputeService(const std::string& hostname,
                                                       std::vector<std::string> compute_hosts,
                                                       std::string scratch_space_mount_point,
                                                       WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                       WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) :
        ComputeService(hostname,
                       "ServerlessComputeService",
                       scratch_space_mount_point)
    {
        _compute_hosts = compute_hosts;
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsStandardJobs()
    {
        return false;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsCompoundJobs()
    {
        return false;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsPilotJobs()
    {
        return false;
    }

    /**
     * @brief Method to submit a compound job to the service
     *
     * @param job: The job being submitted
     * @param service_specific_args: the set of service-specific arguments
     */
    void ServerlessComputeService::submitCompoundJob(std::shared_ptr<CompoundJob> job,
                                                     const std::map<std::string, std::string>& service_specific_args)
    {
        throw std::runtime_error("ServerlessComputeService::submitCompoundJob: should not be called");
    }

    /**
     * @brief Method to terminate a compound job at the service
     *
     * @param job: The job being submitted
     */
    void ServerlessComputeService::terminateCompoundJob(std::shared_ptr<CompoundJob> job)
    {
        throw std::runtime_error("ServerlessComputeService::terminateCompoundJob: should not be called");
    }

    /**
     * @brief Construct a dict for resource information
     * @param key: the desired key
     * @return a dictionary
     */
    std::map<std::string, double> ServerlessComputeService::constructResourceInformation(const std::string& key)
    {
        throw std::runtime_error("ServerlessComputeService::constructResourceInformation: not implemented");
    }

    /**
     * @brief 
     * 
     * @param function 
     * @param time_limit_in_seconds 
     * @param disk_space_limit_in_bytes 
     * @param RAM_limit_in_bytes 
     * @param ingress_in_bytes 
     * @param egress_in_bytes 
     * @return true 
     * @return false 
     */
    bool ServerlessComputeService::registerFunction(std::shared_ptr<Function> function, double time_limit_in_seconds,
                                                    sg_size_t disk_space_limit_in_bytes, sg_size_t RAM_limit_in_bytes,
                                                    sg_size_t ingress_in_bytes, sg_size_t egress_in_bytes)
    {
        WRENCH_INFO(("Serverless Provider Registered function " + function->getName()).c_str());
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
            new ServerlessComputeServiceFunctionRegisterRequestMessage(
                answer_commport, function, time_limit_in_seconds, disk_space_limit_in_bytes, RAM_limit_in_bytes, ingress_in_bytes, egress_in_bytes,
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionRegisterAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::registerFunction(): Received an");

        // TODO: Deal with failures later
        if (not msg->success) {
             throw ExecutionException(msg->failure_cause);
        }
        return true;
    }

    /**
     * @brief 
     * 
     * @param function 
     * @param input 
     * @param notify_commport 
     * @return std::shared_ptr<Invocation> 
     */
    std::shared_ptr<Invocation> ServerlessComputeService::invokeFunction(std::shared_ptr<Function> function, std::shared_ptr<FunctionInput> input, S4U_CommPort* notify_commport) {
        WRENCH_INFO(("Serverless Provider received invoke function " + function->getName()).c_str());
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();
        this->commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationRequestMessage(answer_commport,
                function, input,
                notify_commport, 0)
        );

        // Block here for return, if non blocking then function manager has to check up on it? or send a message
        auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionInvocationAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::invokeFunction(): Received an");

        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
        return msg->invocation;
    }

    int ServerlessComputeService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Serverless provider starting (%s)", this->commport->get_cname());

        while (processNextMessage()) {
            dispatchFunctionInvocation();
        }
        return 0;
    }

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool ServerlessComputeService::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        } catch (ExecutionException &e) {
            WRENCH_INFO(
                    "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        //        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto ss_mesg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // TODO: Die...
            return false;
        }
        else if (auto scsfrr_msg = std::dynamic_pointer_cast<ServerlessComputeServiceFunctionRegisterRequestMessage>(message)) {
            processFunctionRegistrationRequest(
                scsfrr_msg->answer_commport, scsfrr_msg->function, scsfrr_msg->time_limit_in_seconds, 
                scsfrr_msg->disk_space_limit_in_bytes, scsfrr_msg->ram_limit_in_bytes, 
                scsfrr_msg->ingress_in_bytes, scsfrr_msg->egress_in_bytes);
            return true;
        }
        else if (auto scsfir_msg = std::dynamic_pointer_cast<ServerlessComputeServiceFunctionInvocationRequestMessage>(message)) {
            processFunctionInvocationRequest(scsfir_msg->answer_commport, scsfir_msg->function, scsfir_msg->function_input, scsfir_msg->notify_commport);
            return true;
        }
         else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief 
     * 
     * @param answer_commport 
     * @param function 
     * @param time_limit 
     * @param disk_space_limit_in_bytes 
     * @param ram_limit_in_bytes 
     * @param ingress_in_bytes 
     * @param egress_in_bytes 
     */
    void ServerlessComputeService::processFunctionRegistrationRequest(S4U_CommPort *answer_commport, std::shared_ptr<Function> function, double time_limit, sg_size_t disk_space_limit_in_bytes, sg_size_t ram_limit_in_bytes, sg_size_t ingress_in_bytes, sg_size_t egress_in_bytes) {
        if (_registeredFunctions.find(function->getName()) != _registeredFunctions.end()) {
            // TODO: Create failure case for duplicate function?
            std::string msg = "Duplicate Function";
            auto answerMessage = 
                new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                    false, function,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(), msg), 0);
            answer_commport->dputMessage(answerMessage);
        } else {        
            _registeredFunctions[function->getName()] = std::make_shared<RegisteredFunction>(
                function, 
                time_limit, 
                disk_space_limit_in_bytes, 
                ram_limit_in_bytes, 
                ingress_in_bytes, 
                egress_in_bytes
            );
            auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(true, function, nullptr, 0);
            answer_commport->dputMessage(answerMessage);
        }
    }

    /**
     * @brief 
     * 
     * @param answer_commport 
     * @param function 
     * @param input 
     * @param notify_commport 
     */
    void ServerlessComputeService::processFunctionInvocationRequest(S4U_CommPort *answer_commport, std::shared_ptr<Function> function, std::shared_ptr<FunctionInput> input, S4U_CommPort *notify_commport) {
        if (_registeredFunctions.find(function->getName()) == _registeredFunctions.end()) { // Not found
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                false, nullptr, std::make_shared<FunctionNotFound>(function), 0);
            answer_commport->dputMessage(answerMessage);
        } else {
            const auto invocation = std::make_shared<Invocation>(_registeredFunctions.at(function->getName()), input, notify_commport);
            _newInvocations.push(invocation);
            // TODO: return some sort of function invocation object?
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(true, invocation, nullptr, 0);
            answer_commport->dputMessage(answerMessage);
        }

    }

    /**
     * @brief 
     * 
     */
    void ServerlessComputeService::dispatchFunctionInvocation() {
        while (!_newInvocations.empty()) {
            // Might need to think about how RegisteredFunction is storing info here...
            auto invocation_to_place = _newInvocations.front();
            WRENCH_INFO("Invoking function [%s]", invocation_to_place->_registered_function->_function->getName().c_str());
            _newInvocations.pop();
            // TODO: Do invocation for real
            // invocation_to_place->is_running = true
            Simulation::sleep(1);
            // invocation_to_place->output = whatever
            // invocation_to_place->_registered_function->_function->run_lambda()
            invocation_to_place->_notify_commport->dputMessage(new ServerlessComputeServiceFunctionInvocationCompleteMessage(true,
                                                                invocation_to_place, nullptr, 0 ));

        }
    }
};
