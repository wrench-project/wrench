/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <string>
#include <boost/algorithm/string/split.hpp>
#include <utility>

#include "wrench/exceptions/ExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/managers/function_manager/FunctionManager.h"

#include <wrench/services/compute/serverless/ServerlessComputeServiceMessage.h>

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/helper_services/action_executor/ActionExecutor.h"
#include "wrench/services/helper_services/action_execution_service/ActionExecutionService.h"
#include "wrench/managers/function_manager/FunctionManagerMessage.h"

WRENCH_LOG_CATEGORY(wrench_core_function_manager, "Log category for Function Manager");

namespace wrench {
    /**
     * @brief Constructor
     *
     * @param hostname: the name of host on which the job manager will run
     * @param creator_commport: the commport of the manager's creator
     */
    FunctionManager::FunctionManager(const std::string& hostname, S4U_CommPort* creator_commport) : Service(
        hostname, "function_manager") {
        this->creator_commport = creator_commport;
    }

    void FunctionManager::stop() {
        // Implementation of stop logic, e.g., cleanup resources or notify shutdown
        this->Service::stop();
    }

    /**
     * @brief Kill the function manager
     */
    void FunctionManager::kill() {
        this->killActor();
        _registered_functions.clear();
        while (!_functions_to_invoke.empty()) {
            _functions_to_invoke.pop();
        }
        _pending_invocations.clear();
        _finished_invocations.clear();
        _invocations_being_waited_for.clear();
    }

    /**
     * @brief Destructor
     */
    FunctionManager::~FunctionManager() = default;

    /**
     * @brief Creates a shared pointer to a Function object and returns it
     *
     * @param name the name of the function
     * @param lambda the code of the function
     * @param image the location of image to execute the function on
     * @return std::shared_ptr<Function> a shared pointer to the Function object created
     */
    std::shared_ptr<Function> FunctionManager::createFunction(const std::string& name,
                                                              const std::function<std::shared_ptr<FunctionOutput>(
                                                              const std::shared_ptr<FunctionInput>&,
                                                              const std::shared_ptr<StorageService>&)>& lambda,
                                                              const std::shared_ptr<FileLocation>& image) {
        // Create the notion of a function
        return std::make_shared<Function>(name, lambda, image);
    }

    /**
     * @brief Registers a function with the ServerlessComputeService
     *
     * @param function the function to register
     * @param sl_compute_service the ServerlessComputeService to register the function on
     * @param time_limit_in_seconds the time limit for the function execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param RAM_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit (this is currently completely IGNORED)
     * @param egress_in_bytes the egress data limit (this is currently completely IGNORED)
     * @return true if the function was registered successfully
     * @throw ExecutionException if the function registration fails
     */
    std::shared_ptr<RegisteredFunction> FunctionManager::registerFunction(const std::shared_ptr<Function>& function,
                                                                          const std::shared_ptr<
                                                                              ServerlessComputeService>&
                                                                          sl_compute_service,
                                                                          double time_limit_in_seconds,
                                                                          sg_size_t disk_space_limit_in_bytes,
                                                                          sg_size_t RAM_limit_in_bytes,
                                                                          sg_size_t ingress_in_bytes,
                                                                          sg_size_t egress_in_bytes) {
        WRENCH_INFO("Function [%s] registered with compute service [%s]", function->getName().c_str(),
                    sl_compute_service->getName().c_str());
        // Logic to register the function with the serverless compute service
        return sl_compute_service->registerFunction(function, time_limit_in_seconds, disk_space_limit_in_bytes,
                                                    RAM_limit_in_bytes, ingress_in_bytes, egress_in_bytes);
    }

    /**
     * @brief Invokes a function on a ServerlessComputeService
     *
     * @param registered_function the (registered) function to invoke
     * @param sl_compute_service the ServerlessComputeService to invoke the function on
     * @param function_input the input (object) to the function
     * @return std::shared_ptr<Invocation> an Invocation object created by the ServerlessComputeService
     */
    std::shared_ptr<Invocation> FunctionManager::invokeFunction(
        const std::shared_ptr<RegisteredFunction>& registered_function,
        const std::shared_ptr<ServerlessComputeService>& sl_compute_service,
        const std::shared_ptr<FunctionInput>& function_input) {
        // WRENCH_INFO("Function [%s] invoked with compute service [%s]", registered_function->getFunction()->getName().c_str(), sl_compute_service->getName().c_str());
        // Pass in the function manager's commport as the commport to notify

        return sl_compute_service->invokeFunction(registered_function, function_input, this->commport);
    }

    /**
     * @brief State finding method to check if an invocation is done
     *
     * @param invocation the invocation to check
     * @return true if the invocation is done
     * @return false if the invocation is not done
     */
    bool FunctionManager::isDone(const std::shared_ptr<Invocation>& invocation) {
        if (_finished_invocations.find(invocation) != _finished_invocations.end()) {
            return true;
        }
        return false;
    }

    /**
     * @brief Waits for a single invocation to finish
     *
     * @param invocation the invocation to wait for
     */
    void FunctionManager::wait_one(const std::shared_ptr<Invocation>& invocation) {
        // WRENCH_INFO("FunctionManager::wait_one(): Waiting for invocation to finish");
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        // send a "wait one" message to the FunctionManager's commport
        this->commport->putMessage(
            new FunctionManagerWaitOneMessage(
                answer_commport,
                invocation
            ));

        // unblock up the EC with a wakeup message
        auto msg = answer_commport->getMessage<FunctionManagerWakeupMessage>(
            // this->network_timeout, // commented out for unlimited timeout time
            "FunctionManager::wait_one(): Received an");

        // WRENCH_INFO("FunctionManager::wait_one(): Received a wakeup message");
    }

