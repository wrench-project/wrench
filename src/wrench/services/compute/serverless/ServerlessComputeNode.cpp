/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeNode.h>
#include <wrench/function/Invocation.h>
#include <wrench/function/Image.h>
#include <wrench/function/ImageLayer.h>
#include <wrench/services/compute/serverless/Container.h>
#include <wrench/services/storage/simple/SimpleStorageService.h>
#include <wrench/services/helper_services/alarm/Alarm.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/services/compute/serverless/ServerlessComputeServiceMessage.h>

#include "wrench/exceptions/ExecutionException.h"
#include "wrench/failure_causes/NotEnoughResources.h"

WRENCH_LOG_CATEGORY(wrench_core_serverless_compute_node, "Log category for Serverless Compute Node");

namespace wrench {
    /**
    *  @brief Constructor
    *  @param h: hostname
    *  @param service: the ServerlessComputeService that owns this compute node
    */
    ServerlessComputeNode::ServerlessComputeNode(std::string h,
                                                 ServerlessComputeService* service) :
        hostname(std::move(h)),
        _serverless_compute_service(service) {
        _max_num_containers_per_compute_node =
            service->getPropertyValueAsUnsignedLong(ServerlessComputeServiceProperty::MAX_NUM_CONTAINERS_PER_NODE);
        _max_num_invocations_per_compute_node =
            service->getPropertyValueAsUnsignedLong(ServerlessComputeServiceProperty::MAX_NUM_CONCURRENT_INVOCATIONS_PER_NODE);
        _max_num_invocations_per_container =
            service->getPropertyValueAsUnsignedLong(ServerlessComputeServiceProperty::MAX_NUM_CONCURRENT_INVOCATIONS_PER_CONTAINER);
    }



    /**
     * @brief Shutdown a container
     * @param container a container
     */
    void ServerlessComputeNode::shutdownContainer(const std::shared_ptr<Container>& container) {
        if (_initializing_containers.find(container) != _initializing_containers.end()) {
            throw std::runtime_error("Trying to shutdown a container that's initializing [not implemented for now]");
        }
        if (_initialized_containers.find(container) == _initialized_containers.end()) {
            throw std::runtime_error("Trying to shutdown some unknown container");
        }
        if (not container->_in_flight_invocations.empty()) {
            throw std::runtime_error("Trying to shutdown a container that has in-flight invocations [not implemented for now]");
        }
        _initialized_containers.erase(container);
        WRENCH_INFO("Shutting down a container for function [%s]", container->getFunction()->getName().c_str());
        container->shutdown();
    }

    /**
     * @brief Get the compute node's disk storage
     * @return A storage service
     */
    std::shared_ptr<SimpleStorageService> ServerlessComputeNode::getDiskStorage() const {
        return _disk;
    }

    /**
     * @brief Get the compute node's RAM storage
     * @return A storage service
     */
    std::shared_ptr<SimpleStorageService> ServerlessComputeNode::getMemoryStorage() const {
        return _memory;
    }

    /**
     * @brief Get the last access date for an in-RAM layer
     * @param layer The layer
     * @return a date
     */
    double ServerlessComputeNode::getLayerLastAccessDateInRAM(const std::shared_ptr<ImageLayer>& layer) const {
        if (not this->_memory->hasFile(layer->getRAMFile())) {
            throw std::runtime_error(
                "ServerlessComputeNode::getLayerLastAccessDateInRAM(): Layer " + layer->getName() + " is not in RAM");
        }
        return _memory->getLastAccessDate(FileLocation::LOCATION(_memory, layer->getRAMFile()));
    }

    /**
     * @brief Get the last access date for an in-disk layer
     * @param layer The layer
     * @return a date
     */
    double ServerlessComputeNode::getLayerLastAccessDateOnDisk(const std::shared_ptr<ImageLayer>& layer) const {
        if (not this->_disk->hasFile(layer->getFile())) {
            throw std::runtime_error(
                "ServerlessComputeNode::getLayerLastAccessDateOnDisk(): Layer " + layer->getName() + " is not on disk");
        }
        return _disk->getLastAccessDate(FileLocation::LOCATION(_disk, layer->getFile()));
    }

    /**
     * @brief Determine whether the compute node is at its container limit
     * @return true if no container could be started on this node, false otherwise
     */
    bool ServerlessComputeNode::isAtPerNodeContainerLimit() const {
        return (_initializing_containers.size() + _initialized_containers.size() >= _max_num_containers_per_compute_node);
    }

    /**
     * @brief Determine whether the compute node is at its invocation limit
     * @return true if no new invocation could be started on this node, false otherwise
     */
    bool ServerlessComputeNode::isAtPerNodeInvocationLimit() const {
        auto num_invocations = 0UL;
        for (auto const &container : _initialized_containers) {
            num_invocations += container->_in_flight_invocations.size();
        }
        return (num_invocations >= _max_num_invocations_per_compute_node);

    }

