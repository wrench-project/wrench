/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <utility>
#include <cmath>

#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/services/compute/serverless/ServerlessComputeServiceMessage.h>
#include <wrench/services/compute/serverless/ServerlessComputeServiceMessagePayload.h>
#include <wrench/function/Invocation.h>
#include <wrench/services/compute/serverless/Container.h>
#include <wrench/services/helper_services/alarm/Alarm.h>
#include <wrench/function/Function.h>
#include <wrench/function/Image.h>
#include <wrench/function/ImageLayer.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/failure_causes/FunctionNotFound.h>
#include <wrench/action/CustomAction.h>
#include <wrench/failure_causes/NotEnoughResources.h>
#include <wrench/failure_causes/ServiceIsDown.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/compute/batch/BatchComputeServiceMessage.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/services/storage/simple/SimpleStorageService.h>
#include <wrench/simulation/Simulation.h>

WRENCH_LOG_CATEGORY(wrench_core_serverless_service, "Log category for Serverless Compute Service");

namespace wrench {
    unsigned long ServerlessComputeService::_sequence_number = 0;

    /**
     * @brief A no-op lambda
     */
    const std::function lambda_noop = [](const std::shared_ptr<ActionExecutor>& action_executor) {
    };

    /**
     * @brief Constructor
     *
     * @param hostname name of the head host on which the service runs
     * @param head_node_storage_mount_point the mount point of storage at the head host (where images will be stored)
     * @param compute_nodes list of (homogeneous) compute node hostnames
     * @param scheduler the scheduler used to decide which invocations should be executed and when
     * @param property_list: a property list ({} means "use all defaults")
     * @param messagepayload_list: a message payload list ({} means "use all defaults")
     */
    ServerlessComputeService::ServerlessComputeService(
        const std::string& hostname,
        const std::string& head_node_storage_mount_point,
        const std::vector<std::string>& compute_nodes,
        const std::shared_ptr<ServerlessScheduler>& scheduler,
        const WRENCH_PROPERTY_COLLECTION_TYPE& property_list,
        const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE&
        messagepayload_list) :
        ComputeService(hostname,
                       "ServerlessComputeService", "") {
        // Check that there is at least one compute host
        if (compute_nodes.empty()) {
            throw std::invalid_argument(
                "ServerlessComputeService::ServerlessComputeService(): The list of compute hosts cannot be empty");
        }

        // Check that the scheduler is specified
        if (not scheduler) {
            throw std::invalid_argument(
                "ServerlessComputeService::ServerlessComputeService(): A non-null scheduler must be provided");
        }

        // Set some convenient node-spec values and heck platform homogeneity
        auto first_hostname = *(compute_nodes.begin());
        _compute_node_num_cores = S4U_Simulation::getHostNumCores(first_hostname);
        _compute_node_core_speed = S4U_Simulation::getHostFlopRate(first_hostname);
        _compute_node_ram = S4U_Simulation::getHostMemoryCapacity(first_hostname);
        try {
            _compute_node_disk_space = S4U_Simulation::getDiskCapacity(first_hostname, "/");
        } catch (std::invalid_argument&) {
            throw std::invalid_argument("Compute nodes for a serverless compute service must have a '/' mountpoint");
        }
        check_homogeneity(compute_nodes);

        // Set default and specified message payloads
        this->setMessagePayloads(this->default_messagepayload_values, messagepayload_list);

        // Set default and specified properties
        this->setProperties(this->default_property_values, property_list);

        // Deal with the 0-value defaults
        if (this->getPropertyValueAsUnsignedLong(
                ServerlessComputeServiceProperty::MAX_NUM_CONCURRENT_INVOCATIONS_PER_NODE)
            == 0) {
            this->setProperty(ServerlessComputeServiceProperty::MAX_NUM_CONCURRENT_INVOCATIONS_PER_NODE,
                              std::to_string(_compute_node_num_cores));
        }
        if (this->getPropertyValueAsUnsignedLong(ServerlessComputeServiceProperty::MAX_NUM_CONTAINERS_PER_NODE)
            == 0) {
            this->setProperty(ServerlessComputeServiceProperty::MAX_NUM_CONTAINERS_PER_NODE,
                              std::to_string(_compute_node_num_cores));
        }

        // Validate MAX_NUM_CONCURRENT_INVOCATIONS_PER_CONTAINER
        if (this->getPropertyValueAsUnsignedLong(ServerlessComputeServiceProperty::MAX_NUM_CONCURRENT_INVOCATIONS_PER_CONTAINER) == 0) {
            throw std::invalid_argument("The ServerlessComputeServiceProperty::MAX_NUM_CONCURRENT_INVOCATIONS_PER_CONTAINER property value must be >0");
        }


        // Create the state of the system object
        _state_of_the_system = std::shared_ptr<ServerlessStateOfTheSystem>(
            new ServerlessStateOfTheSystem(compute_nodes, this));
        _state_of_the_system->_head_storage_service_mount_point = head_node_storage_mount_point;

        _scheduler = scheduler;
    }

