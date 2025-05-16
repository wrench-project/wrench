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
    unsigned long ServerlessComputeService::sequence_number = 0;
    /**
     * @brief Constructor
     *
     * @param hostname name of the head host on which the service runs
     * @param head_node_storage_mount_point the mount point of storage at the head host (where images will be stored)
     * @param compute_hosts list of (homogeneous) compute node hostnames
     * @param scheduler the scheduler used to decide which invocations should be executed and when
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    ServerlessComputeService::ServerlessComputeService(const std::string& hostname,
                                                       const std::string& head_node_storage_mount_point,
                                                       const std::vector<std::string>& compute_hosts,
                                                       const std::shared_ptr<ServerlessScheduler>& scheduler,
                                                       const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
                                                       const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE&
                                                       messagepayload_list) :
        ComputeService(hostname,
                       "ServerlessComputeService", "") {
        // Check platform homogeneity
        check_homogeneity(compute_hosts);

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        // Set default and specified properties
        this->setProperties(this->default_property_values, property_list);

        // Create the state of the system object
        _state_of_the_system = std::shared_ptr<ServerlessStateOfTheSystem>(
            new ServerlessStateOfTheSystem(compute_hosts));
        _state_of_the_system->_head_storage_service_mount_point = head_node_storage_mount_point;

        _scheduler = scheduler;
    }


    /**
     * @brief Helper method to check homogeneity of the compute hosts
     * @param compute_hosts a list of compute hosts
     */
    void ServerlessComputeService::check_homogeneity(const std::vector<std::string>& compute_hosts) {
        // Check Platform homogeneity
        auto first_hostname = *(compute_hosts.begin());
        this->num_cores_of_compute_host = S4U_Simulation::getHostNumCores(first_hostname);
        this->speed_of_compute_core = S4U_Simulation::getHostFlopRate(first_hostname);
        this->ram_of_compute_host = S4U_Simulation::getHostMemoryCapacity(first_hostname);
        try {
            this->disk_space_of_compute_host = S4U_Simulation::getDiskCapacity(first_hostname, "/");
        }
        catch (std::invalid_argument& e) {
            throw std::invalid_argument("Compute hosts for a serverless compute service must have a '/' mountpoint");
        }

        for (auto const& hostname : compute_hosts) {
            auto num_cores_available = S4U_Simulation::getHostNumCores(hostname);
            double speed = S4U_Simulation::getHostFlopRate(hostname);
            sg_size_t ram_available = S4U_Simulation::getHostMemoryCapacity(hostname);
            sg_size_t disk_capacity;
            try {
                disk_capacity = S4U_Simulation::getDiskCapacity(hostname, "/");
            }
            catch (std::invalid_argument& e) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless compute service must have a '/' mountpoint");
            }

            // Compute speed
            if (std::abs(speed - this->speed_of_compute_core) > DBL_EPSILON) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless compute service need "
                    "to be homogeneous (different flop rates detected)");
            }
            // RAM
            if (ram_available != this->ram_of_compute_host) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless compute service need "
                    "to be homogeneous (different RAM capacities detected)");
            }
            // Num cores
            if (num_cores_available != this->num_cores_of_compute_host) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless service need "
                    "to be homogeneous (different number of cores detected)");
            }
            // Disk capacity
            if (disk_capacity != this->disk_space_of_compute_host) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless service need "
                    "to be homogeneous (different disk capacities detected)");
            }
        }
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
    * @brief Returns true if the service supports functions
    * @return true or false
    */
    bool ServerlessComputeService::supportsFunctions() {
        return true;
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
        // Build a dictionary
        std::map<std::string, double> information;

        if (key == "num_hosts") {
            // Num hosts
            std::map<std::string, double> num_hosts;
            num_hosts.insert(std::make_pair(this->getName(), this->_state_of_the_system->_compute_hosts.size()));
            return num_hosts;
        }
        else if (key == "num_cores") {
            // Num cores per hosts
            std::map<std::string, double> num_cores;
            for (auto const& h : this->_state_of_the_system->_compute_hosts) {
                num_cores[h] = static_cast<double>(S4U_Simulation::getHostNumCores(h));
            }
            return num_cores;
        }
        else if (key == "num_idle_cores") {
            // Num idle cores per hosts
            std::map<std::string, double> num_idle_cores;
            for (const auto& [hostname, cores] : this->_state_of_the_system->_available_cores) {
                num_idle_cores[hostname] = static_cast<double>(cores);
            }
            return num_idle_cores;
        }
        else if (key == "flop_rates") {
            // Flop rate per host
            std::map<std::string, double> flop_rates;
            for (const auto& h : this->_state_of_the_system->_compute_hosts) {
                flop_rates[h] = S4U_Simulation::getHostFlopRate(h);
            }
            return flop_rates;
        }
        else if (key == "ram_capacities") {
            // RAM capacity per host
            std::map<std::string, double> ram_capacities;
            for (const auto& h : this->_state_of_the_system->_compute_hosts) {
                ram_capacities[h] = static_cast<double>(S4U_Simulation::getHostMemoryCapacity(h));
            }
            return ram_capacities;
        }
        else if (key == "ram_availabilities") {
            // RAM availability per host
            std::map<std::string, double> ram_availability;
            for (auto const& h : this->_state_of_the_system->_compute_hosts) {
                auto ss = this->_state_of_the_system->_compute_memories[h];
                ram_availability[h] = (ss ? static_cast<double>(ss->getTotalFreeSpaceZeroTime()) : 0.0);
            }
            return ram_availability;
        }
        else {
            throw std::runtime_error("ServerlessComputeService::getResourceInformation(): unknown key");
        }
    }

    /**
     * @brief Register a function in the serverless compute service
     *
     * @param function the function to register
     * @param time_limit_in_seconds the time limit for execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param RAM_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit
     * @param egress_in_bytes the egress data limit
     * @return A RegisteredFunction object
     * @throw ExecutionException if the function registration fails
     */
    std::shared_ptr<RegisteredFunction> ServerlessComputeService::registerFunction(
        const std::shared_ptr<Function>& function, const double time_limit_in_seconds,
        const sg_size_t disk_space_limit_in_bytes, const sg_size_t RAM_limit_in_bytes,
        const sg_size_t ingress_in_bytes, const sg_size_t egress_in_bytes) {
        // WRENCH_INFO("Serverless Provider Registered function %s", function->getName().c_str());
        const auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        //  send a "run a standard job" message to the daemon's commport
        this->commport->putMessage(
            new ServerlessComputeServiceFunctionRegisterRequestMessage(
                answer_commport, function, time_limit_in_seconds, disk_space_limit_in_bytes, RAM_limit_in_bytes,
                ingress_in_bytes, egress_in_bytes,
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        const auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionRegisterAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::registerFunction(): Received an");

        // TODO: Deal with failures later
        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
        return msg->registered_function;
    }

    /**
     * @brief Invoke a function in the serverless compute service
     *
     * @param registered_function the (registered) function to invoke
     * @param input the input to the function
     * @param notify_commport the ExecutionController commport to notify
     * @return std::shared_ptr<Invocation> Pointer to the invocation created by the ServerlessComputeService
     */
    std::shared_ptr<Invocation> ServerlessComputeService::invokeFunction(
        const std::shared_ptr<RegisteredFunction>& registered_function, const std::shared_ptr<FunctionInput>& input,
        S4U_CommPort* notify_commport) {
        const auto answer_commport = S4U_CommPort::getTemporaryCommPort();
        this->commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationRequestMessage(answer_commport,
                                                                         registered_function, input,
                                                                         notify_commport, this->getMessagePayloadValue(
                                                                             ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_REQUEST_MESSAGE_PAYLOAD)));

        // Block here for return, if non-blocking then function manager has to check up on it? or send a message
        const auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionInvocationAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::invokeFunction(): Received an");

        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
        return msg->invocation;
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int ServerlessComputeService::main() {
        S4U_Simulation::computeZeroFlop(); // to block in case pstate speed is 0
        this->state = Service::UP;

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_MAGENTA);
        WRENCH_INFO("Serverless provider starting (%s)", this->commport->get_cname());

        // Start the Head Storage Service
        startHeadStorageService();
        // Start a storage service on each host.
        startComputeHostsServices();

        bool do_scheduling;
        while (processNextMessage(do_scheduling)) {
            if (do_scheduling) {
                // Make invocations whose images have downloaded schedulable
                admitInvocations();

                // Invoke the scheduler
                auto decisions = invokeScheduler();

                // Implement the scheduler's decisions, if possible.
                // It's important to do things in this order below so that files get open(), and thus
                // unevictable, thus preventing ping-pong effects.
                dispatchInvocations(decisions);
                initiateImageLoads(decisions);
                initiateImageCopies(decisions);
            }
        }
        return 0;
    }

    /**
     * @brief Process the next message in the commport
     *
     * @return true if the ServerlessComputeService daemon should continue processing messages
     * @return false if the ServerlessComputeService daemon should die
     */
    bool ServerlessComputeService::processNextMessage(bool& do_scheduling) {
        S4U_Simulation::computeZeroFlop();

        // By default, set do_scheduling to true
        do_scheduling = true;

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = this->commport->getMessage();
        }
        catch (ExecutionException& e) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (const auto ss_mesg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // TODO: Die...
            return false;
        }
        else if (const auto scsfrr_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionRegisterRequestMessage>(message)) {
            processFunctionRegistrationRequest(
                scsfrr_msg->answer_commport, scsfrr_msg->function, scsfrr_msg->time_limit_in_seconds,
                scsfrr_msg->disk_space_limit_in_bytes, scsfrr_msg->ram_limit_in_bytes,
                scsfrr_msg->ingress_in_bytes, scsfrr_msg->egress_in_bytes);
            return true;
        }
        else if (const auto scsfir_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionInvocationRequestMessage>(message)) {
            processFunctionInvocationRequest(scsfir_msg->answer_commport, scsfir_msg->registered_function,
                                             scsfir_msg->function_input, scsfir_msg->notify_commport);
            return true;
        }
        else if (const auto scsdc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceDownloadCompleteMessage>(message)) {
            processImageDownloadCompletion(scsdc_msg->_action, scsdc_msg->_image_file);
            return true;
        }
        else if (const auto scsiec_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceInvocationExecutionCompleteMessage>(message)) {
            processInvocationCompletion(scsiec_msg->_invocation, scsiec_msg->_action);
            return true;
        }
        else if (const auto scsncc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodeCopyCompleteMessage>(message)) {
            _state_of_the_system->_being_copied_images[scsncc_msg->_compute_host].erase(scsncc_msg->_image_file);
            if (scsncc_msg->_action->getState() != Action::State::COMPLETED) {
                WRENCH_INFO("An image copy has failed (due to disk pressure) for image %s... nevermind",
                            scsncc_msg->_image_file->getID().c_str());
                do_scheduling = false;
            }
            else {
                WRENCH_INFO("ServerlessComputeService::processNextMessage(): Image file %s was stored at %s",
                            scsncc_msg->_image_file->getID().c_str(), scsncc_msg->_compute_host.c_str());
            }
            // _state_of_the_system->_copied_images[scsncc_msg->_compute_host].insert(scsncc_msg->_image_file);
            return true;
        }
        else if (const auto scsnlc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodeLoadCompleteMessage>(message)) {
            _state_of_the_system->_being_loaded_images[scsnlc_msg->_compute_host].erase(scsnlc_msg->_image_file);
            if (scsnlc_msg->_action->getState() != Action::State::COMPLETED) {
                WRENCH_INFO("An image load has failed (due to memory pressure) for image %s... nevermind",
                            scsnlc_msg->_image_file->getID().c_str());
                do_scheduling = false;
            }
            else {
                WRENCH_INFO("ServerlessComputeService::processNextMessage(): Image file %s was loaded at %s",
                            scsnlc_msg->_image_file->getID().c_str(), scsnlc_msg->_compute_host.c_str());
            }
            return true;
        }
        else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
     * @brief Processes a "function registration request" message
     *
     * @param answer_commport the FunctionManager commport to answer to
     * @param function the function to register
     * @param time_limit the time limit for execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param ram_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit
     * @param egress_in_bytes the egress data limit
     */
    void ServerlessComputeService::processFunctionRegistrationRequest(S4U_CommPort* answer_commport,
                                                                      const std::shared_ptr<Function>& function,
                                                                      double time_limit,
                                                                      sg_size_t disk_space_limit_in_bytes,
                                                                      sg_size_t ram_limit_in_bytes,
                                                                      sg_size_t ingress_in_bytes,
                                                                      sg_size_t egress_in_bytes) {
        // Check that function can ever run!
        {
            sg_size_t needed_disk_space = function->getImage()->getFile()->getSize() + disk_space_limit_in_bytes;
            sg_size_t needed_ram_space = function->getImage()->getFile()->getSize() + ram_limit_in_bytes;

            if (needed_disk_space > this->disk_space_of_compute_host) {
                const auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                    false, nullptr,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(),
                                                 "Function cannot be registered because no compute host has sufficient disk space to execute it"),
                    this->getMessagePayloadValue(
                        ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD));
                answer_commport->dputMessage(answerMessage);
            }

            if (needed_ram_space > this->ram_of_compute_host) {
                const auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                    false, nullptr,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(),
                                                 "Function cannot be registered because no compute host has sufficient RAM space to execute it"),
                    this->getMessagePayloadValue(
                        ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD));
                answer_commport->dputMessage(answerMessage);
            }
        }


        // Register the function
        auto registered_function = std::make_shared<RegisteredFunction>(
            function,
            time_limit,
            disk_space_limit_in_bytes,
            ram_limit_in_bytes,
            ingress_in_bytes,
            egress_in_bytes);

        _state_of_the_system->_registered_functions.insert(registered_function);

        const auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(
            true, registered_function, nullptr, this->getMessagePayloadValue(
                ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD));
        answer_commport->dputMessage(answerMessage);
    }

    /**
     * @brief Processes a "function invocation request" message
     *
     * @param answer_commport the FunctionManager commport to answer to
     * @param registered_function the (registered) function to invoke
     * @param input the input to the function
     * @param notify_commport the ExecutionController commport to notify
     */
    void ServerlessComputeService::processFunctionInvocationRequest(S4U_CommPort* answer_commport,
                                                                    const std::shared_ptr<RegisteredFunction>
                                                                    & registered_function,
                                                                    const std::shared_ptr<FunctionInput>& input,
                                                                    S4U_CommPort* notify_commport) {
        if (_state_of_the_system->_registered_functions.find(registered_function) ==
            _state_of_the_system->_registered_functions.end()) {
            // Not found
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                false, nullptr, std::make_shared<FunctionNotFound>(registered_function), this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD));
            answer_commport->dputMessage(answerMessage);
        }
        else {
            auto invocation = std::make_shared<Invocation>(registered_function, input, notify_commport);
            invocation->_submit_date = Simulation::getCurrentSimulatedDate();
            _state_of_the_system->_new_invocations.push(invocation);
            auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                true, invocation, nullptr, 0);
            answer_commport->dputMessage(answerMessage);
        }
    }

    /**
     * @brief Helper method to process an "image download completion" message
     *
     * @param action to get failure cause from
     * @param image_file The image file that was downloaded, used as key to map downloading functions
     */
    void ServerlessComputeService::processImageDownloadCompletion(const std::shared_ptr<Action>& action,
                                                                  const std::shared_ptr<DataFile>& image_file) {
        if (action->getFailureCause()) {
            throw std::runtime_error("ServerlessComputeService::processImageDownloadCompletion(): "
                "An image download (from remote) has failed. Handling of such failures is currently not implemented");
        }
        WRENCH_INFO("ServerlessComputeService::processImageDownloadCompletion(): Image file %s was downloaded",
                    image_file->getID().c_str());
        _state_of_the_system->_being_downloaded_image_files.erase(image_file);
        // _state_of_the_system->_downloaded_image_files.insert(image_file);

        // Move all relevant invocations from the admitted to the schedulable queue
        auto& queue = _state_of_the_system->_admitted_invocations[image_file];
        while (not queue.empty()) {
            _state_of_the_system->_schedulable_invocations.emplace(
                _state_of_the_system->_schedulable_invocations.end(), std::move(queue.front()));
            queue.pop();
        }
        _state_of_the_system->_admitted_invocations.erase(image_file);
    }

    /**
     * @brief Helper method to process an invocation completion
     * @param invocation The invocation that has completed
     * @param action The action that was responsible for running the invocation
     */
    void ServerlessComputeService::processInvocationCompletion(const std::shared_ptr<Invocation>& invocation,
                                                               const std::shared_ptr<Action>& action) {
        std::shared_ptr<FailureCause> failure_cause = action->getFailureCause();
        invocation->_end_date = Simulation::getCurrentSimulatedDate();
        WRENCH_INFO("A function invocation for function %s has finished [%s]",
                    invocation->getRegisteredFunction()->getFunction()->getName().c_str(),
                    (failure_cause ? "FAILURE" : "SUCCESS"));

        const auto host = invocation->_target_host;
        // _state_of_the_system->_scheduling_decisions.erase(invocation);
        invocation->_opened_image_ram_file->close();
        invocation->_opened_tmp_ram_file->close();
        StorageService::removeFileAtLocation(invocation->_tmp_ram_file_location);
        _state_of_the_system->_available_cores[host]++;

        bool success = action->getState() == Action::State::COMPLETED;


        invocation->_notify_commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                success,
                invocation,
                failure_cause, this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
    }


    /**
     * @brief Dispatches scheduled function invocations to compute hosts
     * @return true if at least one invocation was dispatched
     */
    void ServerlessComputeService::dispatchInvocations(const std::shared_ptr<SchedulingDecisions>& decisions) {
        // Dispatched the invocations in the order of the schedulable list
        std::set<std::shared_ptr<Invocation>> dispatched_invocations;
        for (const auto& [hostname, invocations_to_place] : decisions->invocations_to_start_at_compute_node) {
            for (const auto& invocation : invocations_to_place) {
                // WRENCH_INFO("Trying to dispatch scheduled invocation for function [%s]...",
                //             invocation_to_place->_registered_function->_function->getName().c_str());

                if (dispatchInvocation(invocation, hostname)) {
                    _state_of_the_system->_running_invocations.push(invocation);
                    invocation->_target_host = hostname;
                    dispatched_invocations.insert(invocation);
                }
            }
        }

        // Update the list of schedulable invocations (not super efficient, but clearer than the erase-as-I-go)
        std::vector<std::shared_ptr<Invocation>> updated_list_of_schedulable_invocations;
        for (auto const& invocation : _state_of_the_system->_schedulable_invocations) {
            if (dispatched_invocations.find(invocation) == dispatched_invocations.end()) {
                updated_list_of_schedulable_invocations.push_back(invocation);
                dispatched_invocations.erase(invocation); // Should be more efficient
            }
        }
        _state_of_the_system->_schedulable_invocations = updated_list_of_schedulable_invocations;
    }

    /**
     * @brief Helper method to ensure that an invocation can be started
     * @param invocation: the invocation to start
     * @param hostname: the hostname on which to start it
     * @return true if the invocation can be started, false otherwise
     */
    bool ServerlessComputeService::invocationCanBeStarted(
        const std::shared_ptr<Invocation>& invocation,
        const std::string& hostname) const {
        auto ss_memory = _state_of_the_system->_compute_memories[hostname];
        auto image_file = invocation->getRegisteredFunction()->getOriginalImageLocation()->getFile();

        // The image is in RAM
        if (not ss_memory->hasFile(image_file, "/ram_disk")) {
            WRENCH_INFO("Scheduled invocation cannot be started because image %s is not loaded at node %s",
                        image_file->getID().c_str(), hostname.c_str());
            return false;
        }
        // There is an available core
        if (_state_of_the_system->_available_cores[hostname] < 1) {
            WRENCH_INFO("Scheduled invocation cannot be started because there is no available core");
            return false;
        }
        // We shouldn't check this, this will fail if LRU says it should...
        // // There is available RAM space for the function itself
        // if (ss_memory->getTotalFreeSpaceZeroTime() < invocation->getRegisteredFunction()->getRAMLimit()) {
        //     WRENCH_INFO("Scheduled invocation cannot be started because there is not enough available RAM");
        //     return false;
        // }
        return true;
    }


    /**
     * @brief Helper method to dispatch an invocation
     *
     * @param invocation invocation to dispatch
     * @param target_host the target host
     * @return true on success, false on failure
     */
    bool ServerlessComputeService::dispatchInvocation(const std::shared_ptr<Invocation>& invocation,
                                                      const std::string& target_host) {
        // Check that things can work, which may not be the case because scheduling and LRU is complicated
        if (not invocationCanBeStarted(invocation, target_host)) {
            return false;
        }

        // Start the invocation's own private storage service, on disk, if possible
        std::shared_ptr<StorageService> private_ss;
        try {
            private_ss = startInvocationStorageService(invocation, target_host);
        }
        catch (ExecutionException& e) {
            WRENCH_INFO("Couldn't start private on-disk storage for an invocation for %s due to lack of space",
                        invocation->_registered_function->_function->getName().c_str());
            return false;
        }


        // Create and open a tmp memory file in RAM and open it, if possible
        auto tmp_memory_file = Simulation::addFile(
            "tmp_ram_file_" + std::to_string(++ServerlessComputeService::sequence_number),
            invocation->getRegisteredFunction()->getRAMLimit());
        auto compute_ram_ss = _state_of_the_system->_compute_memories[target_host];
        try {
            auto file_location = FileLocation::LOCATION(compute_ram_ss, tmp_memory_file);
            StorageService::createFileAtLocation(file_location);
            invocation->_tmp_ram_file_location = file_location;
            invocation->_opened_tmp_ram_file = compute_ram_ss->openFile(invocation->_tmp_ram_file_location);
        }
        catch (ExecutionException& e) {
            WRENCH_INFO("Couldn't create a private RAM space for an invocation for %s due to lack of space",
                        invocation->_registered_function->_function->getName().c_str());
            // Kill private storage service
            private_ss->stop();
            return false;
        }

        // Open the image memory file
        invocation->_opened_image_ram_file = compute_ram_ss->openFile(
            FileLocation::LOCATION(compute_ram_ss,
                                   invocation->getRegisteredFunction()->getOriginalImageLocation()->getFile()));


        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        const std::function lambda_execute = [invocation](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            const auto function = invocation->_registered_function->_function;

            // Invoke the user's lambda function
            invocation->_function_output = function->_lambda(invocation->_function_input,
                                                             invocation->_tmp_storage_service);

            // Clean up the on-disk storage
            invocation->_tmp_storage_service->stop();
            invocation->_opened_tmp_file->close();
            invocation->_tmp_storage_service = nullptr; // Should free up all memory...
            StorageService::removeFileAtLocation(invocation->_tmp_file);
            // WRENCH_INFO("Done with the lambda execute!!");
        };


        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "run_invocation_" + invocation->_registered_function->_function->getName(),
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceInvocationExecutionCompleteMessage(
            action,
            invocation, 0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            target_host,
            1,
            0,
            this->getPropertyValueAsDouble(ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD),
            false,
            this->commport,
            custom_message,
            action,
            nullptr);

        action_executor->setActionTimeout(invocation->getRegisteredFunction()->getTimeLimit());
        action_executor->setSimulation(this->simulation_);

        WRENCH_INFO("Dispatched an invocation for function %s",
                    invocation->getRegisteredFunction()->getFunction()->getName().c_str());
        _state_of_the_system->_available_cores[target_host] -= 1;
        invocation->_start_date = Simulation::getCurrentSimulatedDate();
        action_executor->start(action_executor, true, false);

        // WRENCH_INFO("Function [%s] invoked", invocation->_registered_function->_function->getName().c_str());
        return true;
    }

    /**
     * @brief Start a SimpleStorageService for each compute host. We don't start a bare-metal
     *        service as we'll do everything ourselves with action executor services.
     */
    void ServerlessComputeService::startComputeHostsServices() {
        for (auto const& hostname : _state_of_the_system->_compute_hosts) {
            if (not S4U_Simulation::hostHasMountPoint(hostname, "/")) {
                throw std::invalid_argument("ServerlessComputeService::startComputeHostsServices(): "
                    "each compute host in a serverless compute service should have a \"/\" mountpoint");
            }

            // Start a compute service, with LRU caching, to implement compute-node storage
            {
                const auto ss = std::dynamic_pointer_cast<SimpleStorageService>(this->simulation_->startNewService(
                    SimpleStorageService::createSimpleStorageService(
                        hostname,
                        {"/"},
                        {{SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}},
                        {})));
                ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
                _state_of_the_system->_compute_storages[hostname] = ss;
            }

            // Start a compute service, with LRU caching, to implement compute-node memory
            {
                // Create a RAM disk on the host
                auto host = simgrid::s4u::Engine::get_instance()->host_by_name(hostname);
                auto ram_disk = host->add_disk("ram_disk",
                                               S4U_Simulation::RAM_READ_BANDWIDTH,
                                               S4U_Simulation::RAM_WRITE_BANDWIDTH);
                auto ram_capacity = S4U_Simulation::getHostMemoryCapacity(hostname);
                std::string ram_mount_point = "/ram_disk";
                ram_disk->set_property("size", std::to_string(ram_capacity) + "B");
                ram_disk->set_property("mount", ram_mount_point);

                const auto ss = std::dynamic_pointer_cast<SimpleStorageService>(this->simulation_->startNewService(
                    SimpleStorageService::createSimpleStorageService(
                        hostname,
                        {ram_mount_point},
                        {{SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"}},
                        {})));
                ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
                _state_of_the_system->_compute_memories[hostname] = ss;
            }
        }
    }

    /**
     * @brief Method to create a tmp storage service for an invocation
     *
     * @param invocation A function invocation
     * @param target_host the target host
     * @return A storage service
     */
    std::shared_ptr<StorageService> ServerlessComputeService::startInvocationStorageService(
        const std::shared_ptr<Invocation>& invocation,
        const std::string& target_host) {
        // WRENCH_INFO("Starting a new storage service for an invocation...");
        // Reserve space on the storage service if possible
        std::shared_ptr<FileLocation> tmp_file;
        std::shared_ptr<simgrid::fsmod::File> opened_tmp_file;
        try {
            tmp_file = wrench::FileLocation::LOCATION(_state_of_the_system->_compute_storages[target_host],
                                                      Simulation::addFile(
                                                          "tmp_" + std::to_string(
                                                              ++ServerlessComputeService::sequence_number),
                                                          invocation->_registered_function->_disk_space));
            StorageService::createFileAtLocation(tmp_file);
            opened_tmp_file = _state_of_the_system->_compute_storages[target_host]->openFile(tmp_file);
        }
        catch (ExecutionException& e) {
            throw;
        }

        // Create a tmp file system
        const auto disk = S4U_Simulation::hostHasMountPoint(target_host, "/");
        const auto ods = simgrid::fsmod::OneDiskStorage::create(
            "is_" + std::to_string(ServerlessComputeService::sequence_number), disk);
        const auto fs = simgrid::fsmod::FileSystem::create(
            "fs" + std::to_string(ServerlessComputeService::sequence_number));
        fs->mount_partition("/", ods, invocation->_registered_function->_disk_space);

        // Create a tmp storage service
        auto ss = std::shared_ptr<SimpleStorageService>(
            SimpleStorageService::createSimpleStorageServiceWithExistingFileSystem(target_host, fs, {}, {}));
        ss->setSimulation(this->simulation_);
        ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
        ss->start(ss, true, false);

        // Keep track of all this
        invocation->_tmp_file = tmp_file;
        invocation->_opened_tmp_file = opened_tmp_file;
        invocation->_tmp_storage_service = ss;

        return ss;
    }

    /**
     * @brief Helper method to start the storage service on the head node
     *
     */
    void ServerlessComputeService::startHeadStorageService() {
        const auto ss = SimpleStorageService::createSimpleStorageService(
            hostname,
            {_state_of_the_system->_head_storage_service_mount_point},
            {
                {
                    wrench::SimpleStorageServiceProperty::BUFFER_SIZE,
                    this->getPropertyValueAsString(ComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE)
                }
            },
            {});
        ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
        ss->setSimulation(this->simulation_);
        _state_of_the_system->_head_storage_service = this->simulation_->startNewService(ss);
        _state_of_the_system->_free_space_on_head_storage = _state_of_the_system->_head_storage_service->
            getTotalSpace();
    }

    /**
     * @brief Helper method to admit invocations
     *
     */
    void ServerlessComputeService::admitInvocations() {
        // This implements a FCFS algorithm. That is, if an invocation is placed for an image
        // that cannot be downloaded right now (due to lack of space), then we stop and do not
        // consider invocations that were placed later, even if their images have been downloaded
        // and are available right now. This is an arbitrary non-backfilling choice, that can later
        // be revisited (e.g., creating a property that allows the user to pick one of several
        // strategies).
        while (!_state_of_the_system->_new_invocations.empty()) {
            // WRENCH_INFO("Admitting an invocation...");
            auto invocation = _state_of_the_system->_new_invocations.front();
            const auto image = invocation->_registered_function->_function->_image;
            // std::cerr << "ADMITTING INVOCATION.. " << invocation->_registered_function->_function->_image->getFile()->getID() << std::endl;

            // If the image file is already downloaded, make the invocation schedulable immediately
            if (_state_of_the_system->_head_storage_service->hasFile(image->getFile())) {
                _state_of_the_system->_new_invocations.pop();
                _state_of_the_system->_schedulable_invocations.emplace(
                    _state_of_the_system->_schedulable_invocations.begin(), invocation);
                continue;
            }

            // If the image file is being downloaded, make the invocation admitted
            if (_state_of_the_system->_being_downloaded_image_files.find(image->getFile()) != _state_of_the_system->
                _being_downloaded_image_files.end()) {
                _state_of_the_system->_new_invocations.pop();
                _state_of_the_system->_admitted_invocations[image->getFile()].push(invocation);
                continue;
            }

            // Otherwise, if there is enough space on the head node storage service to store it,
            // then launch the downloaded and admit the invocation
            if (_state_of_the_system->_free_space_on_head_storage >= image->getFile()->getSize()) {
                // "Reserve" space on the storage service
                _state_of_the_system->_free_space_on_head_storage -= image->getFile()->getSize();
                // initiate the download
                _state_of_the_system->_being_downloaded_image_files.insert(image->getFile());
                initiateImageDownloadFromRemote(invocation);
                _state_of_the_system->_new_invocations.pop();
                _state_of_the_system->_admitted_invocations[image->getFile()].push(invocation);
                continue;
            }

            // If we're here, we couldn't admit invocations, and so we stop
            break;
        }
    }


    /**
     * @brief Helper method to initiate an image download
     *
     * @param invocation an invocation for which the download is needed
     */
    void ServerlessComputeService::initiateImageDownloadFromRemote(const std::shared_ptr<Invocation>& invocation) {
        // Create a custom action (we could use a simple FileCopyAction here, but we are using a CustomAction
        // to demonstrate its use)
        // std::cerr << "INITIATING DOWNLOAD FROM REMOTE: " << invocation->_registered_function->_function->_image->getFile()->getID() << std::endl;
        const std::function lambda_execute = [invocation, this
            ](const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the lambda execute!!");
            const auto src_location = invocation->_registered_function->_function->_image;
            const auto dst_location = FileLocation::LOCATION(_state_of_the_system->_head_storage_service,
                                                             src_location->getFile());
            StorageService::copyFile(src_location, dst_location);
        };
        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "download_image_" + invocation->_registered_function->_function->_image->getFile()->getID(),
                0, 0, lambda_execute, lambda_terminate));

        // Spin up an ActionExecutor service, and have it send us back a custom message
        auto custom_message = new ServerlessComputeServiceDownloadCompleteMessage(
            action,
            invocation->_registered_function->_function->_image->getFile(), 0);

        const auto action_executor = std::make_shared<ActionExecutor>(
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
        // WRENCH_INFO("Starting an action executor for downloading from remote...");
        action_executor->start(action_executor, true, false); // Daemonized, no auto-restart
    }

    /**
     * @brief Helper method to invoke the scheduler
     * @return the scheduler's scheduling decisions
     */
    std::shared_ptr<SchedulingDecisions> ServerlessComputeService::invokeScheduler() const {
        // Invoke the scheduler so that it manages images
        auto decisions = _scheduler->schedule(_state_of_the_system->_schedulable_invocations, _state_of_the_system);
        // std::cerr << "DECISIONS:  COPY=" <<
        //     decisions->images_to_copy_to_compute_node.size() << " LOAD=" <<
        //     decisions->images_to_load_into_RAM_at_compute_node.size() << " INVOKE=" <<
        //     decisions->invocations_to_start_at_compute_node.size() << "\n";
        return decisions;
    }

    /**
     * @brief Helper method to initiate image loads
     * @param decisions scheduling decisions
     */
    void ServerlessComputeService::initiateImageLoads(const std::shared_ptr<SchedulingDecisions>& decisions) {
        // For each compute node, load initiate image load from disk into RAM and if space
        for (const auto& [hostname, images_to_load] : decisions->images_to_load_into_RAM_at_compute_node) {
            for (const auto& image : images_to_load) {
                initiateImageLoadAtComputeHost(hostname, image);
            }
        }
    }

    /**
    * @brief Helper method to initiate image copies
    * @param decisions scheduling decisions
    */
    void ServerlessComputeService::initiateImageCopies(const std::shared_ptr<SchedulingDecisions>& decisions) {
        // For each compute node, initiate image copy (from head node) if need be
        for (const auto& [hostname, image_files] : decisions->images_to_copy_to_compute_node) {
            for (const auto& image : image_files) {
                initiateImageCopyToComputeHost(hostname, image);
            }
        }
    }

    /**
     * @brief Method to initiate an image copy from the head host to a compute host
     * @param compute_host The compute host
     * @param image The image
     */
    void ServerlessComputeService::initiateImageCopyToComputeHost(const std::string& compute_host,
                                                                  const std::shared_ptr<DataFile>& image) {
        // Add the image to the being_copied_images data structure for this host
        _state_of_the_system->_being_copied_images[compute_host].insert(image);

        // std::cerr << "INITIATING IMAGE COPY FOR " << image->getID() << std::endl;
        // Initiate an asynchronous action that copies the image (identified by imageID)
        // from the head node storage service to the compute node's storage service.
        // This might involve creating and starting a dedicated ActionExecutor.
        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        const std::function lambda_execute = [compute_host, image, this](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the image copy lambda execute!!");

            // Copy the image file from the head host to the current host's storage service
            auto head_host_image_path = FileLocation::LOCATION(_state_of_the_system->_head_storage_service, image);
            auto local_image_path = wrench::FileLocation::LOCATION(
                _state_of_the_system->_compute_storages[compute_host],
                image);
            StorageService::copyFile(head_host_image_path, local_image_path);
            // WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "copy_image_" + image->getID() + "_to_" + compute_host,
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceNodeCopyCompleteMessage(
            action,
            image,
            compute_host,
            0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            compute_host,
            1,
            0,
            0,
            false,
            this->commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        // WRENCH_INFO("Starting an action executor for copying image...");
        action_executor->start(action_executor, true, false);

        // WRENCH_INFO("Initiating image copy for image [%s] to compute host [%s]", image->getID().c_str(),
        //             computeHost.c_str());
    }

    /**
     * @brief Method to initiate an image load from disk to RAM at a compute host
     * @param compute_host The compute host
     * @param image The image
     */
    void ServerlessComputeService::initiateImageLoadAtComputeHost(const std::string& compute_host,
                                                                  const std::shared_ptr<DataFile>& image) {
        // Add the image to the being_copied_images data structure for this host
        _state_of_the_system->_being_loaded_images[compute_host].insert(image);

        // std::cerr << "INITIATE IMAGE LOAD FOR " << image->getID() << std::endl;
        // Initiate an asynchronous action that simply read the image file from disk
        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        const std::function lambda_execute = [compute_host, image, this](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the image load lambda execute!!");
            auto src_location = wrench::FileLocation::LOCATION(
                _state_of_the_system->_compute_storages[compute_host], image);
            auto dst_location = wrench::FileLocation::LOCATION(
                _state_of_the_system->_compute_memories[compute_host], "/ram_disk", image);
            try {
                StorageService::copyFile(src_location, dst_location);
                // std::cerr << "THE IMAGE LOAD SUCCEEDED\n";
            }
            catch (ExecutionException& e) {
                // std::cerr << "THE IMAGE LOAD FAILED: " << e.what() << std::endl;
                throw;
            }
            // WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "load_image_" + image->getID() + "_at_" + compute_host,
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceNodeLoadCompleteMessage(
            action,
            image,
            compute_host,
            0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            compute_host,
            1,
            0,
            0,
            false,
            this->commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        // WRENCH_INFO("Starting an action executor for copying image...");
        action_executor->start(action_executor, true, false);

        // WRENCH_INFO("Initiating image copy for image [%s] to compute host [%s]", image->getID().c_str(),
        //             computeHost.c_str());
    }
}; // namespace wrench
