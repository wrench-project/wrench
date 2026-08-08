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

#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_serverless_compute_node, "Log category for Serverless Compute Node");

namespace wrench {

    /**
    *  @brief Constructor
    *  @param h: hostname
    *  @param num_cores: number of cores
    */
    ServerlessComputeNode::ServerlessComputeNode(std::string h, unsigned int num_cores, ServerlessComputeService *service) :
                hostname(std::move(h)), total_cores(num_cores), available_cores(num_cores), serverless_compute_service(service) {}


    /**
     * @brief Make a container idle
     * @param container a container
     */
    void ServerlessComputeNode::makeContainerIdle(const std::shared_ptr<Container>& container) {
        if (this->busy_containers.find(container) == this->busy_containers.end()) {
            throw std::runtime_error("Trying to make a non-busy container idle");
        }
        busy_containers.erase(container);
        container->makeIdle();
        idle_containers.insert(container);
    }

    /**
     * @brief Make a container busy
     * @param container a container
     */
    void ServerlessComputeNode::makeContainerBusy(const std::shared_ptr<Container>& container) {
        if (this->idle_containers.find(container) == this->idle_containers.end()) {
            throw std::runtime_error("Trying to make a non-idle container busy");
        }
        idle_containers.erase(container);
        container->makeBusy();
        busy_containers.insert(container);
    }

    /**
     * @brief Shutdown a container
     * @param container a container
     */
    void ServerlessComputeNode::shutdownContainer(const std::shared_ptr<Container>& container) {
        if (this->busy_containers.find(container) != this->busy_containers.end()) {
            throw std::runtime_error("Trying to shutdown a non-idle container busy");
        }
        idle_containers.erase(container);
        container->shutdown();
    }

    /**
     * @brief Method to see if there is an appropriate idle container
     * @param registered_function the target function
     * @return A container, if found, or nullptr
     */
    std::shared_ptr<Container> ServerlessComputeNode::findIdleContainer(RegisteredFunction* registered_function) {
        for (auto const &idle_container : idle_containers) {
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
        auto container = std::shared_ptr<Container>(new Container(registered_function, this, this->serverless_compute_service, Container::State::BUSY));
        container->spawn();
        busy_containers.insert(container);
        return container;
    }


}; // namespace wrench