    /**
     * @brief Helper method to check homogeneity of the compute nodes
     * @param compute_nodes a list of compute node hostnames
     */
    void ServerlessComputeService::check_homogeneity(const std::vector<std::string>& compute_nodes) const {
        for (auto const& hostname : compute_nodes) {
            auto num_cores_available = S4U_Simulation::getHostNumCores(hostname);
            double speed = S4U_Simulation::getHostFlopRate(hostname);
            sg_size_t ram_available = S4U_Simulation::getHostMemoryCapacity(hostname);
            sg_size_t disk_capacity;
            // sg_size_t disk_read_bandwidth;
            // sg_size_t disk_write_bandwidth;
            try {
                disk_capacity = S4U_Simulation::getDiskCapacity(hostname, "/");
            } catch (std::invalid_argument&) {
                throw std::invalid_argument(
                    "Compute nodes for a serverless compute service must have a '/' mountpoint");
            }

            // Compute speed
            if (std::abs(speed - _compute_node_core_speed) > DBL_EPSILON) {
                throw std::invalid_argument(
                    "Compute nodes for a serverless compute service need to be homogeneous (different flop rates detected)");
            }
            // RAM
            if (ram_available != _compute_node_ram) {
                throw std::invalid_argument(
                    "Compute nodes for a serverless compute service need to be homogeneous (different RAM capacities detected)");
            }
            // Num cores
            if (num_cores_available != _compute_node_num_cores) {
                throw std::invalid_argument(
                    "Compute nodes for a serverless service need to be homogeneous (different number of cores detected)");
            }
            // Disk capacity
            if (disk_capacity != _compute_node_disk_space) {
                throw std::invalid_argument(
                    "Compute nodes for a serverless service need to be homogeneous (different disk capacities detected)");
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
     * @brief Get the service's scheduler
     * @return a scheduler
     */
    std::shared_ptr<ServerlessScheduler> ServerlessComputeService::getScheduler() const {
        return _scheduler;
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
            num_hosts.insert(std::make_pair(this->getName(),
                                            _state_of_the_system->_activated_compute_nodes.size() +
                                            _state_of_the_system->_being_activated_compute_nodes.size() +
                                            _state_of_the_system->_deactivated_compute_nodes.size()));
            return num_hosts;
        } else if (key == "num_cores") {
            // Num cores per host
            std::map<std::string, double> num_cores;
            for (auto const& compute_node : _state_of_the_system->_activated_compute_nodes) {
                num_cores[compute_node->hostname] = static_cast<double>(_compute_node_num_cores);
            }
            for (auto const& compute_node : _state_of_the_system->_being_activated_compute_nodes) {
                num_cores[compute_node->hostname] = static_cast<double>(_compute_node_num_cores);
            }
            for (auto const& compute_node : _state_of_the_system->_deactivated_compute_nodes) {
                num_cores[compute_node->hostname] = static_cast<double>(_compute_node_num_cores);
            }
            return num_cores;
        } else if (key == "num_idle_cores") {
            // Num idle cores per host
            std::map<std::string, double> num_idle_cores;
            for (const auto& compute_node : _state_of_the_system->_activated_compute_nodes) {
                auto num_in_flight_invocations = 0UL;
                for (auto const& container : compute_node->_initializing_containers) {
                    num_in_flight_invocations += container->_in_flight_invocations.size();
                }
                for (auto const& container : compute_node->_initialized_containers) {
                    num_in_flight_invocations += container->_in_flight_invocations.size();
                }
                unsigned long idle_cores = (num_in_flight_invocations < _compute_node_num_cores
                                                ? _compute_node_num_cores - num_in_flight_invocations
                                                : 0);
                num_idle_cores[compute_node->hostname] = static_cast<double>(idle_cores);
            }
            for (const auto& compute_node : _state_of_the_system->_being_activated_compute_nodes) {
                num_idle_cores[compute_node->hostname] = static_cast<double>(_compute_node_num_cores);
            }
            for (const auto& compute_node : _state_of_the_system->_deactivated_compute_nodes) {
                num_idle_cores[compute_node->hostname] = static_cast<double>(_compute_node_num_cores);
            }
            return num_idle_cores;
        } else if (key == "flop_rates") {
            // Flop rate per host
            std::map<std::string, double> flop_rates;
            for (const auto& compute_node : _state_of_the_system->_activated_compute_nodes) {
                flop_rates[compute_node->hostname] = S4U_Simulation::getHostFlopRate(compute_node->hostname);
            }
            for (const auto& compute_node : _state_of_the_system->_being_activated_compute_nodes) {
                flop_rates[compute_node->hostname] = S4U_Simulation::getHostFlopRate(compute_node->hostname);
            }
            for (const auto& compute_node : _state_of_the_system->_deactivated_compute_nodes) {
                flop_rates[compute_node->hostname] = S4U_Simulation::getHostFlopRate(compute_node->hostname);
            }
            return flop_rates;
        } else if (key == "ram_capacities") {
            // RAM capacity per host
            std::map<std::string, double> ram_capacities;
            for (const auto& compute_node : _state_of_the_system->_activated_compute_nodes) {
                ram_capacities[compute_node->hostname] = static_cast<double>(compute_node->_memory->getTotalSpace());
            }
            for (const auto& compute_node : _state_of_the_system->_being_activated_compute_nodes) {
                ram_capacities[compute_node->hostname] = static_cast<double>(compute_node->_memory->getTotalSpace());
            }
            for (const auto& compute_node : _state_of_the_system->_deactivated_compute_nodes) {
                ram_capacities[compute_node->hostname] = static_cast<double>(compute_node->_memory->getTotalSpace());
            }
            return ram_capacities;
        } else if (key == "ram_availabilities") {
            // RAM availability per host
            std::map<std::string, double> ram_availability;
            for (auto const& compute_node : _state_of_the_system->_activated_compute_nodes) {
                ram_availability[compute_node->hostname] = static_cast<double>(compute_node->_memory->
                    getTotalFreeSpaceZeroTime());
            }
            for (auto const& compute_node : _state_of_the_system->_being_activated_compute_nodes) {
                ram_availability[compute_node->hostname] = static_cast<double>(_compute_node_ram);
            }
            for (auto const& compute_node : _state_of_the_system->_deactivated_compute_nodes) {
                ram_availability[compute_node->hostname] = static_cast<double>(_compute_node_ram);
            }
            return ram_availability;
        } else {
            throw std::runtime_error("ServerlessComputeService::getResourceInformation(): unknown key");
        }
    }

    /**
     * @brief Register a function in the serverless compute service
     *
     * @param name the function's name
    * @param initialization_code the function's initialization code, which is executed only once on each container
     *        for the function, before any actual function executions can take place.
     * @param code the function's code, which can only be executed after the initialization code has executed (successfully) on the container.
     * @param image the function's image
     * @param time_limit_in_seconds the time limit for execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param RAM_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit (ignored for now)
     * @param egress_in_bytes the egress data limit (ignored for now)
     * @return A Function object
     * @throw ExecutionException if the function registration fails
     */
    std::shared_ptr<Function> ServerlessComputeService::registerFunction(
        const std::string& name,
        const std::function<void(const std::shared_ptr<StorageService>&)>& initialization_code,
        const std::function<std::shared_ptr<FunctionOutput>(
            const std::shared_ptr<FunctionInput>&,
            const std::shared_ptr<StorageService>&)>& code,
        const std::shared_ptr<Image>& image,
        const double time_limit_in_seconds,
        const sg_size_t disk_space_limit_in_bytes,
        const sg_size_t RAM_limit_in_bytes,
        const sg_size_t ingress_in_bytes,
        const sg_size_t egress_in_bytes) const {
        const auto answer_commport = S4U_CommPort::getTemporaryCommPort();

        _commport->putMessage(
            new ServerlessComputeServiceFunctionRegisterRequestMessage(
                answer_commport, name, initialization_code, code, image, time_limit_in_seconds,
                disk_space_limit_in_bytes,
                RAM_limit_in_bytes,
                ingress_in_bytes, egress_in_bytes,
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        const auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionRegisterAnswerMessage>(
            this->network_timeout, "ServerlessComputeService::registerFunction(): Received an");
        S4U_CommPort::retireTemporaryCommPort(answer_commport);

        if (not msg->success) {
            throw ExecutionException(msg->failure_cause);
        }
        return msg->function;
    }

    /**
     * @brief Invoke a function in the serverless compute service
     *
     * @param function the function to invoke
     * @param input the input to the function
     * @param notify_commport the ExecutionController commport to notify
     * @return std::shared_ptr<Invocation> Pointer to the invocation created by the ServerlessComputeService
     */
    std::shared_ptr<Invocation> ServerlessComputeService::invokeFunction(
        const std::shared_ptr<Function>& function, const std::shared_ptr<FunctionInput>& input,
        S4U_CommPort* notify_commport) const {
        const auto answer_commport = S4U_CommPort::getTemporaryCommPort();
        _commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationRequestMessage(answer_commport,
                                                                         function, input,
                                                                         notify_commport, this->getMessagePayloadValue(
                                                                             ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_REQUEST_MESSAGE_PAYLOAD)));

        // Get the answer
        const auto msg = answer_commport->getMessage<ServerlessComputeServiceFunctionInvocationAnswerMessage>(
            this->network_timeout, "ServerlessComputeService::invokeFunction(): Received an");
        S4U_CommPort::retireTemporaryCommPort(answer_commport);

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

        // Start a storage service on each compute node.
        startComputeNodeServices();

        bool do_scheduling = true;
        do {
            // At each activated compute node, if an image layer is in RAM but not on Disk, due to what we did in the previous
            // scheduling decisions, remove it from RAM (to be realistic).
            // This is a hack, but, as of now, there is no way to "tie" two files together. This
            // should never happen unless the scheduler is buggy, but better safe than sorry
            for (auto const& node : _state_of_the_system->_activated_compute_nodes) {
                for (auto const& rf : _state_of_the_system->_functions) {
                    auto image = rf->getImage();
                    for (auto const& layer : image->getLayers()) {
                        if (node->isImageLayerInRAM(layer) and (not node->isImageLayerOnDisk(layer))) {
                            StorageService::removeFileAtLocation(
                                FileLocation::LOCATION(node->_memory, layer->getRAMFile()));
                            _state_of_the_system->_image_layers_in_ram.at(node).erase(layer);
                        }
                    }
                }
            }

            // Admit invocations whose images have or are being downloaded
            admitInvocations();

            if (do_scheduling) {
                // Invoke the scheduler
                auto decisions = invokeScheduler();

                // Implement the scheduler's decisions, if possible. Note that
                // some decision could perhaps be invalid, and that's ok. The idea is that
                // even a super naive scheduler can be used, even though most of its scheduling
                // decisions may not feasible at a given time. It will just be invoked again later.

                // Activate compute nodes, if any
                activateComputeNodes(decisions->compute_node_activations);
                // Deactivate compute nodes, if any
                deactivateComputeNodes(decisions->compute_node_deactivations);
                // Fail invocations if any
                failInvocations(decisions->invocation_fails);
                // Terminate idle containers if any
                terminateIdleContainers(decisions->idle_container_terminations);
                // Evict layers from RAM if any
                evictLayersFromRAM(decisions->layer_evictions_from_ram);
                // Evict layers from disk if any
                evictLayersFromDisk(decisions->layer_evictions_from_disk);
                // Do invocations if any
                dispatchInvocations(decisions->invocation_dispatches);
                // Do the image loads if any
                initiateImageLayerLoads(decisions->image_layer_loads_to_RAM);
                // Do the image copies if any
                initiateImageLayerCopies(decisions->image_layer_copies_to_disk);
                // Prewarm containers if any
                prewarmContainers(decisions->container_prewarms);

                // Setup scheduler wake-up, if any
                if (decisions->scheduler_wakeup_date.has_value()) {
                    setNextSchedulerWakeup(decisions->scheduler_wakeup_date.value());
                }
                do_scheduling = false;
            }
        } while (processNextMessage(do_scheduling));

        return 0;
    }

    void ServerlessComputeService::setNextSchedulerWakeup(double wakeup_date) {
        const double now = S4U_Simulation::getClock();

        // Ignore wakeup_date in the past
        if (!std::isfinite(wakeup_date) || wakeup_date <= now) {
            WRENCH_INFO("Ignoring serverless scheduler wakeup date in the past");
            return;
        }
        // If previous wakeup exists and will occur sooner, do nothing
        if (_next_scheduler_wakeup_date.has_value() and _next_scheduler_wakeup_date.value() <= wakeup_date) {
            WRENCH_INFO("Ignoring serverless scheduler wakeup date later than the current wakeup date");
            return;
        }

        // Kill any previous wakeup
        if (_next_scheduler_wakeup_date.has_value()) {
            _scheduler_wakeup_alarm->kill();
            _scheduler_wakeup_alarm = nullptr;
            _next_scheduler_wakeup_date.reset();
        }

        // Set up wakeup
        _scheduler_wakeup_alarm = Alarm::createAndStartAlarm(this->simulation_, wakeup_date,
                                                             _hostname, _commport,
                                                             new ServerlessComputeServiceSchedulerWakeupMessage(),
                                                             "scheduler_wakeup");
        _next_scheduler_wakeup_date = wakeup_date;
    }

    /**
     * @brief Process the next message in the commport
     *
     * @return true if the ServerlessComputeService daemon should continue processing messages
     * @return false if the ServerlessComputeService daemon should die
     */
    bool ServerlessComputeService::processNextMessage(bool& do_scheduling) {
        S4U_Simulation::computeZeroFlop();

        // By default, set do_scheduling to true so that a new
        // scheduling round will be triggered
        do_scheduling = true;

        // Wait for a message
        std::shared_ptr<SimulationMessage> message;
        try {
            message = _commport->getMessage();
        } catch (ExecutionException&) {
            WRENCH_INFO("Got a network error while getting some message... ignoring");
            return true;
        }

        WRENCH_DEBUG("Got a [%s] message", message->getName().c_str());

        if (const auto ss_msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            this->state = Service::DOWN;
            terminateAllServicesAndReleaseAllResources();

            // Send back an ack
            if (ss_msg->ack_commport) {
                ss_msg->ack_commport->putMessage(
                    new ServiceDaemonStoppedMessage(
                        getMessagePayloadValue(
                            ServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
            }
            return false;
        } else if (const auto scsfrr_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionRegisterRequestMessage>(message)) {
            processFunctionRegistrationRequest(
                scsfrr_msg->answer_commport, scsfrr_msg->name,
                scsfrr_msg->initialization_code, scsfrr_msg->code, scsfrr_msg->image,
                scsfrr_msg->time_limit_in_seconds,
                scsfrr_msg->disk_space_limit_in_bytes, scsfrr_msg->ram_limit_in_bytes,
                scsfrr_msg->ingress_in_bytes, scsfrr_msg->egress_in_bytes);
            return true;
        } else if (const auto scsfir_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceFunctionInvocationRequestMessage>(message)) {
            processFunctionInvocationRequest(scsfir_msg->answer_commport, scsfir_msg->function,
                                             scsfir_msg->function_input, scsfir_msg->notify_commport);
            return true;
        } else if (const auto scsdc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceDownloadCompleteMessage>(message)) {
            processImageLayerDownloadCompletion(scsdc_msg->_action, scsdc_msg->_layer);
            return true;
        } else if (const auto scsiec_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceInvocationExecutionCompleteMessage>(message)) {
            processInvocationCompletion(scsiec_msg->_invocation, scsiec_msg->_action);
            return true;
            // } else if (const auto scscpc_msg = std::dynamic_pointer_cast<
            //     ServerlessComputeServiceContainerPrewarmCompleteMessage>(message)) {
            //     processPrewarmCompletion(scscpc_msg->_container, scscpc_msg->_action);
            //     return true;
        } else if (const auto scsncc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodeCopyCompleteMessage>(message)) {
            processNodeCopyCompletion(scsncc_msg->_layer, scsncc_msg->_compute_node, scsncc_msg->_action);
            do_scheduling = true;
            return true;
        } else if (const auto scsnlc_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodeLoadCompleteMessage>(message)) {
            processNodeLoadCompletion(scsnlc_msg->_layer, scsnlc_msg->_compute_node, scsnlc_msg->_action);
            do_scheduling = true;
            return true;
        } else if (const auto sclcit_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceContainerIdleTimeoutMessage>(message)) {
            processContainerIdleTimeout(sclcit_msg->_container, sclcit_msg->_idle_sequence);
            return true;
        } else if (const auto csrir_msg =
            std::dynamic_pointer_cast<ComputeServiceResourceInformationRequestMessage>(message)) {
            csrir_msg->answer_commport->dputMessage(
                new ComputeServiceResourceInformationAnswerMessage(
                    constructResourceInformation(csrir_msg->key),
                    getMessagePayloadValue(
                        ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_ANSWER_MESSAGE_PAYLOAD)));
            // This request only observes resources; it need not trigger scheduling.
            do_scheduling = false;
            return true;
        } else if (const auto csitalohwarr_msg =
            std::dynamic_pointer_cast<
                ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage>(
                message)) {
            bool has_available_resources = false;

            // Look at activated compute nodes first
            for (const auto& node : _state_of_the_system->_activated_compute_nodes) {
                auto num_inflight_invocations = 0UL;
                for (auto const& container : node->_initializing_containers) {
                    num_inflight_invocations += container->_in_flight_invocations.size();
                }
                for (auto const& container : node->_initialized_containers) {
                    num_inflight_invocations += container->_in_flight_invocations.size();
                }
                unsigned long num_idle_cores = (num_inflight_invocations < _compute_node_num_cores
                                                    ? _compute_node_num_cores - num_inflight_invocations
                                                    : 0);
                if (num_idle_cores >= csitalohwarr_msg->num_cores &&
                    node->_memory->getTotalFreeSpaceZeroTime() >= csitalohwarr_msg->ram) {
                    has_available_resources = true;
                    break;
                }
            }
            // Look at being-activated compute nodes if need be (as they could become activated)
            if (not has_available_resources) {
                if ((not _state_of_the_system->_being_activated_compute_nodes.empty()) and
                    (_compute_node_num_cores >= csitalohwarr_msg->num_cores) and
                    (_compute_node_ram >= csitalohwarr_msg->ram)) {
                    has_available_resources = true;
                }
            }
            // Look at de-activated compute nodes if need be (as they could become activated)
            if (not has_available_resources) {
                if ((not _state_of_the_system->_deactivated_compute_nodes.empty()) and
                    (_compute_node_num_cores >= csitalohwarr_msg->num_cores) and
                    (_compute_node_ram >= csitalohwarr_msg->ram)) {
                    has_available_resources = true;
                }
            }

            csitalohwarr_msg->answer_commport->dputMessage(
                new ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage(
                    has_available_resources,
                    getMessagePayloadValue(
                        ComputeServiceMessagePayload::
                        IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_ANSWER_MESSAGE_PAYLOAD)));

            // Observational request: no scheduling round is needed.
            do_scheduling = false;
            return true;
        } else if (const auto scsswu_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceSchedulerWakeupMessage>(message)) {
            do_scheduling = true;
            _scheduler_wakeup_alarm = nullptr;
            _next_scheduler_wakeup_date.reset();
            return true;
        } else if (const auto scsnam_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceNodesActivatedMessage>(message)) {
            for (auto const& node : scsnam_msg->_nodes) {
                WRENCH_INFO("Activating compute node %s", node->hostname.c_str());
                _state_of_the_system->_being_activated_compute_nodes.erase(node);
                _state_of_the_system->_activated_compute_nodes.push_back(node);
            }
            // If there are no nodes being activated, just clear all the alarms.
            // Would be better to clear the specific alarm that sent us this message,
            // but that's a bit complicated to do
            if (_state_of_the_system->_being_activated_compute_nodes.empty()) {
                _node_activation_alarms.clear();
            }
            return true;
        } else if (const auto scscic_msg = std::dynamic_pointer_cast<
            ServerlessComputeServiceContainerInitializationCompleteMessage>(message)) {
            processContainerInitializationCompletion(scscic_msg->_container, scscic_msg->_init_action);
            return true;
        } else {
            throw std::runtime_error("Unexpected [" + message->getName() + "] message");
        }
    }

    /**
    * @brief Method to add (outside the simulation, in zero time) a new registered function
    *        to a serverless compute service. Typically used before the simulation launch.
    *
    * @param name the name of the function to register
    * @param initialization_code the function's initialization code, which is executed only once on each container
     *        for the function, before any actual function executions can take place.
     * @param code the function's code, which can only be executed after the initialization code has executed (successfully) on the container.
    * @param time_limit_in_seconds the time limit for execution
    * @param disk_space_limit_in_bytes the disk space limit for the function
    * @param RAM_limit_in_bytes the RAM limit for the function
    * @param ingress_in_bytes the ingress data limit
    * @param egress_in_bytes the egress data limit
    * @return The Function object that was registered
    */
    std::shared_ptr<Function> ServerlessComputeService::addRegisteredFunction(
        const std::string& name,
        const std::function<void(const std::shared_ptr<StorageService>&)>& initialization_code,
        const std::function<std::shared_ptr<FunctionOutput>(
            const std::shared_ptr<FunctionInput>&,
            const std::shared_ptr<StorageService>&)>& code,
        const std::shared_ptr<Image>& image,
        double time_limit_in_seconds,
        sg_size_t disk_space_limit_in_bytes,
        sg_size_t RAM_limit_in_bytes,
        sg_size_t ingress_in_bytes,
        sg_size_t egress_in_bytes) {
        // Check that function can ever run!
        sg_size_t needed_disk_space = image->getDiskFootprint() + disk_space_limit_in_bytes;
        sg_size_t needed_ram_space = image->getRAMFootprint() + RAM_limit_in_bytes;

        if (needed_disk_space > _compute_node_disk_space) {
            throw ExecutionException(
                std::make_shared<NotEnoughResources>(
                    "Function cannot be registered because no compute node has sufficient disk space to execute it"));
        }

        if (needed_ram_space > _compute_node_ram) {
            throw ExecutionException(std::make_shared<NotEnoughResources>(
                "Function cannot be registered because no compute node has sufficient RAM space to execute it"));
        }

        // At this point, we can create a function object
        auto function = std::make_shared<Function>(
            name,
            initialization_code,
            code,
            image,
            time_limit_in_seconds,
            disk_space_limit_in_bytes,
            RAM_limit_in_bytes,
            ingress_in_bytes,
            egress_in_bytes);
        // Add it to the set of registered function
        _state_of_the_system->_functions.insert(function);

        return function;
    }

    /**
     * @brief Processes a "function registration request" message
     *
     * @param answer_commport the FunctionManager commport to answer to
     * @param name the name of the function to register
     * @param initialization_code the function's initialization code
     * @param code the function's code
     * @param image the function's image
     * @param time_limit the time limit for execution
     * @param disk_space_limit_in_bytes the disk space limit for the function
     * @param ram_limit_in_bytes the RAM limit for the function
     * @param ingress_in_bytes the ingress data limit
     * @param egress_in_bytes the egress data limit
     */
    void ServerlessComputeService::processFunctionRegistrationRequest(
        S4U_CommPort* answer_commport,
        const std::string& name,
        const std::function<void(const std::shared_ptr<StorageService>&)>& initialization_code,
        const std::function<std::shared_ptr<FunctionOutput>(
            const std::shared_ptr<FunctionInput>&,
            const std::shared_ptr<StorageService>&)>& code,
        const std::shared_ptr<Image>& image,
        const double time_limit,
        const sg_size_t disk_space_limit_in_bytes,
        const sg_size_t ram_limit_in_bytes,
        const sg_size_t ingress_in_bytes,
        const sg_size_t egress_in_bytes) {
        std::shared_ptr<Function> new_function;
        try {
            new_function = this->addRegisteredFunction(name, initialization_code, code, image, time_limit,
                                                       disk_space_limit_in_bytes,
                                                       ram_limit_in_bytes, ingress_in_bytes, egress_in_bytes);
        } catch (ExecutionException& e) {
            const auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(
                false, nullptr,
                e.getCause(),
                this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD));
            answer_commport->dputMessage(answerMessage);
            return;
        }

        // At this point, the function has been registered, so we can reply
        const auto answerMessage = new ServerlessComputeServiceFunctionRegisterAnswerMessage(
            true, new_function, nullptr, this->getMessagePayloadValue(
                ServerlessComputeServiceMessagePayload::FUNCTION_REGISTER_ANSWER_MESSAGE_PAYLOAD));
        answer_commport->dputMessage(answerMessage);
    }

    /**
     * @brief Processes a "function invocation request" message
     *
     * @param answer_commport the FunctionManager commport to answer to
     * @param function the function to invoke
     * @param input the input to the function
     * @param notify_commport the ExecutionController commport to notify
     */
    void ServerlessComputeService::processFunctionInvocationRequest(
        S4U_CommPort* answer_commport,
        const std::shared_ptr<Function>& function,
        const std::shared_ptr<FunctionInput>& input,
        S4U_CommPort* notify_commport) {
        // Apply the overhead
        S4U_Simulation::sleep(
            this->getPropertyValueAsTimeInSecond(ServerlessComputeServiceProperty::INVOCATION_PROCESSING_OVERHEAD));

        // If the function is not registered, answer with some error
        if (_state_of_the_system->_functions.find(function) == _state_of_the_system->_functions.end()) {
            // Not found
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                false, nullptr, std::make_shared<FunctionNotFound>(function), this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD));
            answer_commport->dputMessage(answerMessage);
            return;
        }

        // Reject the invocation if too many outstanding requests
        auto max_total = this->getPropertyValueAsUnsignedLong(
            ServerlessComputeServiceProperty::MAX_OUTSTANDING_INVOCATIONS);
        auto max_per_function = this->getPropertyValueAsUnsignedLong(
            ServerlessComputeServiceProperty::MAX_OUTSTANDING_INVOCATIONS_PER_FUNCTION);
        if ((_state_of_the_system->_num_outstanding_invocations >= max_total) or (_state_of_the_system->_outstanding_invocation_counts[function] >=
            max_per_function)) {
            const auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
                false, nullptr, std::make_shared<NotAllowed>(
                    this->getSharedPtr<ServerlessComputeService>(),
                    "Too many outstanding invocations"), this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD));
            answer_commport->dputMessage(answerMessage);
            return;
        }

        // Put the invocation in the right list
        auto invocation = std::make_shared<Invocation>(function, input, notify_commport);
        invocation->_submit_date = Simulation::getCurrentSimulatedDate();

        if (this->isInvocationSchedulable(invocation)) {
            _state_of_the_system->_schedulable_invocations.push_back(invocation);
        } else if (this->canInvocationBeAdmitted(invocation)) {
            _state_of_the_system->_admitted_invocations[function->getImage()].push_back(invocation);
        } else {
            _state_of_the_system->_new_invocations.push(invocation);
        }

        _state_of_the_system->_num_outstanding_invocations++;
        _state_of_the_system->_outstanding_invocation_counts.at(function)++;

        auto answerMessage = new ServerlessComputeServiceFunctionInvocationAnswerMessage(
            true, invocation, nullptr, this->getMessagePayloadValue(
                ServerlessComputeServiceMessagePayload::FUNCTION_INVOKE_ANSWER_MESSAGE_PAYLOAD));
        answer_commport->dputMessage(answerMessage);
    }

    /**
     * @brief Helper method to process an "image layer download completion" message
     *
     * @param action to get failure cause from
     * @param layer The image layer that was downloaded, used as key to map downloading functions
     */
    void ServerlessComputeService::processImageLayerDownloadCompletion(const std::shared_ptr<Action>& action,
                                                                       const std::shared_ptr<ImageLayer>& layer) {
        // If the download has failed, fail all corresponding admitted function invocations
        if (action->getFailureCause()) {
            _state_of_the_system->_free_space_on_head_storage += layer->getDiskFootprint();
            _state_of_the_system->_being_downloaded_image_layers.erase(layer);

            std::set<std::shared_ptr<Image>> images_to_remove_from_admitted_map;
            for (auto const& [image, list] : _state_of_the_system->_admitted_invocations) {
                // If this layer concerns this image, nevermind, we're good
                const auto& layers = image->getLayers();
                if (layers.find(layer) == layers.end()) {
                    continue;
                }

                for (auto const& invocation : _state_of_the_system->_admitted_invocations.at(image)) {
                    invocation->_notify_commport->dputMessage(
                        new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                            false,
                            invocation,
                            action->getFailureCause(), this->getMessagePayloadValue(
                                ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
                    _state_of_the_system->_num_outstanding_invocations--;
                    _state_of_the_system->_outstanding_invocation_counts.at(invocation->getFunction())--;
                }
                _state_of_the_system->_admitted_invocations.at(image).clear();
                images_to_remove_from_admitted_map.insert(image);
            }
            // Clean up (should we leave them around?)
            for (auto const& image : images_to_remove_from_admitted_map) {
                _state_of_the_system->_admitted_invocations.erase(image);
            }
            return;
        }

        WRENCH_INFO("ServerlessComputeService::processImageLayerDownloadCompletion(): Image layer [%s] was downloaded",
                    layer->getName().c_str());
        _state_of_the_system->_being_downloaded_image_layers.erase(layer);
        // _state_of_the_system->_downloaded_image_files.insert(image_file);

        // Move all relevant invocations from the admitted to the schedulable queue
        std::set<std::shared_ptr<Image>> images_to_remove_from_admitted_map;
        for (auto const& [image, list] : _state_of_the_system->_admitted_invocations) {
            // If this layer concerns this image, nevermind, we're good
            if (std::find(image->getLayers().begin(), image->getLayers().end(), layer) == image->getLayers().end()) {
                continue;
            }
            // If not all layers for the image have been download, nevermind
            bool all_layers_downloaded_for_image = true;
            for (auto const& l : image->getLayers()) {
                if (not _state_of_the_system->_head_storage_service->hasFile(l->getFile())) {
                    all_layers_downloaded_for_image = false;
                    break;
                }
            }
            if (not all_layers_downloaded_for_image) {
                continue;
            }
            // All layers are available for this image!
            auto& to_make_schedulable = _state_of_the_system->_admitted_invocations.at(image);
            _state_of_the_system->_schedulable_invocations.insert(
                _state_of_the_system->_schedulable_invocations.end(), to_make_schedulable.begin(),
                to_make_schedulable.end());
            _state_of_the_system->_admitted_invocations.at(image).clear();
            images_to_remove_from_admitted_map.insert(image);
        }
        // Remove images from the map (should we keep them around?)
        for (auto const& image : images_to_remove_from_admitted_map) {
            _state_of_the_system->_admitted_invocations.erase(image);
        }
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
                    invocation->getFunction()->getName().c_str(),
                    (failure_cause ? "FAILURE" : "SUCCESS"));

        auto compute_node = invocation->_compute_node;

        // Update outstanding counts
        _state_of_the_system->_num_outstanding_invocations--;
        _state_of_the_system->_outstanding_invocation_counts.at(invocation->getFunction())--;

        // Update container
        auto container = invocation->_container;

        if (!container->_initialized) {
            container->_in_flight_invocations.erase(invocation);
            compute_node->_initializing_containers.erase(container);
            container->shutdown();
        } else {
            compute_node->acknowledgeInvocationCompletion(invocation);
        }

        bool success = action->getState() == Action::State::COMPLETED;

        _state_of_the_system->_dispatched_invocations.erase(invocation);

        invocation->_notify_commport->dputMessage(
            new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                success,
                invocation,
                failure_cause, this->getMessagePayloadValue(
                    ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
    }

    /**
     * @brief Process a container initialization completion
     * @param container The container
     */
    void ServerlessComputeService::processContainerInitializationCompletion(const std::shared_ptr<Container>& container,
                                                                            const std::shared_ptr<Action>& init_action) {
        auto compute_node = container->getComputeNode();

        if (compute_node->_initializing_containers.count(container) == 0) {
            // Some stale message?
            return;
        }

        // Did this fail?
        if (init_action && init_action->getState() != Action::State::COMPLETED) {
            const auto cause = init_action->getFailureCause();
            const std::string reason =
                cause ? cause->toString() : "No failure cause recorded";
            WRENCH_INFO("Container initialization for function [%s] failed: %s",
                        container->getFunction()->getName().c_str(), reason.c_str());

            compute_node->_initializing_containers.erase(container);
            if (not container->_in_flight_invocations.empty()) {
                throw std::runtime_error("A container that failed to initialized should not have any attached invocation");
            }
            // for (auto const& inv : container->_in_flight_invocations) {
            //     inv->_notify_commport->dputMessage(
            //         new ServerlessComputeServiceFunctionInvocationCompleteMessage(
            //             false,
            //             inv,
            //             container->_init_failure_cause,
            //             this->getMessagePayloadValue(
            //                 ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
            //     _state_of_the_system->_dispatched_invocations.erase(inv);
            //     _num_outstanding_invocations -= 1;
            //     _outstanding_invocation_counts.at(inv->getFunction()) -= 1;
            // }
            // container->_in_flight_invocations.clear();
            container->shutdown();
            return;
        }
        compute_node->_initializing_containers.erase(container);
        container->_state = Container::State::INITIALIZED;
        compute_node->_initialized_containers.insert(container);


        // What about an alarm?
        if (container->_in_flight_invocations.empty()) {
            double idle_timeout = this->getPropertyValueAsTimeInSecond(
                ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT);
            if (idle_timeout <= 0) {
                compute_node->shutdownContainer(container);
            } else {
                auto now = S4U_Simulation::getClock();
                auto expiration_date = now + idle_timeout;
                // Start the timeout alarm
                container->_expiration_alarm = Alarm::createAndStartAlarm(
                    this->simulation_,
                    expiration_date,
                    _hostname,
                    _commport,
                    new
                    ServerlessComputeServiceContainerIdleTimeoutMessage(
                        container, container->getIdleSequence()),
                    "container_idle_timout");
                container->_expiration_date = expiration_date;
                container->_idle_date = now;
            }
        }
    }

    /**
     * @brief Process the completion of a layer copy to a node
     * @param layer The image layer that was to be copied
     * @param node The compute node where it was to be copied to
     * @param action The copying action
     */
    void ServerlessComputeService::processNodeCopyCompletion(const std::shared_ptr<ImageLayer>& layer,
                                                             const std::shared_ptr<ServerlessComputeNode>& node,
                                                             const std::shared_ptr<Action>& action) {
        _state_of_the_system->_image_layers_being_copied_to_disk.at(node).erase(layer);
        if (action->getState() != Action::State::COMPLETED) {
            WRENCH_INFO("An image layer copy has failed for image layer [%s] (%s)",
                       layer->getName().c_str(), action->getFailureCause()->toString().c_str());
        } else {
            WRENCH_INFO("Image layer [%s] was stored on disk at [%s]",
                        layer->getName().c_str(), node->hostname.c_str());
            _state_of_the_system->_image_layers_on_disk.at(node).insert(layer);
        }
    }

    /**
     * @brief Process the completion of a layer load to a node
     * @param layer The image layer that was to be loaded
     * @param node The compute node where it was to be loaded to
     * @param action The loading action
     */
    void ServerlessComputeService::processNodeLoadCompletion(const std::shared_ptr<ImageLayer>& layer,
        const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<Action>& action) {
        _state_of_the_system->_image_layers_being_loaded_in_ram.at(node).erase(layer);
        if (action->getState() != Action::State::COMPLETED) {
            WRENCH_INFO("A memory load has failed for image layer [%s] (%s)",
                        layer->getName().c_str(), action->getFailureCause()->toString().c_str());
        } else {
            WRENCH_INFO("Image layer [%s] was loaded in RAM at [%s]",
                        layer->getName().c_str(), node->hostname.c_str());
            _state_of_the_system->_image_layers_in_ram.at(node).insert(layer);
        }
    }


    /**
     * @brief Helper method to process a container idle timeout
     * @param container the container that has idle-timed out
     * @param idle_sequence the idle sequence number
     */
    void ServerlessComputeService::processContainerIdleTimeout(const std::shared_ptr<Container>& container,
                                                               const std::uint64_t idle_sequence) {
       WRENCH_INFO("WE GOT A CONTAINER TIME OUT");
        // If this isn't a stale/no-longer-valid message, nevermind
        if (container->getIdleSequence() != idle_sequence) {
            return;
        }
        // Shutdown the container
        container->getComputeNode()->shutdownContainer(container);
        container->_expiration_alarm = nullptr;
        container->_expiration_date = DBL_MAX;
    }

    /**
     * @brief Dispatches scheduled function invocations to compute nodes
     * @param decisions Scheduling decisions
     */
    void ServerlessComputeService::dispatchInvocations(
        const std::vector<DispatchInvocation>& decisions) {
        // Dispatched the invocations in the order of the schedulable list
        std::set<std::shared_ptr<Invocation>> dispatched_invocations;

        // Dispatch invocation
        for (const auto& [invocation, compute_node, container] : decisions) {
            if (std::find(_state_of_the_system->_activated_compute_nodes.begin(),
                          _state_of_the_system->_activated_compute_nodes.end(), compute_node) ==
                _state_of_the_system->_activated_compute_nodes.end()) {
                WRENCH_INFO("Ignoring invocation dispatch scheduling decision at non-activated compute node %s",
                            compute_node->hostname.c_str());
                continue;
            }
            // If the invocation is not schedulable or already dispatched, ignore the scheduling decision!
            const auto& ready = _state_of_the_system->_schedulable_invocations;
            if (std::find(ready.begin(), ready.end(), invocation) == ready.end() ||
                dispatched_invocations.count(invocation) != 0) {
                WRENCH_INFO("Ignoring dispatch scheduling decision for non-dispatchable invocation for function %s",
                            invocation->getFunction()->getName().c_str());
                continue;
            }

            if (dispatchInvocation(invocation, compute_node, container)) {
                _state_of_the_system->_dispatched_invocations.insert(invocation);
                dispatched_invocations.insert(invocation);
            }
        }

        // Update the list of schedulable invocations (not super efficient, but clearer than the erase-as-I-go)
        if (!dispatched_invocations.empty()) {
            auto& ready = _state_of_the_system->_schedulable_invocations;

            ready.erase(
                std::remove_if(
                    ready.begin(),
                    ready.end(),
                    [&dispatched_invocations](const auto& invocation) {
                        return dispatched_invocations.count(invocation) != 0;
                    }),
                ready.end());
        }
    }

    /**
    * @brief Terminate idle containers
    * @param decisions Scheduling decisions
    */
    void ServerlessComputeService::failInvocations(const std::vector<FailInvocation>& decisions) {
        // Fail invocations
        for (const auto& [inv] : decisions) {
            // Is the invocation running?
            if (_state_of_the_system->_dispatched_invocations.count(inv) > 0) {
                WRENCH_INFO("Cannot forcefully termine a running invocation - ignoring scheduling decision");
                continue;
            }
            auto image = inv->getFunction()->getImage();

            bool send_notification_message = false;

            // Is the invocation admitted?
            {
                if (_state_of_the_system->_admitted_invocations.find(image) != _state_of_the_system->
                                                                               _admitted_invocations.end()) {
                    auto it = std::find(_state_of_the_system->_admitted_invocations.at(image).begin(),
                                        _state_of_the_system->_admitted_invocations.at(image).end(), inv);
                    if (it != _state_of_the_system->_admitted_invocations.at(image).end()) {
                        _state_of_the_system->_admitted_invocations.at(image).erase(it);
                        _state_of_the_system->_outstanding_invocation_counts.at(inv->getFunction())--;
                        _state_of_the_system->_num_outstanding_invocations--;
                        send_notification_message = true;
                    }
                }
            }

            // Is the invocation schedulable?
            {
                auto it = std::find(_state_of_the_system->_schedulable_invocations.begin(),
                                    _state_of_the_system->_schedulable_invocations.end(), inv);
                if (it != _state_of_the_system->_schedulable_invocations.end()) {
                    _state_of_the_system->_schedulable_invocations.erase(it);
                    _state_of_the_system->_outstanding_invocation_counts.at(inv->getFunction())--;
                    _state_of_the_system->_num_outstanding_invocations--;
                    send_notification_message = true;
                }
            }

            // Notify the submitted
            if (send_notification_message) {
                inv->_notify_commport->dputMessage(
                    new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                        false,
                        inv,
                        std::make_shared<NotAllowed>(this->getSharedPtr<ServerlessComputeService>(),
                                                     "Invocation was forcefully failed by the scheduler"),
                        this->getMessagePayloadValue(
                            ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
            }
        }
    }

    /**
     * @brief Terminate idle containers
     * @param decisions Scheduling decisions
     */
    void ServerlessComputeService::terminateIdleContainers(const std::vector<TerminateIdleContainer>& decisions) const {
        // Terminate the containers
        for (const auto& [container] : decisions) {
            auto node = container->getComputeNode();
            node->shutdownContainer(container);
        }
    }

    /**
     * @brief Evict layers from RAM
     * @param decisions Scheduling decisions
     */
    void ServerlessComputeService::evictLayersFromRAM(const std::vector<EvictLayerFromRAM>& decisions) const {
        for (const auto& [layer, node] : decisions) {
            StorageService::removeFileAtLocation(FileLocation::LOCATION(node->_memory, layer->getRAMFile()));
            _state_of_the_system->_image_layers_in_ram.at(node).erase(layer);
        }
    }

    /**
     * @brief Evict layers from disk
     * @param decisions Scheduling decisions
     */
    void ServerlessComputeService::evictLayersFromDisk(const std::vector<EvictLayerFromDisk>& decisions) const {
        for (const auto& [layer, node] : decisions) {
            StorageService::removeFileAtLocation(FileLocation::LOCATION(node->_disk, layer->getFile()));
            _state_of_the_system->_image_layers_on_disk.at(node).erase(layer);
        }
    }

    /**
     * @brief Helper method to dispatch an invocation
     *
     * @param invocation invocation to dispatch
     * @param target_compute_node the target compute node
     * @param target_container the target container (if nullptr, create a new container)
     *
     * @return true on success, false on failure
     */
    bool ServerlessComputeService::dispatchInvocation(const std::shared_ptr<Invocation>& invocation,
                                                      const std::shared_ptr<ServerlessComputeNode>& target_compute_node,
                                                      std::shared_ptr<Container> target_container) {
        // Check that things can work at all
        if (not target_compute_node->couldInvocationRunRightNow(invocation, target_container)) {
            WRENCH_INFO("Cannot dispatch invocation.... ignoring scheduling decision");
            return false;
        }

        bool hot_start = target_container != nullptr;

        if (not hot_start) {
            // Try to spawn a container
            try {
                target_container = target_compute_node->spawnContainer(invocation->getFunction());
            } catch (ExecutionException& e) {
                WRENCH_INFO("Couldn't spawn a container for an invocation for function %s: %s",
                            invocation->_function->getName().c_str(),
                            e.getCause()->toString().c_str());
                return false;
            }
        }

        // Bookkeeping
        invocation->_was_hot_start = hot_start;
        invocation->_container = target_container;
        invocation->_compute_node = target_compute_node;
        invocation->_dispatch_date = Simulation::getCurrentSimulatedDate();
        invocation->_dispatched = true;

        target_compute_node->acknowledgeInvocationStart(invocation);

        // Declare the function invocation's necessary lambdas
        const std::function lambda_terminate = [invocation](const std::shared_ptr<ActionExecutor>& action_executor) {
            // noop
        };

        const std::function lambda_execute = [invocation, this](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            auto container = invocation->_container;
            auto private_storage_service = container->getPrivateStorageService();

            if (not container->_initialized) {
                // Invoke the user's initialization function
                try {
                    invocation->_function->_initialization_code(private_storage_service);
                    container->_initialized = true;
                    this->getCommPort()->dputMessage(
                        new ServerlessComputeServiceContainerInitializationCompleteMessage(
                            container, nullptr));
                } catch (ExecutionException& e) {
                    container->_init_failure_cause = e.getCause();
                    throw;
                }
            }


            // Invoke the user's lambda function
            invocation->_function_start_date = S4U_Simulation::getClock();
            try {
                invocation->_function_output =
                    invocation->_function->_code(invocation->_function_input,
                                                 private_storage_service);
            } catch (ExecutionException&) {
                // invocation->_container->clearPrivateStorage();
                invocation->_function_end_date = S4U_Simulation::getClock();
                throw;
            }
            invocation->_function_end_date = S4U_Simulation::getClock();
        };

        // Create the action and create a corresponding action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "run_invocation_" + invocation->_function->getName(),
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceInvocationExecutionCompleteMessage(action, invocation, 0);

        const auto action_executor = std::make_shared<ActionExecutor>(
            target_compute_node->hostname,
            1,
            0,
            (hot_start
                 ? 0
                 : this->getPropertyValueAsTimeInSecond(ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD)),
            false,
            _commport,
            custom_message,
            action,
            nullptr);

        action_executor->setActionTimeout(invocation->getFunction()->getTimeLimit());
        action_executor->setSimulation(this->simulation_);

        WRENCH_INFO("Dispatched an invocation for [%s] to [%s]",
                    invocation->getFunction()->getName().c_str(),
                    target_compute_node->hostname.c_str());

        // Start the action executor object
        try {
            action_executor->start(action_executor, true, false);
        } catch (ExecutionException&) {
            throw std::runtime_error(
                "ServerlessComputeService::dispatchInvocation(): Action executor could not be started - internal error");
        }

        return true;
    }

    /**
     * @brief Method to shut everything down
     */
    void ServerlessComputeService::terminateAllServicesAndReleaseAllResources() {
        // Terminate the scheduler wakeup alarm if any
        if (_scheduler_wakeup_alarm) {
            _scheduler_wakeup_alarm->kill();
            _scheduler_wakeup_alarm.reset();;
            _next_scheduler_wakeup_date.reset();
        }

        // Terminate the node activation alarms if any
        for (auto const& alarm : _node_activation_alarms) {
            alarm->kill();
        }
        _node_activation_alarms.clear();

        // Shutdown all containers (it doesn't actually kill the running functions, but who cares really)
        for (auto const& node : _state_of_the_system->_activated_compute_nodes) {
            node->killAllContainers();
        }

        // Terminate the head storage service
        _state_of_the_system->_head_storage_service->stop();

        // Terminate all node storage services
        for (auto const& node : _state_of_the_system->_activated_compute_nodes) {
            node->_disk->stop();
            node->_disk.reset();
            node->_memory->stop();
            node->_memory.reset();
        }
        for (auto const& node : _state_of_the_system->_being_activated_compute_nodes) {
            node->_disk->stop();
            node->_disk.reset();
            node->_memory->stop();
            node->_memory.reset();
        }
        for (auto const& node : _state_of_the_system->_deactivated_compute_nodes) {
            node->_disk->stop();
            node->_disk.reset();
            node->_memory->stop();
            node->_memory.reset();
        }

        // Send all failure notifications
        for (auto& [image, list] : _state_of_the_system->_admitted_invocations) {
            for (auto const& inv : list) {
                inv->_notify_commport->dputMessage(
                    new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                        false,
                        inv,
                        std::make_shared<ServiceIsDown>(this->getSharedPtr<ServerlessComputeService>()),
                        this->getMessagePayloadValue(
                            ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
            }
        }
        for (auto const& inv : _state_of_the_system->_schedulable_invocations) {
            inv->_notify_commport->dputMessage(
                new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                    false,
                    inv,
                    std::make_shared<ServiceIsDown>(this->getSharedPtr<ServerlessComputeService>()),
                    this->getMessagePayloadValue(
                        ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
        }
        for (auto const& inv : _state_of_the_system->_dispatched_invocations) {
            inv->_notify_commport->dputMessage(
                new ServerlessComputeServiceFunctionInvocationCompleteMessage(
                    false,
                    inv,
                    std::make_shared<ServiceIsDown>(this->getSharedPtr<ServerlessComputeService>()),
                    this->getMessagePayloadValue(
                        ServerlessComputeServiceMessagePayload::FUNCTION_COMPLETION_MESSAGE_PAYLOAD)));
        }

        _state_of_the_system->_admitted_invocations.clear();
        _state_of_the_system->_schedulable_invocations.clear();
        _state_of_the_system->_dispatched_invocations.clear();

        _state_of_the_system->_num_outstanding_invocations = 0;
        _state_of_the_system->_outstanding_invocation_counts.clear();
    }

    /**
     * @brief Start disk/ram storage services at a node.
     */
    void ServerlessComputeService::startServicesAtComputeNode(const std::shared_ptr<ServerlessComputeNode>& node) {
        if (not S4U_Simulation::hostHasMountPoint(node->hostname, "/")) {
            throw std::invalid_argument("ServerlessComputeService::startComputeNodeServices(): "
                "each compute node in a serverless compute service should have a \"/\" mountpoint");
        }

        // Start a compute service, to implement compute-node storage
        {
            const auto ss = std::dynamic_pointer_cast<SimpleStorageService>(this->simulation_->startNewService(
                SimpleStorageService::createSimpleStorageService(
                    node->hostname,
                    {"/"},
                    {
                        {
                            SimpleStorageServiceProperty::BUFFER_SIZE,
                            this->getPropertyValueAsString(
                                ServerlessComputeServiceProperty::STORAGE_SERVICES_BUFFER_SIZE)
                        }
                    },
                    {})));
            ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
            node->_disk = ss;
        }

        // Start a compute service, with LRU caching, to implement compute-node memory
        {
            // Create a RAM disk on the compute node
            auto host = simgrid::s4u::Engine::get_instance()->host_by_name(node->hostname);
            auto ram_disk = host->add_disk(
                "ram_disk_" + std::to_string(_sequence_number++),
                S4U_Simulation::RAM_READ_BANDWIDTH,
                S4U_Simulation::RAM_WRITE_BANDWIDTH);
            auto ram_capacity = S4U_Simulation::getHostMemoryCapacity(node->hostname);
            std::string ram_mount_point = "/" + ram_disk->get_name();
            ram_disk->set_property("size", std::to_string(ram_capacity) + "B");
            ram_disk->set_property("mount", ram_mount_point);

            const auto ss = std::dynamic_pointer_cast<SimpleStorageService>(this->simulation_->startNewService(
                SimpleStorageService::createSimpleStorageService(
                    node->hostname,
                    {ram_mount_point},
                    {
                        // {SimpleStorageServiceProperty::CACHING_BEHAVIOR, "LRU"},
                        {
                            SimpleStorageServiceProperty::BUFFER_SIZE,
                            this->getPropertyValueAsString(
                                ServerlessComputeServiceProperty::STORAGE_SERVICES_BUFFER_SIZE)
                        }
                    },
                    {})));
            ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
            node->_memory = ss;
        }
    }

    /**
     * @brief Start a SimpleStorageService for each compute node. We don't start a bare-metal
     *        service as we'll do everything ourselves with action executor services.
     */
    void ServerlessComputeService::startComputeNodeServices() {
        for (auto const& compute_node : _state_of_the_system->_activated_compute_nodes) {
            this->startServicesAtComputeNode(compute_node);
        }
        for (auto const& compute_node : _state_of_the_system->_deactivated_compute_nodes) {
            this->startServicesAtComputeNode(compute_node);
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
            auto invocation = _state_of_the_system->_new_invocations.front();
            const auto image = invocation->_function->getImage();
            WRENCH_INFO("Admitting invocation %llu...", invocation->getId());

            if (this->canInvocationBeAdmitted(invocation)) {
                _state_of_the_system->_new_invocations.pop();
                _state_of_the_system->_admitted_invocations[image].push_back(invocation);
                continue;
            }

            // Otherwise, initiate all layer downloads (in parallel)
            for (auto const& layer : image->getLayers()) {
                // If already available, nevermind
                if (_state_of_the_system->_head_storage_service->hasFile(layer->getFile())) {
                    continue;
                }
                // If being downloaded, nevermind
                if (_state_of_the_system->_being_downloaded_image_layers.count(layer) > 0) {
                    continue;
                }

                if (_state_of_the_system->_free_space_on_head_storage >= layer->getDiskFootprint()) {
                    // "Reserve" space on the storage service
                    _state_of_the_system->_free_space_on_head_storage -= layer->getDiskFootprint();
                    // initiate the download
                    _state_of_the_system->_being_downloaded_image_layers.insert(layer);
                    initiateImageLayerDownloadFromRemote(layer);
                } else {
                    throw std::runtime_error("ServerlessComputeService::admitInvocations(): "
                        "An invocation cannot be admitted because there is not enough space on the head storage to download its (remote) image. "
                        "Currently, no mechanism is implemented to manage the head storage (just make it bigger).");
                }
            }
            _state_of_the_system->_new_invocations.pop();
            _state_of_the_system->_admitted_invocations[image].push_back(invocation);
        }
    }

    /**
     * @brief Helper method to initiate an image layer download
     *
     * @param layer an image layer
     */
    void ServerlessComputeService::initiateImageLayerDownloadFromRemote(const std::shared_ptr<ImageLayer>& layer) {
        // Create a custom action
        const std::function lambda_execute = [layer, this
            ](const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the lambda execute!!");
            const auto src_location = layer->getLocation();
            const auto dst_location = FileLocation::LOCATION(_state_of_the_system->_head_storage_service,
                                                             src_location->getFile());
            if (this->getPropertyValueAsBoolean(ServerlessComputeServiceProperty::SIMULATE_REMOTE_IMAGE_DOWNLOADS)) {
                StorageService::copyFile(src_location, dst_location);
            } else {
                StorageService::createFileAtLocation(dst_location);
            }
        };
        const std::function lambda_terminate = lambda_noop;

        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "download_image_layer_" + layer->getName(),
                0, 0, lambda_execute, lambda_terminate));

        // Spin up an ActionExecutor service, and have it send us back a custom message
        auto custom_message = new ServerlessComputeServiceDownloadCompleteMessage(
            action,
            layer, 0);

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
        // WRENCH_INFO("Starting an action executor for downloading a remove image...");
        try {
            action_executor->start(action_executor, true, false); // Daemonized, no auto-restart
        } catch (const std::exception&) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageDownloadFromRemote(): Action executor could not be started - internal error");
        }
    }

    /**
     * @brief Helper method to invoke the scheduler
     * @return the scheduler's scheduling decisions
     */
    std::shared_ptr<ServerlessSchedulingDecisions> ServerlessComputeService::invokeScheduler() const {
        auto decisions =
            _scheduler->schedule(_state_of_the_system.get());
#if 0
        decisions->print();
#endif
        return decisions;
    }

    /**
     * @brief Helper method to initiate compute node activations
     * @param decisions
     */
    void ServerlessComputeService::activateComputeNodes(const std::vector<ActivateComputeNodes>& decisions) {
        unsigned long num_to_activate = 0;

        for (const auto& [num_compute_nodes] : decisions) {
            num_to_activate += num_compute_nodes;
        }

        if (_state_of_the_system->_deactivated_compute_nodes.size() < num_to_activate) {
            WRENCH_INFO(
                "Scheduling decision to activate %lu compute nodes, but activating all %lu deactivated nodes instead",
                num_to_activate, _state_of_the_system->_deactivated_compute_nodes.size());
            num_to_activate = _state_of_the_system->_deactivated_compute_nodes.size();
        }

        if (num_to_activate == 0) {
            return;
        }

        double node_activation_overhead = this->getPropertyValueAsTimeInSecond(
            ServerlessComputeServiceProperty::COMPUTE_NODE_ACTIVATION_OVERHEAD);
        if (!std::isfinite(node_activation_overhead) || node_activation_overhead < 0.0) {
            throw std::invalid_argument(
                "ServerlessComputeServiceProperty::COMPUTE_NODE_ACTIVATION_OVERHEAD must be finite and non-negative");
        }

        // Remove the last num_to_activates compute node from the deactivated list
        std::vector<std::shared_ptr<ServerlessComputeNode>>& deactivated_nodes =
            _state_of_the_system->_deactivated_compute_nodes;
        const auto first =
            deactivated_nodes.end() -
            static_cast<typename std::vector<std::shared_ptr<ServerlessComputeNode>>::difference_type>(num_to_activate);
        std::vector to_activate(std::make_move_iterator(first),
                                std::make_move_iterator(deactivated_nodes.end()));
        deactivated_nodes.erase(first, deactivated_nodes.end());

        // Put the being activated compute node in the list
        for (auto const& node : to_activate) {
            _state_of_the_system->_being_activated_compute_nodes.insert(node);
        }

        auto alarm = Alarm::createAndStartAlarm(this->simulation_,
                                                S4U_Simulation::getClock() + node_activation_overhead,
                                                _hostname, _commport,
                                                new ServerlessComputeServiceNodesActivatedMessage(to_activate),
                                                "nodes_activated");
        _node_activation_alarms.insert(alarm);
    }

    /**
     * @brief Helper method to initiate compute node deactivations
     * @param decisions
     */
    void ServerlessComputeService::deactivateComputeNodes(const std::vector<DeactivateComputeNodes>& decisions) {

        for (const auto& [node] : decisions) {

            const bool active = std::find(_state_of_the_system->_activated_compute_nodes.begin(),
                _state_of_the_system->_activated_compute_nodes.end(), node) != _state_of_the_system->_activated_compute_nodes.end();

            if (not active) {
                WRENCH_INFO("Scheduling decision to deactivate compute node %s, "
                            "but node is not activated - ignoring", node->hostname.c_str());
                continue;
            }

            if (!_state_of_the_system->_image_layers_being_copied_to_disk.at(node).empty() ||
                !_state_of_the_system->_image_layers_being_loaded_in_ram.at(node).empty()) {
                WRENCH_INFO("Scheduling decision to deactivate compute node %s, "
                            "but a layers are being copied/loaded on that node", node->hostname.c_str());
                continue;
            }

            if (not node->getInitializingContainers().empty()) {
                WRENCH_INFO("Scheduling decision to deactivate compute node %s, "
                            "but a container is initializing on that node - ignoring", node->hostname.c_str());
                continue;
            }
            bool in_flight_invocations = false;
            for (auto &container : node->getInitializedContainers()) {
                if (container->getNumInflightInvocations() > 0) {
                    in_flight_invocations = true;
                    break;
                }
            }
            if (in_flight_invocations) {
                WRENCH_INFO("Scheduling decision to deactivate compute node %s, "
                            "but some invocation is running on that node - ignoring", node->hostname.c_str());
                continue;
            }

            // Shutdown all containers
            while (!node->getInitializedContainers().empty()) {
                // Copy the shared_ptr: shutdownContainer() erases its set entry.
                const auto container = *node->getInitializedContainers().begin();
                node->shutdownContainer(container);
            }

            // Remove all the layers from RAM
            auto& ram_layers = _state_of_the_system->_image_layers_in_ram.at(node);
            while (!ram_layers.empty()) {
                // Copy the handle because the eviction helper removes its set entry.
                const auto layer = *ram_layers.begin();
                evictLayersFromRAM({{layer, node}});
            }

            // Remove all the layers from Disk
            auto& disk_layers = _state_of_the_system->_image_layers_on_disk.at(node);
            while (!disk_layers.empty()) {
                const auto layer = *disk_layers.begin();
                evictLayersFromDisk({{layer, node}});
            }

            // Update the bookkeeping
            _state_of_the_system->_activated_compute_nodes.erase(std::remove(_state_of_the_system->_activated_compute_nodes.begin(),
                _state_of_the_system->_activated_compute_nodes.end(), node), _state_of_the_system->_activated_compute_nodes.end());
            _state_of_the_system->_deactivated_compute_nodes.push_back(node);

        }
    }

    /**
     * @brief Helper method to initiate image loads
     * @param decisions scheduling decisions
     */
    void ServerlessComputeService::initiateImageLayerLoads(const std::vector<LoadImageLayer>& decisions) {
        // For each compute node, load initiate image load from disk into RAM and if space
        for (const auto& [layer, compute_node] : decisions) {
            if (std::find(_state_of_the_system->_activated_compute_nodes.begin(),
                          _state_of_the_system->_activated_compute_nodes.end(), compute_node) ==
                _state_of_the_system->_activated_compute_nodes.end()) {
                WRENCH_INFO("Ignoring image layer load scheduling decision at non-activated compute node %s",
                            compute_node->hostname.c_str());
                continue;
            }
            initiateImageLayerLoadAtComputeNode(compute_node, layer);
        }
    }

    /**
    * @brief Helper method to initiate image copies
    * @param decisions scheduling decisions
    */
    void ServerlessComputeService::initiateImageLayerCopies(
        const std::vector<CopyImageLayer>& decisions) {
        // For each compute node, initiate image copy (from head node) if need be
        for (const auto& [layer, compute_node] : decisions) {
            if (std::find(_state_of_the_system->_activated_compute_nodes.begin(),
                          _state_of_the_system->_activated_compute_nodes.end(), compute_node) ==
                _state_of_the_system->_activated_compute_nodes.end()) {
                WRENCH_INFO("Ignoring image layer copy scheduling decision to non-activated compute node %s",
                            compute_node->hostname.c_str());
                continue;
            }
            initiateImageLayerCopyToComputeNode(compute_node, layer);
        }
    }

    /**
    * @brief Helper method to prewarm containers
    * @param decisions scheduling decisions
    */
    void ServerlessComputeService::prewarmContainers(
        const std::vector<PrewarmContainer>& decisions) {
        // For each compute node, prewarm a container (if possible)
        for (const auto& [function, compute_node] : decisions) {
            if (std::find(_state_of_the_system->_activated_compute_nodes.begin(),
                          _state_of_the_system->_activated_compute_nodes.end(), compute_node) ==
                _state_of_the_system->_activated_compute_nodes.end()) {
                WRENCH_INFO("Ignoring container pre-warming scheduling decision at non-activated compute node %s",
                            compute_node->hostname.c_str());
                continue;
            }
            prewarmContainerAtComputeNode(compute_node, function);
        }
    }

    /**
     * @brief Determine whether an invocation is schedulable
     * @param invocation An invocation
     * @return true if schedulable, false otherwise
     */
    bool ServerlessComputeService::isInvocationSchedulable(const std::shared_ptr<Invocation>& invocation) const {
        for (auto const& layer : invocation->getFunction()->getImage()->getLayers()) {
            if (not _state_of_the_system->_head_storage_service->hasFile(layer->getFile())) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Determine whether an invocation can be admitted
     * @param invocation An invocation
     * @return true if the invocation can be admitted, false otherwise
     */
    bool ServerlessComputeService::canInvocationBeAdmitted(const std::shared_ptr<Invocation>& invocation) const {
        for (auto const& layer : invocation->getFunction()->getImage()->getLayers()) {
            if (_state_of_the_system->_head_storage_service->hasFile(layer->getFile())) {
                continue;
            }
            if (not _state_of_the_system->_being_downloaded_image_layers.count(layer)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Method to initiate an image layer copy from the head host to a compute node
     * @param compute_node The compute node
     * @param layer The image layer
     */
    void ServerlessComputeService::initiateImageLayerCopyToComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& compute_node,
        const std::shared_ptr<ImageLayer>& layer) {

        // Sanity check: Is the image layer already there?
        if (compute_node->isImageLayerOnDisk(layer)) {
            WRENCH_INFO("ServerlessComputeService::initiateImageLayerCopyToComputeNode(): Being told to copy image layer [%s] to node [%s], but it's already there - ignoring",
                layer->getName().c_str(), compute_node->hostname.c_str());
            return;
        }

        // Sanity check: Is the image layer being copied there?
        if (_state_of_the_system->_image_layers_being_copied_to_disk.at(compute_node).count(layer) > 0) {
            WRENCH_INFO("ServerlessComputeService::initiateImageLayerCopyToComputeNode(): Being told to copy image layer [%s] to node [%s], but it's already being copied - ignoring",
                layer->getName().c_str(), compute_node->hostname.c_str());
            return;
        }

        const std::function lambda_terminate = lambda_noop;

        const std::function lambda_execute = [compute_node, layer, this](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the image copy lambda execute!!");

            // Copy the image file from the head host to the current compute node's storage service
            auto head_host_image_path = FileLocation::LOCATION(_state_of_the_system->_head_storage_service,
                                                               layer->getFile());
            auto local_image_path = FileLocation::LOCATION(compute_node->_disk, layer->getFile());
            StorageService::copyFile(head_host_image_path, local_image_path);
            // WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "copy_image_layer_" + layer->getName() + "_to_" + compute_node->hostname,
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceNodeCopyCompleteMessage(
            action,
            layer,
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
        } catch (const std::exception&) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageCopyToComputeNode(): Action executor could not be started - internal error");
        }

        // Add the image to the being_copied_images data structure for this compute node
        _state_of_the_system->_image_layers_being_copied_to_disk.at(compute_node).insert(layer);


        WRENCH_INFO("Initiated image layer copy: [%s] to [%s]", layer->getName().c_str(),
                    compute_node->hostname.c_str());
    }

    /**
     * @brief Method to initiate an image layer load from disk to RAM at a compute node
     * @param compute_node The compute node
     * @param layer The image layer
     */
    void ServerlessComputeService::initiateImageLayerLoadAtComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& compute_node,
        const std::shared_ptr<ImageLayer>& layer) {
        // Sanity check: Is the image layer already there?
        if (compute_node->isImageLayerInRAM(layer)) {
            WRENCH_INFO("ServerlessComputeService::initiateImageLayerLoadAtComputeNode(): Being told to load image layer [%s] at node [%s], but it's already loaded - ignoring",
                layer->getName().c_str(), compute_node->hostname.c_str());
            return;
        }

        // Sanity check: Is the image layer being loaded there?
        if (_state_of_the_system->_image_layers_being_loaded_in_ram.at(compute_node).count(layer) > 0) {
            WRENCH_INFO("ServerlessComputeService::initiateImageLayerLoadAtComputeNode(): Being told to load image layer [%s] to node [%s], but it's already being loaded - ignoring",
                layer->getName().c_str(), compute_node->hostname.c_str());
            return;
        }

        // Initiate an asynchronous action that simply read the image file from disk
        const std::function lambda_terminate = lambda_noop;

        const std::function lambda_execute = [compute_node, layer](
            const std::shared_ptr<ActionExecutor>& action_executor) {
            // WRENCH_INFO("In the image load lambda execute!!");
            auto src_location = wrench::FileLocation::LOCATION(
                compute_node->_disk, layer->getFile());
            auto dst_location = wrench::FileLocation::LOCATION(
                compute_node->_memory, compute_node->_memory->getBaseRootPath(),
                layer->getRAMFile());

            // This below is a "hack" to implement the "read only the RAM portion of the on-disk image",
            // for which StorageService implementations don't provide an API to
            // create and open a tmp file to "reserve" space while the reading occurs. AND,
            // this also correctly simulates that the disk bandwidth is the bottleneck.
            auto tmp_file = Simulation::addTmpFile(layer->getRAMFootprint());
            auto tmp_file_location = wrench::FileLocation::LOCATION(compute_node->_memory, tmp_file);
            std::shared_ptr<simgrid::fsmod::File> open_tmp_file;
            try {
                SimpleStorageService::createFileAtLocation(tmp_file_location);
                open_tmp_file = compute_node->_memory->openFile(tmp_file_location);
                // Simulate the read of the RAM portion of the image on disk
                compute_node->_disk->readFile(src_location, layer->getRAMFootprint());
                // Close and remove the tmp file
                open_tmp_file->close();
                open_tmp_file.reset();
                SimpleStorageService::removeFileAtLocation(tmp_file_location);
                Simulation::removeFile(tmp_file);
                // Create the RAM file in its place
                compute_node->_memory->createFile(layer->getRAMFile());
            } catch (ExecutionException&) {
                // Remove/close the tmp file if failed
                if (open_tmp_file) {
                    open_tmp_file->close();
                }
                if (SimpleStorageService::hasFileAtLocation(tmp_file_location)) {
                    SimpleStorageService::removeFileAtLocation(tmp_file_location);
                }
                Simulation::removeFile(tmp_file);
                throw;
            }
            // WRENCH_INFO("Done with the lambda execute!!");
        };

        // Create the action and run it in an action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction(
                "load_image_layer_" + layer->getName() + "_at_" + compute_node->hostname,
                0, 0, lambda_execute, lambda_terminate));

        auto custom_message = new ServerlessComputeServiceNodeLoadCompleteMessage(
            action,
            layer,
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
        } catch (const std::exception&) {
            throw std::runtime_error(
                "ServerlessComputeService::initiateImageLoadAtComputeNode(): Action executor could not be started - internal error");
        }

        // Add the image to the being_copied_images data structure for this compute node
        _state_of_the_system->_image_layers_being_loaded_in_ram.at(compute_node).insert(layer);

        WRENCH_INFO("Initiated image layer load: [%s] at [%s]", layer->getName().c_str(),
                    compute_node->hostname.c_str());
    }

    /**
     * @brief Helper method to prewarm a container at a compute node
     * @param compute_node the compute node
     * @param function the function for which a container should be prewarmed
     */
    void ServerlessComputeService::prewarmContainerAtComputeNode(
        const std::shared_ptr<ServerlessComputeNode>& compute_node,
        const std::shared_ptr<Function>& function) {
        // Check that pre-warming can even do anything
        double idle_timeout = this->getPropertyValueAsTimeInSecond(
            ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT);
        if (idle_timeout <= 0.0) {
            WRENCH_INFO(
                "Ignoring container pre-warm scheduling decision because the "
                "CONTAINER_IDLE_TIMEOUT value is set to 0.0");
            return;
        }

        // Check that limits are ok
        if (compute_node->isAtPerNodeContainerLimit()) {
            WRENCH_INFO(
                "Couldn't pre-warm container for function %s at node %s because we've reached the number of containers limit",
                function->getName().c_str(), compute_node->hostname.c_str());
            return;
        }

        // Check that all layers are in RAM
        for (auto const& layer : function->getImage()->getLayers()) {
            if (!_state_of_the_system->isImageLayerInRAMAtNode(compute_node, layer)) {
                WRENCH_INFO(
                    "Couldn't pre-warm container for function %s at node %s because image layer %s is not in RAM",
                    function->getName().c_str(), compute_node->hostname.c_str(), layer->getName().c_str());
                return;
            }
        }
        // Check that there is enough disk/RAM space to start the new container
        if (_state_of_the_system->getAvailableDiskSpace().at(compute_node) < function->getDiskSpaceLimit()) {
            WRENCH_INFO("Couldn't pre-warm container for function %s at node %s due to lack of available disk space",
                        function->getName().c_str(), compute_node->hostname.c_str());
            return;
        }
        if (_state_of_the_system->getAvailableRAMSpace().at(compute_node) < function->getRAMSpaceLimit()) {
            WRENCH_INFO("Couldn't pre-warm container for function %s at node %s due to lack of available RAM space",
                        function->getName().c_str(), compute_node->hostname.c_str());
            return;
        }
        // At this point, we can start the container
        std::shared_ptr<Container> container;
        try {
            container = compute_node->spawnContainer(function);
        } catch (ExecutionException& e) {
            WRENCH_INFO("Couldn't pre-warm a container for function %s at node %s: %s",
                        function->getName().c_str(),
                        compute_node->hostname.c_str(),
                        e.getCause()->toString().c_str());
            return;
        }

        // The prewarm runs the initialization step!
        const std::function initialization_code = [container, function]
        (const std::shared_ptr<ActionExecutor>& action_executor) {
            auto private_storage_service = container->getPrivateStorageService();
            if (not container->_initialized) {
                // Invoke the user's initialization function
                try {
                    function->_initialization_code(private_storage_service);
                    container->_initialized = true;
                } catch (ExecutionException& e) {
                    container->_init_failure_cause = e.getCause();
                    throw;
                }
            }
        };
        const std::function noop = [](const std::shared_ptr<ActionExecutor>& action_executor) {
        };


        // Create the action and create a corresponding action executor
        auto action = std::shared_ptr<CustomAction>(
            new CustomAction("run_invocation_noop", 0, 0, initialization_code, noop));

        auto custom_message = new ServerlessComputeServiceContainerInitializationCompleteMessage(container, action);

        const auto action_executor = std::make_shared<ActionExecutor>(
            compute_node->hostname, 1, 0,
            this->getPropertyValueAsTimeInSecond(ServerlessComputeServiceProperty::CONTAINER_STARTUP_OVERHEAD),
            false,
            _commport,
            custom_message,
            action,
            nullptr);

        action_executor->setActionTimeout(function->getTimeLimit());
        action_executor->setSimulation(this->simulation_);
        // Start the action executor object
        try {
            action_executor->start(action_executor, true, false);
        } catch (ExecutionException&) {
            throw std::runtime_error(
                "ServerlessComputeService::prewarmContainerAtComputeNode(): Action executor could not be started - internal error");
        }

        WRENCH_INFO("Initiate a container pre-warm for function %s at node %s",
                    function->getName().c_str(), compute_node->hostname.c_str());
    }
} // namespace wrench