    /**
     * @brief Waits for a list of invocations to finish
     *
     */
    void FunctionManager::wait_all(const std::vector<std::shared_ptr<Invocation>>& invocations) {
        // WRENCH_INFO("FunctionManager::wait_all(): Waiting for list of invocations to finish");
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        // send a "wait one" message to the FunctionManager's commport
        this->commport->putMessage(
            new FunctionManagerWaitAllMessage(
                answer_commport,
                invocations
            ));

        // unblock the EC with a wakeup message
        auto msg = answer_commport->getMessage<FunctionManagerWakeupMessage>(
            // this->network_timeout, // commented out for unlimited timeout time
            "FunctionManager::wait_one(): Received an");

        // WRENCH_INFO("FunctionManager::wait_all(): Received a wakeup message");
    }

    /**
     * @brief Main method of the daemon that implements the FunctionManager
     * @return 0 on success
     */
    int FunctionManager::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);
        WRENCH_INFO("New Function Manager starting (%s)", this->commport->get_cname());

        while (processNextMessage()) {
            // TODO: Do something
            processInvocationsBeingWaitedFor();
        }

        return 0;
    }

    /**
     * @brief Processes the next message in the commport
     *
     * @return true when the FunctionManager daemon should continue processing messages
     * @return false when the FunctionManager daemon should die
     */
    bool FunctionManager::processNextMessage() {
        S4U_Simulation::computeZeroFlop();

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        }
        catch (ExecutionException& e) {
            WRENCH_INFO(
                "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        //        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (std::dynamic_pointer_cast<FunctionManagerWakeupMessage>(message)) {
            // wake up!!
            return true;
        }
        else if (std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            return false;
        }
        else if (auto scsfic_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionInvocationCompleteMessage>(message)) {
            processFunctionInvocationComplete(scsfic_msg->invocation, scsfic_msg->success, scsfic_msg->failure_cause);
            return true;
        }
        else if (auto fmfc_msg = std::dynamic_pointer_cast<FunctionManagerFunctionCompletedMessage>(message)) {
            // Do nothing for now
            return true;
        }
        else if (auto wait_one_msg = std::dynamic_pointer_cast<FunctionManagerWaitOneMessage>(message)) {
            processWaitOne(wait_one_msg->invocation, wait_one_msg->answer_commport);
            return true;
        }
        else if (auto wait_many_msg = std::dynamic_pointer_cast<FunctionManagerWaitAllMessage>(message)) {
            processWaitAll(wait_many_msg->invocations, wait_many_msg->answer_commport);
            return true;
        }
        else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Method to process a function invocation completion
     * @param invocation the invocation
     * @param success whether the invocation has succeeded or not
     * @param failure_cause the failure cause (if failure)
     *
     */
    void FunctionManager::processFunctionInvocationComplete(const std::shared_ptr<Invocation>& invocation,
                                                            bool success,
                                                            const std::shared_ptr<FailureCause>& failure_cause) {
        invocation->_done = true;
        invocation->_success = success;
        invocation->_failure_cause = failure_cause;
        // _pending_invocations.erase(invocation);
        _finished_invocations.insert(invocation);
    }

    /**
     * @brief Processes a "wait one" message
     *
     * @param invocation the invocation being waited for
     * @param answer_commport the answer commport to send the wakeup message to when the invocation is finished
     */
    void FunctionManager::processWaitOne(const std::shared_ptr<Invocation>& invocation, S4U_CommPort* answer_commport) {
        // WRENCH_INFO("Processing a wait_one message");
        _invocations_being_waited_for.emplace_back(invocation, answer_commport);
    }

    /**
     * @brief Processes a "wait many" message
     *
     * @param invocations the invocations being waited for
     * @param answer_commport the answer commport to send the wakeup message to when the invocations are finished
     */
    void FunctionManager::processWaitAll(const std::vector<std::shared_ptr<Invocation>>& invocations,
                                         S4U_CommPort* answer_commport) {
        // WRENCH_INFO("Processing a wait_many message");
        for (const auto& invocation : invocations) {
            _invocations_being_waited_for.emplace_back(invocation, answer_commport);
        }
    }

    /**
     * @brief Iterates through the list of invocations being waited for and checks if they are finished
     *
     * TODO: There has to be a better way to do this than storing the answer commport in every single invocation LOL
     */
    void FunctionManager::processInvocationsBeingWaitedFor() {
        // WRENCH_INFO("Processing invocations being waited for");
        // iterate through the list of invocations being waited for
        if (_invocations_being_waited_for.empty()) {
            return;
        }
        auto it = _invocations_being_waited_for.begin();
        while (it != _invocations_being_waited_for.end()) {
            // check if the invocation is finished
            if (_finished_invocations.find(it->first) != _finished_invocations.end()) {
                // if there's only 1 invocation being waited for remaining, send a wakeup message
                if (_invocations_being_waited_for.size() <= 1) {
                    it->second->putMessage(new FunctionManagerWakeupMessage());
                }
                it = _invocations_being_waited_for.erase(it);
            }
            else {
                ++it;
            }
        }
    }
} // namespace wrench
