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
    ServerlessComputeNode::ServerlessComputeNode(std::string h, unsigned int num_cores,
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
                    container->getRegisteredFunction()->getName().c_str());
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
        for (auto const& idle_container : _idle_containers) {
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
     * @brief Retrieve the set of idle container
     * @return A set of containers
     */
    std::set<std::shared_ptr<Container>> ServerlessComputeNode::getIdleContainers() const {
        return _idle_containers;
    }

    /**
     * @brief Spawn a container (and try to kill idle containers if it helps)
     * @param registered_function a registered function
     */
    std::shared_ptr<Container> ServerlessComputeNode::spawnContainer(const RegisteredFunction* registered_function) {
        // Create a container object
        auto container = std::shared_ptr<Container>(
            new Container(registered_function, this, _serverless_compute_service, Container::State::BUSY));
        try {
            container->spawn();
        } catch (ExecutionException& e) {
            if (not std::dynamic_pointer_cast<NotEnoughResources>(e.getCause())) {
                throw;
            }
            // Try to terminate idle containers
            std::set<std::shared_ptr<Container>> victims;
            auto success = this->findIdleContainersToTerminate(
                registered_function->getRAMSpaceLimit(),
                registered_function->getDiskSpaceLimit(),
                victims);
            if (not success) {
                throw;
            } else {
                for (auto const& victim : victims) {
                    WRENCH_INFO("Evicting an idle container [%s, idle for %.2lf seconds, %llu bytes in RAM, %llu bytes on disk",
                        victim->getRegisteredFunction()->getName().c_str(),
                        S4U_Simulation::getClock() - victim->getIdleDate(),
                        victim->getRegisteredFunction()->getRAMSpaceLimit(),
                        victim->getRegisteredFunction()->getDiskSpaceLimit());
                    this->shutdownContainer(victim);
                }
            }
            // Attempt again!
            try {
                container->spawn();
            } catch (ExecutionException&) {
                throw;
            }
        }
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

        // Is there enough space

        return true;
    }

    /**
     * @brief Method to identify which idle containers to terminate to create at least some free RAM space. It
     *        returns the smallest set possible in terms of number of containers to terminate, trying to
     *        free up as little memory as possible (it's not optimal in this regard however, as finding
     *        the smallest set with the smallest sum is NP-hard, and would require dynamic programming, etc.)
     *        This is a really a super-generic method that is implementing an algorithm, and it's not Container-specific
     * @param needed_free_ram_space The RAM space needed in bytes
     * @param needed_free_disk_space The Disk space needed in bytes
     * @param to_terminate The set of containers to terminate (reference, will be updated)
     * @return a set of idle containers that could be terminated to reach free space
     */
    bool ServerlessComputeNode::findIdleContainersToTerminate(sg_size_t needed_free_ram_space,
                                                              sg_size_t needed_free_disk_space,
                                                              std::set<std::shared_ptr<Container>>& to_terminate)
    const {
        // Compute numbers of bytes to be freed up
        auto ram_space_to_free_up = (needed_free_ram_space <= this->getFreeRAMSpace()
                                         ? 0
                                         : needed_free_ram_space - this->getFreeRAMSpace());
        auto disk_space_to_free_up = (needed_free_disk_space <= this->getFreeDiskSpace()
                                          ? 0
                                          : needed_free_disk_space - this->getFreeDiskSpace());

        // If nothing to be done, return
        if (ram_space_to_free_up == 0 and disk_space_to_free_up == 0) {
            return true;
        }

        auto policy = _serverless_compute_service->getPropertyValueAsString(
            ServerlessComputeServiceProperty::IDLE_CONTAINER_EVICTION_POLICY);
        bool success;
        if (policy == "LRU") {
            return this->pickVictimContainersLRU(ram_space_to_free_up, disk_space_to_free_up, to_terminate);
        } else if (policy == "RAM") {
            return this->pickVictimContainersRAM(ram_space_to_free_up, disk_space_to_free_up, to_terminate);
        }
        throw std::invalid_argument("ServerlessComputeNode::findIdleContainersToTerminate():"
            " invalid idle container eviction policy '" + policy + "'");
    }

    /**
     * @brief Pick victim idle containers to terminate using the RAM policy
     * @param to_terminate The set of containers to terminate (reference, will be updated)
     * @param ram_space_to_free_up The number of bytes to free up in RAM
     * @param disk_space_to_free_up The number of bytes to free up in disk
     * @return
     */
    bool ServerlessComputeNode::pickVictimContainersRAM(
        sg_size_t ram_space_to_free_up,
        sg_size_t disk_space_to_free_up,
        std::set<std::shared_ptr<Container>>& to_terminate) const {
        // Compute a to-sort list of the containers
        std::vector<std::shared_ptr<Container>> sorted_containers;
        sorted_containers.reserve(_idle_containers.size());
        for (auto const& container : _idle_containers) {
            sorted_containers.push_back(container);
        }

        // Sort the list
        std::sort(sorted_containers.begin(), sorted_containers.end(),
                  [ram_space_to_free_up](const std::shared_ptr<Container>& a,
                                                                const std::shared_ptr<Container>& b) {
                      // If RAM doesn't matter, sort based on disk
                      if (ram_space_to_free_up == 0) {
                          return a->getRegisteredFunction()->getDiskSpaceLimit() <
                              b->getRegisteredFunction()->getDiskSpaceLimit();
                      }

                      // Otherwise sort based on RAM (which should be the limiting factor)
                      return a->getRegisteredFunction()->getRAMSpaceLimit() <
                          b->getRegisteredFunction()->getRAMSpaceLimit();
                  });

        // TODO: Use dynamic programming to return some optimal set? (smallest cardinal, and smallest sum, NP-hard, but likely only weakly,
        // TODO: but not easy due to the two dimensions...Like some knapscak) - LIKELY OVERKILL
        // sg_size_t ram_space_freed_up = 0;
        // sg_size_t disk_space_freed_up = 0;
        std::cerr << "LOOPING TO FIND VICTIMS IN THE RAM POLICY\n";
        while (true) {
            std::cerr << "IN LOOP: TO FREE UP " << ram_space_to_free_up << "\n";
            if (sorted_containers.empty()) {
                to_terminate.clear();
                return false;
            }
            // Find the smallest container that gets us there, and if none, pick the largest container
            std::cerr << "Find the smallest container that gets us there, and if none, pick the largest container\n";
            auto victim = sorted_containers.end()-1;
            for (auto it = sorted_containers.begin(); it != sorted_containers.end(); ++it) {
                std::cerr << "  LOOKING AT A CONTAINER WITH RAM " << (*it)->getRegisteredFunction()->getRAMSpaceLimit() << " (TO FREE UP: " << ram_space_to_free_up << ")\n";
                if ((*it)->getRegisteredFunction()->getRAMSpaceLimit() >= ram_space_to_free_up and
                    (*it)->getRegisteredFunction()->getDiskSpaceLimit() >= disk_space_to_free_up) {
                    victim = it;
                    break;
                }
            }
            std::cerr << "  CONTAINER FOUND: " << (*victim)->getRegisteredFunction()->getRAMSpaceLimit() << "\n";

            to_terminate.insert(*victim);
            auto victim_ram_space = (*victim)->getRegisteredFunction()->getRAMSpaceLimit();
            auto victim_disk_space = (*victim)->getRegisteredFunction()->getDiskSpaceLimit();
            ram_space_to_free_up = (victim_ram_space > ram_space_to_free_up ? 0 : ram_space_to_free_up - victim_ram_space);
            disk_space_to_free_up = (victim_disk_space > disk_space_to_free_up ? 0 : disk_space_to_free_up - victim_disk_space);
            sorted_containers.erase(victim);

            if ((ram_space_to_free_up == 0) and (disk_space_to_free_up == 0)) {
                break;
            }
        }
        return true;
    }

    /**
    * @brief Pick victim idle containers to terminate using the LRU policy
    * @param ram_space_to_free_up The number of bytes to free up in RAM
    * @param disk_space_to_free_up The number of bytes to free up in disk
    * @param to_terminate The set of containers to terminate (reference, will be updated)
    * @return
    */
    bool ServerlessComputeNode::pickVictimContainersLRU(
        sg_size_t ram_space_to_free_up,
        sg_size_t disk_space_to_free_up,
        std::set<std::shared_ptr<Container>>& to_terminate) const {
        // Compute a to-sort list of the containers
        std::vector<std::shared_ptr<Container>> sorted_containers;
        sorted_containers.reserve(_idle_containers.size());
        for (auto const& container : _idle_containers) {
            sorted_containers.push_back(container);
        }

        // Sort the list
        std::sort(sorted_containers.begin(), sorted_containers.end(),
                  [](
                  const std::shared_ptr<Container>& a, const std::shared_ptr<Container>& b) {
                      double a_time_since_idle = S4U_Simulation::getClock() - a->getIdleDate();
                      double b_time_since_idle = S4U_Simulation::getClock() - b->getIdleDate();
                      return a_time_since_idle > b_time_since_idle;
                  });

        // Go through the list
        sg_size_t ram_space_freed_up = 0;
        sg_size_t disk_space_freed_up = 0;
        for (auto const& container : sorted_containers) {
            to_terminate.insert(container);
            ram_space_freed_up += container->getRegisteredFunction()->getRAMSpaceLimit();
            disk_space_freed_up += container->getRegisteredFunction()->getDiskSpaceLimit();
            if ((ram_space_freed_up >= ram_space_to_free_up) and (disk_space_freed_up >= disk_space_to_free_up)) {
                return true;
            }
        }
        to_terminate.clear();
        return false;

    }
} // namespace wrench
