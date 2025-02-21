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

#include <utility>

#include "wrench/action/CustomAction.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/helper_services/action_executor/ActionExecutor.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");

namespace wrench {
    ServerlessComputeService::ServerlessComputeService(const std::string& hostname,
                                                       std::vector<std::string> compute_hosts,
                                                       std::string head_storage_service_mount_point,
                                                       WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                       WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) :
        ComputeService(hostname,
                       "ServerlessComputeService", "") {
        _compute_hosts = std::move(compute_hosts);
        _head_storage_service_mount_point = std::move(head_storage_service_mount_point);
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);
    }

    /**
     * @brief Returns true if the service supports standard jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsStandardJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports compound jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsCompoundJobs() {
        return false;
    }

    /**
     * @brief Returns true if the service supports pilot jobs
     * @return true or false
     */
    bool ServerlessComputeService::supportsPilotJobs() {
        return false;
    }

    /**
     * @brief Method to submit a compound job to the service
     *
     * @param job: The job being submitted
     * @param service_specific_args: the set of service-specific arguments
     */
    void ServerlessComputeService::submitCompoundJob(std::shared_ptr<CompoundJob> job,
                                                     const std::map<std::string, std::string>& service_specific_args) {
        throw std::runtime_error("ServerlessComputeService::submitCompoundJob: should not be called");
    }

    /**
     * @brief Method to terminate a compound job at the service
     *
     * @param job: The job being submitted
     */
    void ServerlessComputeService::terminateCompoundJob(std::shared_ptr<CompoundJob> job) {
        throw std::runtime_error("ServerlessComputeService::terminateCompoundJob: should not be called");
    }

    /**
     * @brief Construct a dict for resource information
     * @param key: the desired key
     * @return a dictionary
     */
    std::map<std::string, double> ServerlessComputeService::constructResourceInformation(const std::string& key) {
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
                                                    sg_size_t ingress_in_bytes, sg_size_t egress_in_bytes) {
        WRENCH_INFO(("Serverless Provider Registered function " + function->getName()).c_str());
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
            new ServerlessComputeServiceFunctionRegisterRequestMessage(
                answer_commport, function, time_limit_in_seconds, disk_space_limit_in_bytes, RAM_limit_in_bytes,
                ingress_in_bytes, egress_in_bytes,
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionRegisterAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::registerFunction(): Received an");

        // TODO: Deal with failures later
        if (not msg->success)
        {
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
    std::shared_ptr<Invocation> ServerlessComputeService::invokeFunction(
        std::shared_ptr<Function> function, std::shared_ptr<FunctionInput> input, S4U_CommPort* notify_commport) {
        WRENCH_INFO(("Serverless Provider received invoke function %s", function->getName()).c_str());
        auto answer_commport = S4U_CommPort::getTemporaryCommPort();
        this->commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationRequestMessage(answer_commport,
                                                                         function, input,
                                                                         notify_commport, 0)
        );

        // Block here for return, if non-blocking then function manager has to check up on it? or send a message
        auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionInvocationAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::invokeFunction(): Received an");

        if (not msg->success)
        {
            throw ExecutionException(msg->failure_cause);
        }
        return msg->invocation;
    }

    int ServerlessComputeService::main() {
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Serverless provider starting (%s)", this->commport->get_cname());

        // Start the Head Storage Service
        startHeadStorageService();

        while (processNextMessage())
        {
            admitInvocations();
            scheduleInvocations();
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
        try
        {
            message = this->commport->getMessage();
        }
        catch (ExecutionException& e)
        {
            WRENCH_INFO(
                "Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());
        //        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto ss_mesg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message))
        {
            // TODO: Die...
            return false;
        }
        else if (auto scsfrr_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionRegisterRequestMessage>(message))
        {
            processFunctionRegistrationRequest(
                scsfrr_msg->answer_commport, scsfrr_msg->function, scsfrr_msg->time_limit_in_seconds,
                scsfrr_msg->disk_space_limit_in_bytes, scsfrr_msg->ram_limit_in_bytes,
                scsfrr_msg->ingress_in_bytes, scsfrr_msg->egress_in_bytes);
            return true;
        }
        else if (auto scsfir_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionInvocationRequestMessage>(message))
        {
            processFunctionInvocationRequest(scsfir_msg->answer_commport, scsfir_msg->function,
                                             scsfir_msg->function_input, scsfir_msg->notify_commport);
            return true;
        }
        else if (auto scsdc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceDownloadCompleteMessage>(message))
        {
            processImageDownloadCompletion(scsdc_msg->_action, scsdc_msg->_image_file);
            return true;
        }
        else
        {
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
    void ServerlessComputeService::processFunctionRegistrationRequest(S4U_CommPort* answer_commport,
                                                                      std::shared_ptr<Function> function,
                                                                      double time_limit,
                                                                      sg_size_t disk_space_limit_in_bytes,
                                                                      sg_size_t ram_limit_in_bytes,
                                                                      sg_size_t ingress_in_bytes,
                                                                      sg_size_t egress_in_bytes) {
        if (_registeredFunctions.find(function->getName()) != _registeredFunctions.end())
        {
            // TODO: Create failure case for duplicate function?
            std::string msg = "Duplicate Function";
            auto answerMessage =
                new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                    false, function,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(), msg), 0);
            answer_commport->dputMessage(answerMessage);
        }
        else
        {
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
    void ServerlessComputeService::processFunctionInvocationRequest(S4U_CommPort* answer_commport,
                                                                    std::shared_ptr<Function> function,
                                                                    std::shared_ptr<FunctionInput> input,
                                                                    S4U_CommPort* notify_commport) {
        if (_registeredFunctions.find(function->getName()) == _registeredFunctions.end())
        {
            // Not found
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                false, nullptr, std::make_shared<FunctionNotFound>(function), 0);
            answer_commport->dputMessage(answerMessage);
        }
        else
        {
            const auto invocation = std::make_shared<Invocation>(_registeredFunctions.at(function->getName()), input,
                                                                 notify_commport);
            _newInvocations.push(invocation);
            // TODO: return some sort of function invocation object?
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                true, invocation, nullptr, 0);
            answer_commport->dputMessage(answerMessage);
        }
    }

    void ServerlessComputeService::processImageDownloadCompletion(const std::shared_ptr<Action>& action,
                                                                  const std::shared_ptr<DataFile>& image_file) {
        if (action->getFailureCause())
        {
            throw std::runtime_error("ServerlessComputeService::processImageDownloadCompletion(): "
                "An image download (from remote) has failed. Handling of such failures is currently not implemented");
        }
        WRENCH_INFO("ServerlessComputeService::processImageDownloadCompletion(): Image file %s was downloaded",
                    image_file->getID().c_str());
        _being_downloaded_image_files.erase(image_file);
        _downloaded_image_files.insert(image_file);

        // Move all relevant invocations from the admitted to the schedulable queue
        auto& queue = _admittedInvocations[image_file];
        while (not queue.empty())
        {
            _schedulableInvocations.push(std::move(queue.front()));
            queue.pop();
        }
        _admittedInvocations.erase(image_file);
    }


    /**
     * @brief
     *
     */
    void ServerlessComputeService::dispatchFunctionInvocation() {
        while (!_scheduledInvocations.empty())
        {
            // Might need to think about how RegisteredFunction is storing info here...
            auto invocation_to_place = _scheduledInvocations.front();
            WRENCH_INFO("Invoking function [%s]",
                        invocation_to_place->_registered_function->_function->getName().c_str());
            _scheduledInvocations.pop();
            // TODO: Do invocation for real
            // invocation_to_place->is_running = true
            Simulation::sleep(1);
            WRENCH_INFO("Function [%s] invoked",
                        invocation_to_place->_registered_function->_function->getName().c_str());
            // invocation_to_place->output = whatever
            // invocation_to_place->_registered_function->_function->run_lambda()
            invocation_to_place->_notify_commport->dputMessage(
                new ServerlessComputeServiceFunctionInvocationCompleteMessage(true,
                                                                              invocation_to_place, nullptr, 0));
        }
    }

    void ServerlessComputeService::startHeadStorageService() {
        auto ss = SimpleStorageService::createSimpleStorageService(
            hostname,
            {_head_storage_service_mount_point},
            {
                {
                    wrench::SimpleStorageServiceProperty::BUFFER_SIZE,
                    this->getPropertyValueAsString(ComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE)
                }
            }, {});
        ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
        ss->setSimulation(this->simulation_);
        _head_storage_service = this->simulation_->startNewService(ss);
        _free_space_on_head_storage = _head_storage_service->getTotalSpace();
    }

    void ServerlessComputeService::admitInvocations() {
        // This implements a FCFS algorith. That is, if an invocation is placed for an image
        // that cannot be downloaded right now (due to lack of space), then we stop and do not
        // consider invocations that were placed later, even if their images have been downloaded
        // and are available right now. This is an arbitrary non-backfilling choice, that can later
        // be revisited (e.g., creating a property that allows the user to pick one of several
        // strategies).

        while (!_newInvocations.empty())
        {
            auto invocation = _newInvocations.front();
            auto image = invocation->_registered_function->_function->_image;

            // If the image file is already downloaded, make the invocation schedulable immediately
            if (_being_downloaded_image_files.find(image->getFile()) != _being_downloaded_image_files.end())
            {
                _newInvocations.pop();
                _schedulableInvocations.push(invocation);
                continue;
            }

            // If the image file is being downloaded, make the invocation admitted
            if (_being_downloaded_image_files.find(image->getFile()) != _being_downloaded_image_files.end())
            {
                _newInvocations.pop();
                _admittedInvocations[image->getFile()].push(invocation);
                continue;
            }

            // Otherwise, if there is enough space on the head node storage service to store it,
            // then launch the downloaded and admit the invocation
            if (_free_space_on_head_storage >= image->getFile()->getSize())
            {
                // "Reserve" space on the storage service
                _free_space_on_head_storage -= image->getFile()->getSize();
                // initiate the download
                initiateImageDownloadFromRemote(invocation);
                _newInvocations.pop();
                _admittedInvocations[image->getFile()].push(invocation);
                continue;
            }

            // If we're here, we couldn't admit invocations, and so we stop
            break;
        }
    }


    void ServerlessComputeService::initiateImageDownloadFromRemote(const std::shared_ptr<Invocation>& invocation) {
        // Create a custom action (we could use a simple FileCopyAction here, but we are using a CustomAction
        // to demonstrate its use)
        const std::function lambda_execute = [invocation, this](std::shared_ptr<ActionExecutor> action_executor) {
            const auto src_location = invocation->_registered_function->_function->_image;
            const auto dst_location = FileLocation::LOCATION(_head_storage_service, src_location->getFile());
            StorageService::copyFile(src_location, dst_location);
        };
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(name, 0, 0, lambda_execute, nullptr));

        // Spin up an ActionExecutor service, and have it send us back a custom message
        auto custom_message = new ServerlessComputeServiceDownloadCompleteMessage(
            action,
            invocation->_registered_function->_function->_image->getFile(), 0);

        auto action_executor = std::make_shared<ActionExecutor>(
            this->getHostname(),
            0,
            0,
            0,
            false,
            this->commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        action_executor->start(action_executor, true, false); // Daemonized, no auto-restart
    }

    void ServerlessComputeService::scheduleInvocations() {
        // TODO: Implement something fancy.
        WRENCH_INFO("I should be scheduling invocations, but for now I am just making them runnable instantly");
        while (!_schedulableInvocations.empty())
        {
            _scheduledInvocations.push(std::move(_schedulableInvocations.front()));
        }
        return;
    }
};
