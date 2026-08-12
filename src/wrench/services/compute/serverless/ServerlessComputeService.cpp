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
#include <wrench/function/Invocation.h>
#include <wrench/services/compute/serverless/Container.h>
#include <wrench/services/helper_services/alarm/Alarm.h>
#include <wrench/function/Function.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/failure_causes/FunctionNotFound.h>

#include <utility>

#include "wrench/action/CustomAction.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/batch/BatchComputeServiceMessage.h"
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
    ServerlessComputeService::ServerlessComputeService(
        const std::string& hostname,
        const std::string& head_node_storage_mount_point,
        const std::vector<std::string>& compute_hosts,
        const std::shared_ptr<ServerlessScheduler>& scheduler,
        const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
        const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE&
        messagepayload_list) :
        ComputeService(hostname,
                       "ServerlessComputeService", "") {
        // Check that there is at least one compute host
        if (compute_hosts.empty()) {
            throw std::invalid_argument(
                "ServerlessComputeService::ServerlessComputeService(): The list of compute hosts cannot be empty");
        }

        // Check that the scheduler is specified
        if (not scheduler) {
            throw std::invalid_argument(
                "ServerlessComputeService::ServerlessComputeService(): A non-null scheduler must be provided");
        }
        // Check platform homogeneity
        check_homogeneity(compute_hosts);

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        // Set default and specified properties
        this->setProperties(this->default_property_values, property_list);

        // Create the state of the system object
        _state_of_the_system = std::shared_ptr<ServerlessStateOfTheSystem>(
            new ServerlessStateOfTheSystem(compute_hosts, this));
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
        _compute_node_num_cores = S4U_Simulation::getHostNumCores(first_hostname);
        _compute_node_core_speed = S4U_Simulation::getHostFlopRate(first_hostname);
        _compute_node_ram = S4U_Simulation::getHostMemoryCapacity(first_hostname);
        try {
            _compute_node_disk_space = S4U_Simulation::getDiskCapacity(first_hostname, "/");
        }
        catch (std::invalid_argument& e) {
            throw std::invalid_argument("Compute hosts for a serverless compute service must have a '/' mountpoint");
        }

        for (auto const& hostname : compute_hosts) {
            auto num_cores_available = S4U_Simulation::getHostNumCores(hostname);
            double speed = S4U_Simulation::getHostFlopRate(hostname);
            sg_size_t ram_available = S4U_Simulation::getHostMemoryCapacity(hostname);
            sg_size_t disk_capacity;
            // sg_size_t disk_read_bandwidth;
            // sg_size_t disk_write_bandwidth;
            try {
                disk_capacity = S4U_Simulation::getDiskCapacity(hostname, "/");
            }
            catch (std::invalid_argument& e) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless compute service must have a '/' mountpoint");
            }

            // Compute speed
            if (std::abs(speed - _compute_node_core_speed) > DBL_EPSILON) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless compute service need to be homogeneous (different flop rates detected)");
            }
            // RAM
            if (ram_available != _compute_node_ram) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless compute service need to be homogeneous (different RAM capacities detected)");
            }
            // Num cores
            if (num_cores_available != _compute_node_num_cores) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless service need to be homogeneous (different number of cores detected)");
            }
            // Disk capacity
            if (disk_capacity != _compute_node_disk_space) {
                throw std::invalid_argument(
                    "Compute hosts for a serverless service need to be homogeneous (different disk capacities detected)");
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
            num_hosts.insert(std::make_pair(this->getName(), _state_of_the_system->_compute_nodes.size()));
            return num_hosts;
        }
        else if (key == "num_cores") {
            // Num cores per hosts
            std::map<std::string, double> num_cores;
            for (auto const& compute_node : _state_of_the_system->_compute_nodes) {
                num_cores[compute_node->hostname] = static_cast<double>(compute_node->_total_cores);
            }
            return num_cores;
        }
        else if (key == "num_idle_cores") {
            // Num idle cores per hosts
            std::map<std::string, double> num_idle_cores;
            for (const auto& compute_node : _state_of_the_system->_compute_nodes) {
                num_idle_cores[compute_node->hostname] = static_cast<double>(compute_node->_available_cores);
            }
            return num_idle_cores;
        }
        else if (key == "flop_rates") {
            // Flop rate per host
            std::map<std::string, double> flop_rates;
            for (const auto& compute_node : _state_of_the_system->_compute_nodes) {
                flop_rates[compute_node->hostname] = S4U_Simulation::getHostFlopRate(compute_node->hostname);
            }
            return flop_rates;
        }
        else if (key == "ram_capacities") {
            // RAM capacity per host
            std::map<std::string, double> ram_capacities;
            for (const auto& compute_node : _state_of_the_system->_compute_nodes) {
                ram_capacities[compute_node->hostname] = static_cast<double>(compute_node->_memory->getTotalSpace());
            }
            return ram_capacities;
        }
        else if (key == "ram_availabilities") {
            // RAM availability per host
            std::map<std::string, double> ram_availability;
            for (auto const& compute_node : _state_of_the_system->_compute_nodes) {
                ram_availability[compute_node->hostname] = static_cast<double>(compute_node->_memory->
                    getTotalFreeSpaceZeroTime());
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
        _commport->putMessage(
            new ServerlessComputeServiceFunctionRegisterRequestMessage(
                answer_commport, function, time_limit_in_seconds, disk_space_limit_in_bytes, RAM_limit_in_bytes,
                ingress_in_bytes, egress_in_bytes,
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        const auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionRegisterAnswerMessage>(
            this->network_timeout,
            "ServerlessComputeService::registerFunction(): Received an");

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
        _commport->dputMessage(
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
        WRENCH_INFO("Serverless provider starting (%s)", _commport->get_cname());

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
                terminateIdleContainers(decisions->container_terminations);
                dispatchInvocations(decisions->invocation_dispatches);
                initiateImageLoads(decisions->image_loads_to_RAM);
                initiateImageCopies(decisions->image_copies_to_disk);
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
            message = _commport->getMessage();
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
            processImageDownloadCompletion(scsdc_msg->_action, scsdc_msg->_image);
            return true;
        }
        else if (const auto scsiec_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceInvocationExecutionCompleteMessage>(message)) {
            processInvocationCompletion(scsiec_msg->_invocation, scsiec_msg->_action);
            return true;
        }
        else if (const auto scsncc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodeCopyCompleteMessage>(message)) {
            scsncc_msg->_compute_node->_images_being_copied.erase(scsncc_msg->_image);
            if (scsncc_msg->_action->getState() != Action::State::COMPLETED) {
                WRENCH_INFO("An image copy has failed (due to disk pressure) for image %s... nevermind",
                            scsncc_msg->_image->getName().c_str());
                do_scheduling = false;
            }
            else {
                WRENCH_INFO("Image %s was stored at compute node %s",
                            scsncc_msg->_image->getName().c_str(), scsncc_msg->_compute_node->hostname.c_str());
                std::cerr << "FREE SPACE REMAINING: " << scsncc_msg->_compute_node->getFreeDiskSpace() << "\n";
            }
            return true;
        }
        else if (const auto scsnlc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodeLoadCompleteMessage>(message)) {
            scsnlc_msg->_compute_node->_images_being_loaded.erase(scsnlc_msg->_image);
            if (scsnlc_msg->_action->getState() != Action::State::COMPLETED) {
                WRENCH_INFO("An image load has failed (due to memory pressure) for image %s... nevermind",
                            scsnlc_msg->_image->getName().c_str());
                do_scheduling = false;
            }
            else {
                WRENCH_INFO("Image %s was loaded at compute node %s",
                            scsnlc_msg->_image->getName().c_str(), scsnlc_msg->_compute_node->hostname.c_str());
            }
            return true;
        }
        else if (const auto sclcit_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceContainerIdleTimeoutMessage>(message)) {
            processContainerIdleTimeout(sclcit_msg->_container, sclcit_msg->_idle_sequence);
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
            sg_size_t needed_disk_space = function->getImageFile()->getSize() + disk_space_limit_in_bytes;
            sg_size_t needed_ram_space = function->getImage()->getRAMFootprint() + ram_limit_in_bytes;

            if (needed_disk_space > _compute_node_disk_space) {
                const auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                    false, nullptr,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(),
                                                 "Function cannot be registered because no compute host has sufficient disk space to execute it"),
                    this->getMessagePayloadValue(
                        ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD));
                answer_commport->dputMessage(answerMessage);
                return;
            }

            if (needed_ram_space > _compute_node_ram) {
                const auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                    false, nullptr,
                    std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(),
                                                 "Function cannot be registered because no compute host has sufficient RAM space to execute it"),
                    this->getMessagePayloadValue(
                        ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD));
                answer_commport->dputMessage(answerMessage);
                return;
            }
        }


        // At this point, we can register the function
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
                true, invocation, nullptr, this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD));
            answer_commport->dputMessage(answerMessage);
        }
    }

    /**
     * @brief Helper method to process an "image download completion" message
     *
     * @param action to get failure cause from
     * @param image The image that was downloaded, used as key to map downloading functions
     */
    void ServerlessComputeService::processImageDownloadCompletion(const std::shared_ptr<Action>& action,
                                                                  const std::shared_ptr<Image>& image) {
        // If the download has failed, fail all corresponding admitted function invocations
        if (action->getFailureCause()) {
            _state_of_the_system->_free_space_on_head_storage += image->getDiskFootprint();
            _state_of_the_system->_being_downloaded_images.erase(image);

            while (not _state_of_the_system->_admitted_invocations[image].empty()) {
                auto invocation = _state_of_the_system->_admitted_invocations[image].front();
                invocation->_notify_commport->dputMessage(
                    new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                        false,
                        invocation,
                        action->getFailureCause(), this->getMessagePayloadValue(
                            ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
                _state_of_the_system->_admitted_invocations[image].pop();
            }
            _state_of_the_system->_admitted_invocations[image] = std::queue<std::shared_ptr<Invocation>>();
            _state_of_the_system->_admitted_invocations.erase(image);
            return;
        }

        WRENCH_INFO("ServerlessComputeService::processImageDownloadCompletion(): Image %s was downloaded",
                    image->getName().c_str());
        _state_of_the_system->_being_downloaded_images.erase(image);
        // _state_of_the_system->_downloaded_image_files.insert(image_file);

        // Move all relevant invocations from the admitted to the schedulable queue
        auto& queue = _state_of_the_system->_admitted_invocations.at(image);
        while (not queue.empty()) {
            _state_of_the_system->_schedulable_invocations.emplace(
                _state_of_the_system->_schedulable_invocations.end(), std::move(queue.front()));
            queue.pop();
        }
        _state_of_the_system->_admitted_invocations.erase(image);
    }

    /**
     * @brief Helper method to process an invocation completion
     * @param invocation The invocation that has completed
     * @param action The action that was responsible for running the invocation
     */
    void ServerlessComputeService::processInvocationCompletion(const std::shared_ptr<Invocation>& invocation,
                                                               const std::shared_ptr<Action>& action) {
        std::shared_ptr<FailureCause> failure_cause = action->getFailureCause();
        WRENCH_INFO("A function invocation for function %s has finished [%s]",
                    invocation->getRegisteredFunction()->getFunction()->getName().c_str(),
                    (failure_cause ? "FAILURE" : "SUCCESS"));

        auto compute_node = invocation->_compute_node;
        // Free up the core
        compute_node->_available_cores++;

        // Make container idle
        auto container = invocation->_container;
        compute_node->makeContainerIdle(container);

        // Should the container be terminated or left idling?
        double idle_timeout = this->getPropertyValueAsDouble(ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT);
        if (idle_timeout <= 0) {
            compute_node->shutdownContainer(container);
        }
        else {
            // Start the timeout alarm
            std::shared_ptr<Alarm> alarm_ptr = Alarm::createAndStartAlarm(
                this->simulation_,
                S4U_Simulation::getClock() + idle_timeout,
                _hostname,
                _commport,
                new
                ServerlessComputeServiceContainerIdleTimeoutMessage(
                    container, container->getIdleSequence()),
                "container_idle_timout");
        }

        bool success = action->getState() == Action::State::COMPLETED;

        _state_of_the_system->_running_invocations.erase(invocation);

        invocation->_notify_commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                success,
                invocation,
                failure_cause, this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Helper method to process a container idle timeout
     * @param container the container that has idle-timed out
     */
    void ServerlessComputeService::processContainerIdleTimeout(const std::shared_ptr<Container>& container,
                                                               std::uint64_t idle_sequence) {
        // If this isn't a stale/no-longer-valid message, nevermind
        if (container->getIdleSequence() != idle_sequence) {
            return;
        }
        // Shutdown the container
        container->getComputeNode()->shutdownContainer(container);
    }

    /**
     * @brief Terminates idle containers if need me
     * @param decisions scheduling decisions
     */
    void ServerlessComputeService::terminateIdleContainers(const std::vector<TerminateContainer>& decisions) {
        for (const auto& [container] : decisions) {
            if (container->isBusy()) {
                throw std::runtime_error(
                    "ServerlessComputeService::terminateIdleContainers(): container to terminate is not idle!");
            }
            // Shutdown the container
            container->getComputeNode()->shutdownContainer(container);
        }
    }

    /**
     * @brief Dispatches scheduled function invocations to compute hosts
     * @param decisions Scheduling decisions
     * @return true if at least one invocation was dispatched
     */
    void ServerlessComputeService::dispatchInvocations(
        const std::vector<DispatchInvocation>& decisions) {
        // Dispatched the invocations in the order of the schedulable list
        std::set<std::shared_ptr<Invocation>> dispatched_invocations;

        // Dispatch invocation
        for (const auto& [invocation, compute_node, container] : decisions) {
            // WRENCH_INFO("Trying to dispatch scheduled invocation for function [%s]...",
            //             invocation_to_place->_registered_function->_function->getName().c_str());
            if (dispatchInvocation(invocation, compute_node, container)) {
                _state_of_the_system->_running_invocations.insert(invocation);
                dispatched_invocations.insert(invocation);
            }
        }

        // Update the list of schedulable invocations (not super efficient, but clearer than the erase-as-I-go)
        std::vector<std::shared_ptr<Invocation>> updated_list_of_schedulable_invocations;
        for (auto const& invocation : _state_of_the_system->_schedulable_invocations) {
            if (dispatched_invocations.find(invocation) == dispatched_invocations.end()) {
                updated_list_of_schedulable_invocations.push_back(invocation);
            }
        }
        _state_of_the_system->_schedulable_invocations = updated_list_of_schedulable_invocations;
    }

    /**
     * @brief Helper method to dispatch an invocation
     *
     * @param invocation invocation to dispatch
     * @param target_compute_node the target compute node
     * @param target_container the target container (if nullptr, create a new container)
     * @return true on success, false on failure
     */
    bool ServerlessComputeService::dispatchInvocation(const std::shared_ptr<Invocation>& invocation,
                                                      ServerlessComputeNode* target_compute_node,
                                                      std::shared_ptr<Container> target_container) {
        // Check that things can work, which may not be the case because scheduling and LRU is complicated
        if (not target_compute_node->isInvocationFeasible(invocation, target_container)) {
            return false;
        }

        bool hot_start = target_container != nullptr;

        if (not hot_start) {
            // Spawn the container
            try {
                target_container = target_compute_node->spawnContainer(invocation->getRegisteredFunction().get());
            }
            catch (ExecutionException& e) {
                WRENCH_INFO("Couldn't spawn a container for an invocation for %s: %s",
                            invocation->_registered_function->_function->getName().c_str(),
                            e.getCause()->toString().c_str());
                return false;
            }
        }

        // Bookkeeping
        invocation->_container = target_container;
        invocation->_compute_node = target_compute_node;
        invocation->_dispatch_date = Simulation::getCurrentSimulatedDate();
        invocation->_dispatched = true;

        // Update the core count of the compute node
        target_compute_node->_available_cores -= 1;

        // Make the container busy if it was idling
        if (hot_start) {
            target_compute_node->makeContainerBusy(target_container);
        }

        // Declare the function invocation's necessary lambdas
        const std::function lambda_terminate = [invocation](const std::shared_ptr<ActionExecutor>& action_executor) {
            // Clear up all files from the tmp storage service, if any, so that if a next
            // invocation reuses this container, it won't "see" the previous invocation's data, if any
            invocation->_container->clearPrivateStorage();
        };

        const std::function lambda_execute = [invocation](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            const auto function = invocation->_registered_function->_function;

            // Invoke the user's lambda function
            invocation->_function_start_date = S4U_Simulation::getClock();
            try {
                invocation->_function_output = function->execute(invocation->_function_input,
                                                                 invocation->_container->getPrivateStorageService());
            }
            catch (ExecutionException& e) {
                invocation->_container->clearPrivateStorage();
                invocation->_function_end_date = S4U_Simulation::getClock();
                throw;
            }
            invocation->_function_end_date = S4U_Simulation::getClock();
        };

        // Create the action and create a corresponding action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "run_invocation_" + invocation->_registered_function->_function->getName(),
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceInvocationExecutionCompleteMessage(
            action,
            invocation, 0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            target_compute_node->hostname,
            1,
            0,
            (hot_start
                 ? 0
                 : this->getPropertyValueAsDouble(ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD)),
            false,
            _commport,
            custom_message,
            action,
            nullptr);

        action_executor->setActionTimeout(invocation->getRegisteredFunction()->getTimeLimit());
        action_executor->setSimulation(this->simulation_);

        WRENCH_INFO("Dispatched an invocation for function %s",
                    invocation->getRegisteredFunction()->getFunction()->getName().c_str());

        // Start the action executor object
        try {
            action_executor->start(action_executor, true, false);
        }
        catch (ExecutionException& e) {
            throw std::runtime_error(
                "ServerlessComputeService::dispatchInvocation(): Action executor could not be started - internal error");
        }

        return true;
    }

    /**
     * @brief Start a SimpleStorageService for each compute host. We don't start a bare-metal
     *        service as we'll do everything ourselves with action executor services.
     */
    void ServerlessComputeService::startComputeHostsServices() {
        for (auto const& compute_node : _state_of_the_system->_compute_nodes) {
            if (not S4U_Simulation::hostHasMountPoint(compute_node->hostname, "/")) {
                throw std::invalid_argument("ServerlessComputeService::startComputeHostsServices(): "
                    "each compute host in a serverless compute service should have a \"/\" mountpoint");
            }

            // Start a compute service, with LRU caching, to implement compute-node storage
            {
                const auto ss = std::dynamic_pointer_cast<SimpleStorageService>(this->simulation_->startNewService(
                    SimpleStorageService::createSimpleStorageService(
                        compute_node->hostname,
                        {"/"},
                        {
                            {SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"},
                            {
                                SimpleStorageServiceProperty::BUFFER_SIZE,
                                this->getPropertyValueAsString(
                                    ServerlessComputeServiceProperty::STORAGE_SERVICES_BUFFER_SIZE)
                            }
                        },
                        {})));
                ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
                compute_node->_disk = ss;
            }

            // Start a compute service, with LRU caching, to implement compute-node memory
            {
                // Create a RAM disk on the host
                auto host = simgrid::s4u::Engine::get_instance()->host_by_name(compute_node->hostname);
                auto ram_disk = host->add_disk(
                    "ram_disk_" + std::to_string(ServerlessComputeService::sequence_number++),
                    S4U_Simulation::RAM_READ_BANDWIDTH,
                    S4U_Simulation::RAM_WRITE_BANDWIDTH);
                auto ram_capacity = S4U_Simulation::getHostMemoryCapacity(compute_node->hostname);
                std::string ram_mount_point = "/" + ram_disk->get_name();
                ram_disk->set_property("size", std::to_string(ram_capacity) + "B");
                ram_disk->set_property("mount", ram_mount_point);

                const auto ss = std::dynamic_pointer_cast<SimpleStorageService>(this->simulation_->startNewService(
                    SimpleStorageService::createSimpleStorageService(
                        compute_node->hostname,
                        {ram_mount_point},
                        {
                            {SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"},
                            {
                                SimpleStorageServiceProperty::BUFFER_SIZE,
                                this->getPropertyValueAsString(
                                    ServerlessComputeServiceProperty::STORAGE_SERVICES_BUFFER_SIZE)
                            }
                        },
                        {})));
                ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
                compute_node->_memory = ss;
            }
        }
    }


    /**
     * @brief Helper method to start the storage service on the head node
     *
     */
    void ServerlessComputeService::startHeadStorageService() {
        const auto ss = SimpleStorageService::createSimpleStorageService(
            _hostname,
            {_state_of_the_system->_head_storage_service_mount_point},
            {
                {
                    SimpleStorageServiceProperty::BUFFER_SIZE,
                    this->getPropertyValueAsString(ServerlessComputeServiceProperty::STORAGE_SERVICES_BUFFER_SIZE)
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
            const auto image = invocation->_registered_function->_function->getImage();

            // If the image file is already downloaded, make the invocation schedulable immediately
            if (_state_of_the_system->_head_storage_service->hasFile(image->getFile())) {
                _state_of_the_system->_new_invocations.pop();
                _state_of_the_system->_schedulable_invocations.emplace_back(invocation); // Insert at the end
                continue;
            }

            // If the image file is being downloaded, make the invocation admitted
            if (_state_of_the_system->_being_downloaded_images.find(image) != _state_of_the_system->
                                                                              _being_downloaded_images.end()) {
                _state_of_the_system->_new_invocations.pop();
                _state_of_the_system->_admitted_invocations[image].push(invocation);
                continue;
            }

            // Otherwise, if there is enough space on the head node storage service to store it,
            // then launch the downloaded and admit the invocation
            if (_state_of_the_system->_free_space_on_head_storage >= image->getDiskFootprint()) {
                // "Reserve" space on the storage service
                _state_of_the_system->_free_space_on_head_storage -= image->getDiskFootprint();
                // initiate the download
                _state_of_the_system->_being_downloaded_images.insert(image);
                initiateImageDownloadFromRemote(invocation);
                _state_of_the_system->_new_invocations.pop();
                _state_of_the_system->_admitted_invocations[image].push(invocation);
                continue;
            }
            else {
                throw std::runtime_error("ServerlessComputeService::admitInvocations(): "
                    "An invocation cannot be admitted because there is not enough space on the head storage to download its (remote) image. "
                    "Currently, no mechanism is implemented to manage the head storage (just make it bigger).");
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
        const std::function lambda_execute = [invocation, this
            ](const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the lambda execute!!");
            const auto src_location = invocation->_registered_function->_function->getImage()->getLocation();
            const auto dst_location = FileLocation::LOCATION(_state_of_the_system->_head_storage_service,
                                                             src_location->getFile());
            StorageService::copyFile(src_location, dst_location);
        };
        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "download_image_" + invocation->_registered_function->_function->getImageFile()->getID(),
                0, 0, lambda_execute, lambda_terminate));

        // Spin up an ActionExecutor service, and have it send us back a custom message
        auto custom_message = new ServerlessComputeServiceDownloadCompleteMessage(
            action,
            invocation->_registered_function->_function->getImage(), 0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            this->getHostname(),
            0,
            0,
            0,
            false,
            _commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        // WRENCH_INFO("Starting an action executor for downloading from remote...");
        try {
            action_executor->start(action_executor, true, false); // Daemonized, no auto-restart
        }
        catch (const std::exception& e) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageDownloadFromRemote(): Action executor could not be started - internal error");
        }
    }

    /**
     * @brief Helper method to invoke the scheduler
     * @return the scheduler's scheduling decisions
     */
    std::shared_ptr<ServerlessSchedulingDecisions> ServerlessComputeService::invokeScheduler() const {
        // Invoke the scheduler so that it manages images
        auto decisions = _scheduler->schedule(_state_of_the_system->_schedulable_invocations,
                                              _state_of_the_system.get());
#if 0
        decisions->print();
#endif
        return decisions;
    }

    /**
     * @brief Helper method to initiate image loads
     * @param decisions scheduling decisions
     */
    void ServerlessComputeService::initiateImageLoads(const std::vector<LoadImage>& decisions) {
        // For each compute node, load initiate image load from disk into RAM and if space
        for (const auto& [image, compute_node] : decisions) {
            initiateImageLoadAtComputeNode(compute_node, image);
        }
    }

    /**
    * @brief Helper method to initiate image copies
    * @param decisions scheduling decisions
    */
    void ServerlessComputeService::initiateImageCopies(
        const std::vector<CopyImage>& decisions) {
        // For each compute node, initiate image copy (from head node) if need be
        for (const auto& [image_file, compute_node] : decisions) {
            initiateImageCopyToComputeNode(compute_node, image_file);
        }
    }

    /**
     * @brief Method to initiate an image copy from the head host to a compute host
     * @param compute_node The compute node
     * @param image The image
     */
    void ServerlessComputeService::initiateImageCopyToComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& compute_node,
        const std::shared_ptr<Image>& image) {
        // Sanity checks
        if (compute_node->isImageOnDisk(image)) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageLoadAtComputeNode(): Being told to copy image " +
                image->getName() + " to node " + compute_node->hostname + ", but the image is already on disk!");
        }

        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        const std::function lambda_execute = [compute_node, image, this](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the image copy lambda execute!!");

            // Copy the image file from the head host to the current host's storage service
            auto head_host_image_path = FileLocation::LOCATION(_state_of_the_system->_head_storage_service,
                                                               image->getFile());
            auto local_image_path = wrench::FileLocation::LOCATION(compute_node->_disk, image->getFile());
            StorageService::copyFile(head_host_image_path, local_image_path);
            // WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "copy_image_" + image->getName() + "_to_" + compute_node->hostname,
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceNodeCopyCompleteMessage(
            action,
            image,
            compute_node,
            0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            compute_node->hostname,
            1,
            0,
            0,
            false,
            _commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        // WRENCH_INFO("Starting an action executor for copying image...");
        try {
            action_executor->start(action_executor, true, false);
        }
        catch (const std::exception& e) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageCopyToComputeNode(): Action executor could not be started - internal error");
        }

        // Add the image to the being_copied_images data structure for this host
        compute_node->_images_being_copied.insert(image);

        WRENCH_INFO("Initiated image copy for image [%s] from head host to compute host [%s]", image->getName().c_str(),
                    compute_node->hostname.c_str());
    }

    /**
     * @brief Method to initiate an image load from disk to RAM at a compute host
     * @param compute_node The compute node
     * @param image The image
     */
    void ServerlessComputeService::initiateImageLoadAtComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& compute_node,
        const std::shared_ptr<Image>& image) {
        unsigned long sequence_number = 0;

        // Sanity check
        if (compute_node->isImageInRAM(image)) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageLoadAtComputeNode(): Being told to load image " +
                image->getName() + " at node " + compute_node->hostname + ", but the image is already in RAM!");
        }

        // Initiate an asynchronous action that simply read the image file from disk
        const std::function lambda_terminate = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };

        const std::function lambda_execute = [compute_node, image](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the image load lambda execute!!");
            auto src_location = wrench::FileLocation::LOCATION(
                compute_node->_disk, image->getFile());
            auto dst_location = wrench::FileLocation::LOCATION(
                compute_node->_memory, compute_node->_memory->getBaseRootPath(),
                image->getRAMFile());
            // Read the RAM portion only
            // Create and open a tmp file to "reserve" space
            auto tmp_file = Simulation::addTmpFile(image->getRAMFootprint());
            auto tmp_file_location = wrench::FileLocation::LOCATION(compute_node->_memory, tmp_file);
            try {
                compute_node->_memory->createFileAtLocation(tmp_file_location);
                auto open_tmp_file = compute_node->_memory->openFile(tmp_file_location);
                // Simulate the read of the RAM portion of the image on disk
                compute_node->_disk->readFile(src_location, image->getRAMFootprint());
                // Close and remove the tmp file
                open_tmp_file->close();
                compute_node->_memory->removeFileAtLocation(tmp_file_location);
                Simulation::removeFile(tmp_file);
                // Create the RAM file in its place
                compute_node->_memory->createFile(image->getRAMFile());
            }
            catch (ExecutionException& e) {
                Simulation::removeFile(tmp_file);
                throw;
            }
            // WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "load_image_" + image->getName() + "_at_" + compute_node->hostname,
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceNodeLoadCompleteMessage(
            action,
            image,
            compute_node,
            0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            compute_node->hostname,
            1,
            0,
            0,
            false,
            _commport,
            custom_message,
            action,
            nullptr);

        action_executor->setSimulation(this->simulation_);
        // WRENCH_INFO("Starting an action executor for copying image...");
        try {
            action_executor->start(action_executor, true, false);
        }
        catch (const std::exception& e) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageLoadAtComputeNode(): Action executor could not be started - internal error");
        }

        // Add the image to the being_copied_images data structure for this host
        compute_node->_images_being_loaded.insert(image);

        WRENCH_INFO("Initiated image load for image [%s] at compute host [%s]", image->getName().c_str(),
                    compute_node->hostname.c_str());
    }
}; // namespace wrench
