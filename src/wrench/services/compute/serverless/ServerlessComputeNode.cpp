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
    *  @param service: the ServerlessComputeService that owns this compute node
    */
    ServerlessComputeNode::ServerlessComputeNode(std::string h, unsigned int num_cores, ServerlessComputeService *service) :
                hostname(std::move(h)), _serverless_compute_service(service), _total_cores(num_cores), _available_cores(num_cores) {}


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
     * @brief Method to identify which idle containers to terminate to create at least some free RAM space. It
     *        returns the smallest set possible in terms of number of containers to terminate, trying to
     *        free up as little memory as possible (it's not optimal in this regard however, as finding
     *        the smallest set with the smallest sum is NP-hard, and would require dynamic programming, etc.)
     * @param needed_free_ram_space The RAM space needed in bytes
     * @param to_terminate The set of containers to terminate
     * @return a set of idle containers that could be terminated to reach free space
     */
    bool ServerlessComputeNode::findIdleContainersToTerminate(sg_size_t needed_free_ram_space,
                                                              std::set<std::shared_ptr<Container>>& to_terminate) {
        // Perhaps nothing needs to be done
        if (needed_free_ram_space <= getFreeRAMSpace()) {
            return true;
        }

        // Compute the space to free_up
        auto space_to_free_up = needed_free_ram_space - getFreeRAMSpace();

        // Compute a list of the containers, sorted by increasing RAM footprint
        std::vector<std::shared_ptr<Container>> sorted_containers;
        sorted_containers.reserve(_idle_containers.size());
        for (auto const &container : _idle_containers) {
            sorted_containers.push_back(container);
        }
        std::sort(sorted_containers.begin(), sorted_containers.end(),
            [](const std::shared_ptr<Container>& a, const std::shared_ptr<Container>& b) {
                return a->getRegisteredFunction()->getRAMSpaceLimit() < b->getRegisteredFunction()->getRAMSpaceLimit();
        });

        // TODO: Use dynamic programming to return the optimal set? (smallest cardinal, and smallest sum) - LIKELY OVERKILL
        sg_size_t space_freed_up = 0;
        while (space_freed_up < space_to_free_up) {
            if (sorted_containers.empty()) {
                to_terminate.clear();
                return false;
            }
            // Find the smallest container that gets us there, and if non, pick the largest container
            auto victim = sorted_containers.end();
            for (auto it = sorted_containers.begin(); it != sorted_containers.end(); ++it) {
                if ((*it)->getRegisteredFunction()->getRAMSpaceLimit() >= space_to_free_up) {
                    victim = it;
                    break;
                }
            }
            to_terminate.insert(*victim);
            space_freed_up += (*victim)->getRegisteredFunction()->getRAMSpaceLimit();
            sorted_containers.erase(victim);
        }
        return true;
    }

    /**
     * @brief Spawn a container
     * @param registered_function a registered function
     */
    std::shared_ptr<Container> ServerlessComputeNode::spawnContainer(const RegisteredFunction *registered_function) {
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
    bool ServerlessComputeNode::isImageBeingCopied(const std::shared_ptr<Image>& image) const {
        return (_images_being_copied.find(image) != _images_being_copied.end());
    }

    /**
     * @brief Get the set of images being copied to (the disk of) the compute node
     * @return A set of images
     */
    std::set<std::shared_ptr<Image>> ServerlessComputeNode::getImagesBeingCopied() const {
        return _images_being_copied;
    }

    /**
     * @brief Is an image on disk?
     * @param image an image
     * @return True if the image is on disk
     */
    bool ServerlessComputeNode::isImageOnDisk(const std::shared_ptr<Image>& image) const {
        return (this->_disk->hasFile(image->getFile()));
    }

    /**
     * @brief Is an image in the process of being loaded to (the RAM of) the compute node?
     * @param image an image file
     * @return True if the image is being loaded
     */
    bool ServerlessComputeNode::isImageBeingLoaded(const std::shared_ptr<Image>& image) const {
        return (_images_being_loaded.find(image) != _images_being_loaded.end());
    }

    /**
     * @brief Get the set of images being loaded to (the RAM of) the compute node
     * @return A set of images
     */
    std::set<std::shared_ptr<Image>> ServerlessComputeNode::getImagesBeingLoaded() const {
        return _images_being_loaded;
    }

    /**
     * @brief Is an image in RAM?
     * @param image an image file
     * @return True if the image is in RAM
     */
    bool ServerlessComputeNode::isImageInRAM(const std::shared_ptr<Image>& image) const {
        return (this->_memory->hasFile(image->getRAMFile()));
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

        // The image is on disk?
        auto image = invocation->getRegisteredFunction()->getImage();

        if (not this->isImageOnDisk(image)) {
            WRENCH_INFO("Scheduled invocation cannot be started because image %s is not on disk at node %s",
                        image->getName().c_str(), this->hostname.c_str());
            return false;
        }

        // Is image in RAM
        if (not this->isImageInRAM(image)) {
            WRENCH_INFO("Scheduled invocation cannot be started because image %s is not in RAM at node %s",
                        image->getName().c_str(), this->hostname.c_str());
            return false;
        }

        return true;
    }

} // namespace wrench