    /**
     * @brief Determine whether all containers on the compute node are at their invocation limit
     * @return true if no new invocation could be started on any container on this node, false otherwise
     */
    bool ServerlessComputeNode::areAllContainersAtInvocationLimit() const {
        for (auto const &container : _initialized_containers) {
            if (container->_in_flight_invocations.size() < _max_num_invocations_per_container) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Method to kill all containers (brutally)
     */
    void ServerlessComputeNode::killAllContainers() {
        // Initializing containers
        for (auto const& container : _initializing_containers) {
            container->shutdown();
        }
        // Ready containers
        for (auto const& container : _initialized_containers) {
            container->shutdown();
        }
    }

    // /**
    //  * @brief Method to see if there is an appropriate idle container
    //  * @param function the target function
    //  * @param excluded_container set containers to ignore
    //  * @return A container, if found, or nullptr
    //  */
    // std::shared_ptr<Container> ServerlessComputeNode::findIdleContainer(
    //     const Function* function,
    //     const std::set<std::shared_ptr<Container>>& excluded_container) const {
    //     for (auto const& idle_container : _idle_containers) {
    //         if (excluded_container.find(idle_container) != excluded_container.end()) {
    //             continue;
    //         }
    //         if (idle_container->getFunction() == function) {
    //             return idle_container;
    //         }
    //     }
    //     return nullptr;
    // }

    // /**
    //  * @brief Retrieve the set of idle containers
    //  * @return A set of containers
    //  */
    // std::set<std::shared_ptr<Container>>& ServerlessComputeNode::getIdleContainers() {
    //     return _idle_containers;
    // }
    //
    // /**
    //  * @brief Retrieve the set of busy containers
    //  * @return A set of containers
    //  */
    // std::set<std::shared_ptr<Container>>& ServerlessComputeNode::getBusyContainers() {
    //     return _busy_containers;
    // }

    /**
     * @brief Retrieve the set of initializing containers
     * @return A set of containers
     */
    std::set<std::shared_ptr<Container>>& ServerlessComputeNode::getInitializingContainers() {
        return _initializing_containers;
    }

    /**
     * @brief Retrieve the set of initialized containers
     * @return A set of containers
     */
    std::set<std::shared_ptr<Container>>& ServerlessComputeNode::getInitializedContainers() {
        return _initialized_containers;
    }


    /**
     * @brief Spawn a container (and try to kill idle containers if it helps)
     * @param function a function
     * @return A container
     */
    std::shared_ptr<Container> ServerlessComputeNode::spawnContainer(const Function* function) {

        if (_initialized_containers.size() + _initializing_containers.size() >= _max_num_containers_per_compute_node) {
            throw std::runtime_error("ServerlessComputeNode::spawnContainer(): Cannot overcome the per-compute node number of containers limit");
        }

        // Create a container object
        auto container = std::shared_ptr<Container>(
            new Container(function,
                          this,
                          _serverless_compute_service,
                          Container::State::INITIALIZING));
        try {
            container->spawn();
        } catch (ExecutionException& e) {
            throw;
        }

        _initializing_containers.insert(container);
        return container;
    }

    /**
     * @brief Make the compute node aware that an invocation has started
     * @param invocation An invocation
     */
    void ServerlessComputeNode::acknowledgeInvocationStart(const std::shared_ptr<Invocation>& invocation) {
        auto container = invocation->_container;
        container->_in_flight_invocations.insert(invocation);
        container->_idle_date = DBL_MAX;
        if (container->_expiration_alarm) {
            container->_expiration_alarm->kill();
        }
        container->_expiration_alarm.reset();
        container->_expiration_date = DBL_MAX;

        // A warm invocation uses these layers too. Refresh their recency
        // using the same zero-time touch as on a cold container start.
        for (const auto& file : container->_opened_image_layer_disk_files) {
            file->read(1, false);
        }
        for (const auto& file : container->_opened_image_layer_ram_files) {
            file->read(1, false);
        }
    }

    /**
     * @brief Make the compute node aware that an invocation has completed
     * @param invocation An invocation
     */
    void ServerlessComputeNode::acknowledgeInvocationCompletion(const std::shared_ptr<Invocation>& invocation) {
        auto container = invocation->_container;
        container->_in_flight_invocations.erase(invocation);

        // Start the idle timer?
        if (container->_in_flight_invocations.empty()) {
            double idle_timeout = _serverless_compute_service->getPropertyValueAsTimeInSecond(
                ServerlessComputeServiceProperty::CONTAINER_IDLE_TIMEOUT);
            if (idle_timeout <= 0) {
                this->shutdownContainer(container);
            } else {
                // Start the expiration alarm
                container->_expiration_date = S4U_Simulation::getClock() + idle_timeout;
                container->_idle_date = S4U_Simulation::getClock();
                container->_idle_sequence++;
                container->_expiration_alarm = Alarm::createAndStartAlarm(
                    _serverless_compute_service->getSimulation(),
                    container->_expiration_date,
                    _serverless_compute_service->_hostname,
                    _serverless_compute_service->getCommPort(),
                    new
                    ServerlessComputeServiceContainerIdleTimeoutMessage(
                        container, container->getIdleSequence()),
                    "container_idle_timout");
            }
        }
    }

    /**
     * @brief Get the compute node's free disk space
     * @return a number of bytes
     */
    sg_size_t ServerlessComputeNode::getFreeDiskSpace() const {
        return _disk->getTotalFreeSpaceZeroTime();
    }

    /**
     * @brief Get the compute node's free RAM space
     * @return a number of bytes
     */
    sg_size_t ServerlessComputeNode::getFreeRAMSpace() const {
        return _memory->getTotalFreeSpaceZeroTime();
    }

    /**
     * @brief Is an image layer on disk?
     * @param layer an image layer
     * @return True if the image is on disk
     */
    bool ServerlessComputeNode::isImageLayerOnDisk(const std::shared_ptr<ImageLayer>& layer) const {
        return this->_disk->hasFile(layer->getFile());
    }

    /**
     * @brief Is an image layer in RAM?
     * @param layer an image layer
     * @return True if the image layer is in RAM
     */
    bool ServerlessComputeNode::isImageLayerInRAM(const std::shared_ptr<ImageLayer>& layer) const {
        return (this->_memory->hasFile(layer->getRAMFile()));
    }


    /**
     * @brief Helper method to ensure that an invocation can be dispatched
     * @param invocation: the invocation to start
     * @param target_container: the target container (nullptr if none)
     * @return true if the invocation can be dispatched, false otherwise
     */
    bool ServerlessComputeNode::couldInvocationRunRightNow(
        const std::shared_ptr<Invocation>& invocation,
        const std::shared_ptr<Container>& target_container) const {
        // Sanity checks
        if (invocation->isDispatched()) {
            throw std::runtime_error(
                "ServerlessComputeNode::isInvocationFeasible(): The invocation has already been dispatched!");
        }
        if (target_container) {
            if (invocation->getFunction().get() != target_container->getFunction()) {
                throw std::runtime_error(
                    "ServerlessComputeNode::isInvocationFeasible(): The container isn't for the right function!");
            }
            if (_initialized_containers.find(target_container) == _initialized_containers.end()) {
                // The container could have been evicted due to another invocation dispatch
                return false;
            }
            if (target_container->isAtInvocationLimit()) {
                throw std::runtime_error(
                    "ServerlessComputeNode::isInvocationFeasible: Internal error - Scheduled invocation cannot be started because the target container is at the invocation limit");
            }
        }

        // The image is on disk?
        auto image = invocation->getFunction()->getImage();
        bool all_layers_on_disk = true;
        for (auto const& layer : image->getLayers()) {
            if (not this->_disk->hasFile(layer->getFile())) {
                all_layers_on_disk = false;
                break;
            }
        }

        if (not all_layers_on_disk) {
            WRENCH_INFO(
                "Scheduled invocation cannot be started because not all layers for image %s are on disk at node %s",
                image->getName().c_str(), this->hostname.c_str());
            return false;
        }

        // Is the image in RAM
        bool all_layers_in_ram = true;
        for (auto const& layer : image->getLayers()) {
            if (not this->_memory->hasFile(layer->getRAMFile())) {
                all_layers_in_ram = false;
                break;
            }
        }
        if (not all_layers_in_ram) {
            WRENCH_INFO(
                "Scheduled invocation cannot be started because not all layers for image %s are in RAM at node %s",
                image->getName().c_str(), this->hostname.c_str());
            return false;
        }

        // Are we within compute node limits in terms of number of invocations
        auto num_in_flight_invocations = 0UL;
        for (auto const &container : _initializing_containers) {
            num_in_flight_invocations += container->_in_flight_invocations.size();
        }
        for (auto const &container : _initialized_containers) {
            num_in_flight_invocations += container->_in_flight_invocations.size();
        }
        if (num_in_flight_invocations >= _max_num_invocations_per_compute_node) {
            return false;
        }

        // Are we within compute node limits in terms of number of containers
        if (target_container) {
            if (_initializing_containers.size() + _initialized_containers.size()
                >= _max_num_containers_per_compute_node) {
                return false;
            }
        }

        // Are we within container limits in terms of number of invocations
        if (target_container) {
            if (target_container->_in_flight_invocations.size() >= _max_num_invocations_per_container) {
                return false;
            }
        }

        return true;
    }
} // namespace wrench
