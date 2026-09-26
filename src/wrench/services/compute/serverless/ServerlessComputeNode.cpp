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

#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/serverless/ServerlessComputeService.h>

#include "wrench/exceptions/ExecutionException.h"
#include "wrench/failure_causes/NotEnoughResources.h"

WRENCH_LOG_CATEGORY(wrench_core_serverless_compute_node, "Log category for Serverless Compute Node");

namespace wrench {
    /**
    *  @brief Constructor
    *  @param h: hostname
    *  @param num_cores: number of cores
    *  @param service: the ServerlessComputeService that owns this compute node
    */
    ServerlessComputeNode::ServerlessComputeNode(std::string h, const unsigned int num_cores,
                                                 ServerlessComputeService* service) :
        hostname(std::move(h)), _serverless_compute_service(service), _total_cores(num_cores),
        _available_cores(num_cores) {
    }


    /**
     * @brief Make a container idle
     * @param container a container
     */
    void ServerlessComputeNode::makeContainerIdle(const std::shared_ptr<Container>& container) {
        if (_busy_containers.find(container) == _busy_containers.end()) {
            throw std::runtime_error("Trying to make a non-busy container idle");
        }
        _busy_containers.erase(container);
        container->makeIdle();
        _idle_containers.insert(container);
    }

    /**
     * @brief Make a container busy
     * @param container a container
     */
    void ServerlessComputeNode::makeContainerBusy(const std::shared_ptr<Container>& container) {
        if (_idle_containers.find(container) == _idle_containers.end()) {
            throw std::runtime_error("Trying to make a non-idle container busy");
        }
        _idle_containers.erase(container);
        container->makeBusy();
        _busy_containers.insert(container);
    }

    /**
     * @brief Shutdown a container
     * @param container a container
     */
    void ServerlessComputeNode::shutdownContainer(const std::shared_ptr<Container>& container) {
        if (_busy_containers.find(container) != _busy_containers.end()) {
            throw std::runtime_error("Trying to shutdown a busy container");
        }
        if (_idle_containers.find(container) == _idle_containers.end()) {
            throw std::runtime_error("Trying to shutdown a container that's not in the idle list?");
        }
        _idle_containers.erase(container);
        WRENCH_INFO("Shutting down an idle container for function [%s]",
                    container->getFunction()->getName().c_str());
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
            throw std::runtime_error("ServerlessComputeNode::getLayerLastAccessDateInRAM(): Layer " + layer->getName() + " is not in RAM");
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
            throw std::runtime_error("ServerlessComputeNode::getLayerLastAccessDateOnDisk(): Layer " + layer->getName() + " is not on disk");
        }
        return _disk->getLastAccessDate(FileLocation::LOCATION(_disk, layer->getFile()));
    }

    /**
     * @brief Method to kill all containers (brutally)
     */
    void ServerlessComputeNode::killAllContainers() {
        // Idle containers
        for (auto const &container : _idle_containers) {
            container->shutdown();
        }
        // Busy containers
        for (auto const &container : _busy_containers) {
            container->makeIdle();
            container->shutdown();
        }
    }

    /**
     * @brief Method to see if there is an appropriate idle container
     * @param function the target function
     * @param excluded_container set containers to ignore
     * @return A container, if found, or nullptr
     */
    std::shared_ptr<Container> ServerlessComputeNode::findIdleContainer(
        const Function* function,
        const std::set<std::shared_ptr<Container>>& excluded_container) const {
        for (auto const& idle_container : _idle_containers) {
            if (excluded_container.find(idle_container) != excluded_container.end()) {
                continue;
            }
            if (idle_container->getFunction() == function) {
                return idle_container;
            }
        }
        return nullptr;
    }

    /**
     * @brief Retrieve the set of idle containers
     * @return A set of containers
     */
    std::set<std::shared_ptr<Container>>& ServerlessComputeNode::getIdleContainers() {
        return _idle_containers;
    }

    /**
     * @brief Retrieve the set of non-idle containers
     * @return A set of containers
     */
    std::set<std::shared_ptr<Container>>& ServerlessComputeNode::getBusyContainers() {
        return _busy_containers;
    }

    /**
     * @brief Spawn a container (and try to kill idle containers if it helps)
     * @param function a function
     * @return A container
     */
    std::shared_ptr<Container> ServerlessComputeNode::spawnContainer(const Function* function) {
        // Create a container object
        auto container = std::shared_ptr<Container>(
            new Container(function, this, _serverless_compute_service, Container::State::BUSY));
        try {
            container->spawn();
        } catch (ExecutionException& e) {
            throw;
            // // Try to terminate idle containers
            // std::set<std::shared_ptr<Container>> victims;
            // auto success = this->findIdleContainersToTerminate(
            //     function->getRAMSpaceLimit(),
            //     function->getDiskSpaceLimit(),
            //     victims);
            // if (not success) {
            //     throw;
            // } else {
            //     for (auto const& victim : victims) {
            //         WRENCH_INFO(
            //             "Evicting an idle container [%s, idle for %.2lf seconds, %llu bytes in RAM, %llu bytes on disk",
            //             victim->getFunction()->getName().c_str(),
            //             S4U_Simulation::getClock() - victim->getIdleDate(),
            //             victim->getFunction()->getRAMSpaceLimit(),
            //             victim->getFunction()->getDiskSpaceLimit());
            //         this->shutdownContainer(victim);
            //     }
            // }
            // // Attempt again!
            // try {
            //     container->spawn();
            // } catch (ExecutionException&) {
            //     throw;
            // }
        }
        _busy_containers.insert(container);
        return container;
    }

    /**
     * @brief Get the compute node's number of cores
     * @return a number of cores
     */
    unsigned int ServerlessComputeNode::getNumCores() const {
        return _total_cores;
    }

    /**
     * @brief Get the compute node's number of idle cores
     * @return a number of cores
     */
    unsigned int ServerlessComputeNode::getNumIdleCores() const {
        return _available_cores;
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
    bool ServerlessComputeNode::isInvocationFeasible(
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
            if (_idle_containers.find(target_container) == _idle_containers.end()) {
                // The container could have been evicted due to another invocation dispatch
                return false;
            }
            if (not target_container->isIdle()) {
                throw std::runtime_error(
                    "ServerlessComputeNode::isInvocationFeasible: Internal error - Scheduled invocation cannot be started because the target container is not idle");
            }
        }

        // The node has available cores?
        if (this->_available_cores < 1) {
            WRENCH_INFO("Scheduled invocation cannot be started because there is no available core");
            return false;
        }

        auto image = invocation->getFunction()->getImage();

        // The image is on disk?
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

        // Is there enough space

        return true;
    }

} // namespace wrench
