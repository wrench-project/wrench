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
                hostname(std::move(h)), _total_cores(num_cores), _available_cores(num_cores), _serverless_compute_service(service) {}


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
     * @param excluded_container set containers to ignore
     * @return A container, if found, or nullptr
     */
    std::shared_ptr<Container> ServerlessComputeNode::findIdleContainer(
        const RegisteredFunction* registered_function,
        const std::set<std::shared_ptr<Container>>& excluded_container) const {
        for (auto const &idle_container : _idle_containers) {
            if (excluded_container.find(idle_container) != excluded_container.end()) {
                continue;
            }
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
        return (this->_disk->hasFile(image));
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
        return (this->_memory->hasFile(image));
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
            if (invocation->getRegisteredFunction().get() != target_container->getRegisteredFunction()) {
                throw std::runtime_error(
                    "ServerlessComputeNode::isInvocationFeasible(): The container isn't for the right function!");
            }
            if (_idle_containers.find(target_container) == _idle_containers.end()) {
                throw std::runtime_error(
                    "ServerlessComputeNode::isInvocationFeasible(): Internal error - The container does not belong to the compute host!");
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

        // The image is in RAM?
        auto ss_memory = this->_memory;
        auto image_file = invocation->getRegisteredFunction()->getImageFile();
        if (not ss_memory->hasFile(image_file, ss_memory->getBaseRootPath())) {
            WRENCH_INFO("Scheduled invocation cannot be started because image %s is not loaded at node %s",
                        image_file->getID().c_str(), this->hostname.c_str());
            return false;
        } else {
            std::cerr << "NODE " << this->hostname << " HAS IMAGE " << image_file->getID().c_str() << " IN RAM!" << std::endl;
        }

        return true;
    }

}; // namespace wrench
