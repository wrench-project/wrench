/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeNode.h>
#include <wrench/services/compute/serverless/Container.h>
#include <wrench/services/storage/simple/SimpleStorageService.h>

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_serverless_compute_node, "Log category for Serverless Compute Node");

namespace wrench {

    /**
    *  @brief Constructor
    *  @param h: hostname
    *  @param num_cores: number of cores
    */
    ServerlessComputeNode::ServerlessComputeNode(std::string h, unsigned int num_cores, ServerlessComputeService *service) :
                hostname(std::move(h)), total_cores(num_cores), available_cores(num_cores), _serverless_compute_service(service) {}


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
        container->shutdown();
    }

    /**
     * @brief Method to see if there is an appropriate idle container
     * @param registered_function the target function
     * @return A container, if found, or nullptr
     */
    std::shared_ptr<Container> ServerlessComputeNode::findIdleContainer(RegisteredFunction* registered_function) {
        for (auto const &idle_container : _idle_containers) {
            if (idle_container->getRegisteredFunction() == registered_function) {
                return idle_container;
            }
        }
        return nullptr;
    }

    /**
     * @brief Spawn a container
     * @param registered_function a registered function
     */
    std::shared_ptr<Container> ServerlessComputeNode::spawnContainer(RegisteredFunction *registered_function) {
        // Create a container object
        auto container = std::shared_ptr<Container>(new Container(registered_function, this, _serverless_compute_service, Container::State::BUSY));
        container->spawn();
        _busy_containers.insert(container);
        return container;
    }

    /**
     * @brief Is an image in the process of being copied to (the disk of) the compute node?
     * @param image an image file
     * @return True if the image is being copied
     */
    bool ServerlessComputeNode::isImageBeingCopied(const std::shared_ptr<DataFile>& image) const {
        return (_images_being_copied.find(image) != _images_being_copied.end());
    }

    /**
     * @brief Get the set of images being copied to (the disk of) the compute node
     * @return A set of images
     */
    std::set<std::shared_ptr<DataFile>> ServerlessComputeNode::getImagesBeingCopied() const {
        return _images_being_copied;
    }

    /**
     * @brief Is an image on disk?
     * @param image an image file
     * @return True if the image is on disk
     */
    bool ServerlessComputeNode::isImageOnDisk(const std::shared_ptr<DataFile>& image) const {
        return (this->disk->hasFile(image));
    }

    /**
     * @brief Is an image in the process of being loaded to (the RAM of) the compute node?
     * @param image an image file
     * @return True if the image is being loaded
     */
    bool ServerlessComputeNode::isImageBeingLoaded(const std::shared_ptr<DataFile>& image) const {
        return (_images_being_loaded.find(image) != _images_being_loaded.end());
    }

    /**
     * @brief Get the set of images being loaded to (the RAM of) the compute node
     * @return A set of images
     */
    std::set<std::shared_ptr<DataFile>> ServerlessComputeNode::getImagesBeingLoaded() const {
        return _images_being_loaded;
    }

    /**
     * @brief Is an image in RAM?
     * @param image an image file
     * @return True if the image is in RAM
     */
    bool ServerlessComputeNode::isImageInRAM(const std::shared_ptr<DataFile>& image) const {
        return (this->memory->hasFile(image));
    }


}; // namespace wrench
