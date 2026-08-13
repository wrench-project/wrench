/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/services/compute/serverless/ServerlessStateOfTheSystem.h>
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/storage/simple/SimpleStorageService.h"
#include "wrench/simgrid_S4U_util//S4U_Simulation.h"

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_serverless_state_of_the_system, "Log category for Serverless State of the System");

namespace wrench {
    /**
     * @brief Constructor
     * @param compute_hosts the list of compute hosts
     * @param service the ServerlessComputeService whose state this is
     */
    ServerlessStateOfTheSystem::ServerlessStateOfTheSystem(const std::vector<std::string>& compute_hosts,
                                                           ServerlessComputeService* service)
        : _head_storage_service(nullptr),
          _free_space_on_head_storage(0),
          _serverless_compute_service(service) {
        for (const auto& hostname : compute_hosts) {
            auto num_cores = S4U_Simulation::getHostNumCores(hostname);
            auto compute_node = std::make_shared<
                ServerlessComputeNode>(hostname, num_cores, service);
            _compute_nodes.push_back(compute_node);
        }
    }

    /**
     * @brief Getter for the compute hosts
     * @return The compute hosts
     */
    std::vector<std::shared_ptr<ServerlessComputeNode>> ServerlessStateOfTheSystem::getComputeNodes() const {
        return _compute_nodes;
    }

    /**
     * @brief Getter for the map of available cores
     *
     * @return The core availability map
     */
    std::map<std::shared_ptr<ServerlessComputeNode>, unsigned int>
    ServerlessStateOfTheSystem::getAvailableCores() const {
        std::map<std::shared_ptr<ServerlessComputeNode>, unsigned int> to_return;
        for (const auto& compute_node : _compute_nodes) {
            to_return[compute_node] = compute_node->getNumIdleCores();
        }
        return to_return;
    }

    /**
     * @brief Getter for the map of available RAM. Note that RAM is managed in an LRU fashion, so it's
     *        not because RAM is full that nothing can be done, perhaps.
     *
     * @return The RAM availability map
     */
    std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> ServerlessStateOfTheSystem::getAvailableRAMSpace() const {
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> to_return;
        for (const auto& compute_node : _compute_nodes) {
            to_return[compute_node] = compute_node->getMemoryStorage()->getTotalFreeSpaceZeroTime();
        }
        return to_return;
    }

    /**
     * @brief Getter for the map of available disk space. Note that Disk space is managed in an LRU fashion, so it's
     *        not because a disk is full that nothing can be done, perhaps.
     *
     * @return The RAM availability map
     */
    std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t>
    ServerlessStateOfTheSystem::getAvailableDiskSpace() const {
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> to_return;
        for (const auto& compute_node : _compute_nodes) {
            to_return[compute_node] = compute_node->getDiskStorage()->getTotalFreeSpaceZeroTime();
        }
        return to_return;
    }

    /**
     * @brief Get the current images being copied to a node
     *
     * @param node the compute node
     * @return a set of image files
     */
    std::set<std::shared_ptr<Image>> ServerlessStateOfTheSystem::getImagesBeingCopiedToNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return node->getImagesBeingCopied();
    }

    /**
     * @brief Determine whether an image is currently being copied to a node
     *
     * @param node the compute node
     * @param image an image file
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageBeingCopiedToNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                              const std::shared_ptr<Image>& image) const {
        return (node->isImageBeingCopied(image));
    }

    /**
     * @brief Determine whether an image is currently on disk at a node
     *
     * @param node the compute node
     * @param image an image
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageOnDiskAtNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                   const std::shared_ptr<Image>& image) const {
        return node->isImageOnDisk(image);
    }

    /**
     * @brief Get the current images being loaded into RAM at a node
     *
     * @param node the compute node
     * @return a set of images
     */
    std::set<std::shared_ptr<Image>> ServerlessStateOfTheSystem::getImagesBeingLoadedAtNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return node->getImagesBeingLoaded();
    }

    /**
     * @brief Determine whether an image is currently being loading into RAM at a node
     *
     * @param node the compute node
     * @param image an image file
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageBeingLoadedAtNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                              const std::shared_ptr<Image>& image) const {
        return node->isImageBeingLoaded(image);
    }

    /**
     * @brief Determine whether an image is currently in RAM at a node
     *
     * @param node the compute node
     * @param image an image file
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageInRAMAtNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                        const std::shared_ptr<Image>& image) const {
        return node->isImageInRAM(image);
    }
} // namespace wrench
