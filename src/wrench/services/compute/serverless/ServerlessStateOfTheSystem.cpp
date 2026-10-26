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

#include <utility>
#include <numeric>

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
        auto compute_nodes_initially_activated =
            service->getPropertyValueAsBoolean(ServerlessComputeServiceProperty::COMPUTE_NODES_INITIALLY_ACTIVATED);
        for (const auto& hostname : compute_hosts) {
            auto compute_node = std::make_shared<
                ServerlessComputeNode>(hostname, service);
            if (compute_nodes_initially_activated) {
                _activated_compute_nodes.push_back(compute_node);
            } else {
                _deactivated_compute_nodes.push_back(compute_node);
            }
            _image_layers_in_ram[compute_node] = {};
            _image_layers_being_loaded_in_ram[compute_node] = {};
            _image_layers_on_disk[compute_node] = {};
            _image_layers_being_copied_to_disk[compute_node] = {};
        }
    }

    /**
     * @brief Getter for the compute nodes
     * @return The compute nodes
     */
    std::vector<std::shared_ptr<ServerlessComputeNode>> ServerlessStateOfTheSystem::getActivatedComputeNodes() const {
        return _activated_compute_nodes;
    }

    /**
     * @brief Get the number of deactivated compute nodes
     * @return A number of nodes
     */
    unsigned long ServerlessStateOfTheSystem::getNumDeactivatedComputeNodes() const {
        return _deactivated_compute_nodes.size();
    }

    /**
     * @brief Get the number of deactivated compute nodes
     * @return A number of nodes
     */
    unsigned long ServerlessStateOfTheSystem::getNumBeingActivatedComputeNodes() const {
        return _being_activated_compute_nodes.size();
    }

    /**
     * @brief Getter for the map of available RAM for activated compute nodes. Note that RAM is managed in an LRU fashion, so it's
     *        not because RAM is full that nothing can be done, perhaps.
     *
     * @return The RAM availability map
     */
    std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> ServerlessStateOfTheSystem::getAvailableRAMSpace() const {
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> to_return;
        for (const auto& compute_node : _activated_compute_nodes) {
            to_return[compute_node] = compute_node->getMemoryStorage()->getTotalFreeSpaceZeroTime();
        }
        return to_return;
    }

    /**
     * @brief Getter for the map of available disk space for activated compute nodes. Note that Disk space is managed in an LRU fashion, so it's
     *        not because a disk is full that nothing can be done, perhaps.
     *
     * @return The RAM availability map
     */
    std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t>
    ServerlessStateOfTheSystem::getAvailableDiskSpace() const {
        std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> to_return;
        for (const auto& compute_node : _activated_compute_nodes) {
            to_return[compute_node] = compute_node->getDiskStorage()->getTotalFreeSpaceZeroTime();
        }
        return to_return;
    }

    /**
     * @brief Get the current image layers being copied to a node
     *
     * @param node the compute node
     * @return a set of image files
     */
    std::set<std::shared_ptr<ImageLayer>> ServerlessStateOfTheSystem::getImageLayersBeingCopiedToNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _image_layers_being_copied_to_disk.at(node);
    }

    /**
     * @brief Get the current image layers on disk at a node
     *
     * @param node the compute node
     * @return a set of image files
     */
    std::set<std::shared_ptr<ImageLayer>> ServerlessStateOfTheSystem::getImageLayersOnDiskAtNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _image_layers_on_disk.at(node);
    }

    /**
     * @brief Determine whether an image layer is currently being copied to a node
     *
     * @param node the compute node
     * @param layer an image layer
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageLayerBeingCopiedToNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                              const std::shared_ptr<ImageLayer>& layer) const {
        return _image_layers_being_copied_to_disk.at(node).count(layer) != 0;
    }

    /**
     * @brief Determine whether an image layer is currently on disk at a node
     *
     * @param node the compute node
     * @param layer an image layer
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageLayerOnDiskAtNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                   const std::shared_ptr<ImageLayer>& layer) const {
        return _image_layers_on_disk.at(node).count(layer) != 0;
    }

    /**
     * @brief Get the current image layers being loaded into RAM at a node
     *
     * @param node the compute node
     * @return a set of image layers
     */
    std::set<std::shared_ptr<ImageLayer>> ServerlessStateOfTheSystem::getImageLayersBeingLoadedAtNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _image_layers_being_loaded_in_ram.at(node);
    }

    /**
     * @brief Determine whether an image is currently being loading into RAM at a node
     *
     * @param node the compute node
     * @param layer an image layer
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageLayerBeingLoadedAtNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                              const std::shared_ptr<ImageLayer>& layer) const {
        return _image_layers_being_loaded_in_ram.at(node).count(layer) != 0;
    }

    /**
     * @brief Determine whether an image layer is currently in RAM at a node
     *
     * @param node the compute node
     * @param layer an image layer
     *
     * @return true or false
     */
    bool ServerlessStateOfTheSystem::isImageLayerInRAMAtNode(const std::shared_ptr<ServerlessComputeNode>& node,
                                                        const std::shared_ptr<ImageLayer>& layer) const {
        return _image_layers_in_ram.at(node).count(layer) != 0;
    }

    /**
     * @brief Get the current image layers in RAM at a node
     *
     * @param node the compute node
     * @return a set of image files
     */
    std::set<std::shared_ptr<ImageLayer>> ServerlessStateOfTheSystem::getImageLayersInRAMAtNode(
        const std::shared_ptr<ServerlessComputeNode>& node) const {
        return _image_layers_in_ram.at(node);
    }

    /**
     * @brief Get the list of admitted invocations (whose image layers are not all downloaded from remote)
     *
     * @return The list of admitted invocations
     */
    std::map<std::shared_ptr<Image>, std::vector<std::shared_ptr<Invocation>>> ServerlessStateOfTheSystem::getAdmittedInvocations() const {
        return _admitted_invocations;
    }

    /**
     * @brief Get the list of schedulable invocations
     *
     * @return The list of schedulable invocations
     */
    std::vector<std::shared_ptr<Invocation>> ServerlessStateOfTheSystem::getSchedulableInvocations() const {
        return _schedulable_invocations;
    }

    /**
     * @brief Get the current per-function outstanding invocation counts
     * @return a function-count map
     */
    std::map<std::shared_ptr<Function>, unsigned long> ServerlessStateOfTheSystem::
    getOutstandingInvocationCounts() const {
        return _outstanding_invocation_counts;
    }

} // namespace wrench
