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
     */
    ServerlessStateOfTheSystem::ServerlessStateOfTheSystem(const std::vector<std::string>& compute_hosts)
        : _compute_hosts(compute_hosts),
          _head_storage_service(nullptr),
          _free_space_on_head_storage(0) {
        for (const auto& compute_host : _compute_hosts) {
            _available_cores[compute_host] = S4U_Simulation::getHostNumCores(compute_host);
            _available_ram[compute_host] = S4U_Simulation::getHostMemoryCapacity(compute_host);
        }

        for (const auto& compute_host : _compute_hosts) {
            _being_copied_images[compute_host] = {};
        }
    }

    /**
     * @brief Getter for the compute hosts
     * @return The compute hosts
     */
    const std::vector<std::string>& ServerlessStateOfTheSystem::getComputeHosts() {
        return _compute_hosts;
    }

    /**
     * @brief Getter for the map of available cores
     *
     * @return The core availability map
     */
    std::map<std::string, unsigned long> ServerlessStateOfTheSystem::getAvailableCores() {
        return _available_cores;
    }

    /**
     * @brief Getter for the map of available RAM. Note that RAM is managed in an LRU fashion, so it's
     *        not because RAM is full that nothing can be done, perhaps.
     *
     * @return The RAM availability map
     */
    std::map<std::string, sg_size_t> ServerlessStateOfTheSystem::getAvailableRAM() {
        std::map<std::string, sg_size_t> to_return;
        for (const auto& [hostname, ss] : _compute_memories) {
            to_return[hostname] = ss->getTotalFreeSpaceZeroTime();
        }
        return to_return;
    }

    /**
     * @brief Getter for the map of available disk space. Note that Disk space is managed in an LRU fashion, so it's
     *        not because a disk is full that nothing can be done, perhaps.
     *
     * @return The RAM availability map
     */
    std::map<std::string, sg_size_t> ServerlessStateOfTheSystem::getAvailableDiskSpace() {
        std::map<std::string, sg_size_t> to_return;
        for (const auto& [hostname, ss] : _compute_storages) {
            to_return[hostname] = ss->getTotalFreeSpaceZeroTime();
        }
        return to_return;
    }

    /**
     * @brief Get the current images being copied to a node
     *
     * @param node the compute node
     * @return a set of image files
     */
    std::set<std::shared_ptr<DataFile>> ServerlessStateOfTheSystem::getImagesBeingCopiedToNode(const std::string& node) {
        if (_being_copied_images.find(node) != _being_copied_images.end()) {
            return _being_copied_images[node];
        }
        return {};
    }

    /**
     * @brief Determine whether an image is currently being copied to a node
     *
     * @param node the compute node
     * @param image an image file
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageBeingCopiedToNode(const std::string& node,
                                                              const std::shared_ptr<DataFile>& image) {
        // Check if the image has been copied to the node
        if (_being_copied_images.find(node) != _being_copied_images.end()) {
            if (_being_copied_images[node].find(image) != _being_copied_images[node].end()) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Determine whether an image is currently on disk at a node
     *
     * @param node the compute node
     * @param image an image file
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageOnNode(const std::string& node, const std::shared_ptr<DataFile>& image) {
        return _compute_storages[node]->hasFile(image);
    }

    /**
     * @brief Get the current images being loaded into RAM at a node
     *
     * @param node the compute node
     * @return a set of image files
     */
    std::set<std::shared_ptr<DataFile>> ServerlessStateOfTheSystem::getImagesBeingLoadedAtNode(const std::string& node) {
        if (_being_loaded_images.find(node) != _being_loaded_images.end()) {
            return _being_loaded_images[node];
        }
        return {};
    }

    /**
     * @brief Determine whether an image is currently being loading into RAM at a node
     *
     * @param node the compute node
     * @param image an image file
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageBeingLoadedAtNode(const std::string& node,
                                                        const std::shared_ptr<DataFile>& image) {
        // Check if the image has been copied to the node
        if (_being_loaded_images.find(node) != _being_loaded_images.end()) {
            if (_being_loaded_images[node].find(image) != _being_loaded_images[node].end()) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Determine whether an image is currently in RAM at a node
     *
     * @param node the compute node
     * @param image an image file
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageInRAMAtNode(const std::string& node, const std::shared_ptr<DataFile>& image) {
        return _compute_memories[node]->hasFile(image, _compute_memories[node]->getBaseRootPath());
    }
}; // namespace wrench
