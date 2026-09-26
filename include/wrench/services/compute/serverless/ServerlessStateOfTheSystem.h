/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SERVERLESSSTATEOFTHESYSTEM_H
#define WRENCH_SERVERLESSSTATEOFTHESYSTEM_H

#include <vector>
#include <map>
#include <queue>
#include <set>
#include <unordered_set>
#include <memory>
#include <string>
#include <wrench/services/compute/serverless/ServerlessComputeNode.h>
#include <wrench/function/Invocation.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/data_file/DataFile.h>

namespace wrench
{

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A class that stores the current state of a serverless compute service
     */
    class ServerlessStateOfTheSystem {

    public:
        [[nodiscard]] std::vector<std::shared_ptr<ServerlessComputeNode>> getComputeNodes() const;
        [[nodiscard]] std::map<std::shared_ptr<ServerlessComputeNode>, unsigned int> getAvailableCores() const;
        [[nodiscard]] std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> getAvailableRAMSpace() const;
        [[nodiscard]] std::map<std::shared_ptr<ServerlessComputeNode>, sg_size_t> getAvailableDiskSpace() const;

        [[nodiscard]] std::set<std::shared_ptr<ImageLayer>> getImageLayersBeingCopiedToNode(const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool isImageLayerOnDiskAtNode(const std::shared_ptr<ServerlessComputeNode> &node, const std::shared_ptr<ImageLayer> &layer) const;
        [[nodiscard]] bool isImageLayerBeingCopiedToNode(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<ImageLayer>&layer) const;
        std::set<std::shared_ptr<ImageLayer>> getImageLayersOnDiskAtNode(const std::shared_ptr<ServerlessComputeNode>& node) const;

        [[nodiscard]] std::set<std::shared_ptr<ImageLayer>> getImageLayersBeingLoadedAtNode(const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool isImageLayerInRAMAtNode(const std::shared_ptr<ServerlessComputeNode> &node, const std::shared_ptr<ImageLayer> &layer) const;
        [[nodiscard]] bool isImageLayerBeingLoadedAtNode(const std::shared_ptr<ServerlessComputeNode> &node, const std::shared_ptr<ImageLayer> &layer) const;
        std::set<std::shared_ptr<ImageLayer>> getImageLayersInRAMAtNode(const std::shared_ptr<ServerlessComputeNode>& node) const;


        ~ServerlessStateOfTheSystem() = default;

    private:
        friend class ServerlessComputeService;

        explicit ServerlessStateOfTheSystem(const std::vector<std::string>& compute_hosts, ServerlessComputeService *service);

        // set of (registered)) functions
        std::set<std::shared_ptr<Function>> _functions;

        // queue of function invocations waiting to be processed
        std::queue<std::shared_ptr<Invocation>> _new_invocations;
        // queues of function invocations whose image layers are being downloaded
        std::map<std::shared_ptr<Image>, std::queue<std::shared_ptr<Invocation>>> _admitted_invocations;
        // queue of function invocations whose images have been downloaded
        std::vector<std::shared_ptr<Invocation>> _schedulable_invocations;
        // set of function invocations currently running
        std::unordered_set<std::shared_ptr<Invocation>> _running_invocations;

        std::string _head_storage_service_mount_point;
        std::shared_ptr<StorageService> _head_storage_service;
        std::set<std::shared_ptr<ImageLayer>> _being_downloaded_image_layers;
        sg_size_t _free_space_on_head_storage; // We keep track of it ourselves to avoid concurrency shenanigans

        // list of compute nodes
        std::vector<std::shared_ptr<ServerlessComputeNode>> _compute_nodes;

        // Layer tracking
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_in_ram;
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_being_loaded_in_ram;
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_on_disk;
        std::map<std::shared_ptr<ServerlessComputeNode>, std::set<std::shared_ptr<ImageLayer>>> _image_layers_being_copied_to_disk;

        // The compute service this is for
        ServerlessComputeService *_serverless_compute_service;

    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_SERVERLESSSTATEOFTHESYSTEM_H
