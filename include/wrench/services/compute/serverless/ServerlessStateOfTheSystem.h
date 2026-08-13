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

        [[nodiscard]] std::set<std::shared_ptr<Image>> getImagesBeingCopiedToNode(const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool isImageOnDiskAtNode(const std::shared_ptr<ServerlessComputeNode> &node, const std::shared_ptr<Image> &image) const;
        [[nodiscard]] bool isImageBeingCopiedToNode(const std::shared_ptr<ServerlessComputeNode>& node, const std::shared_ptr<Image>& image) const;

        [[nodiscard]] std::set<std::shared_ptr<Image>> getImagesBeingLoadedAtNode(const std::shared_ptr<ServerlessComputeNode> &node) const;
        [[nodiscard]] bool isImageInRAMAtNode(const std::shared_ptr<ServerlessComputeNode> &node, const std::shared_ptr<Image> &image) const;
        [[nodiscard]] bool isImageBeingLoadedAtNode(const std::shared_ptr<ServerlessComputeNode> &node, const std::shared_ptr<Image> &image) const;

        ~ServerlessStateOfTheSystem() = default;

    private:
        friend class ServerlessComputeService;

        explicit ServerlessStateOfTheSystem(const std::vector<std::string>& compute_hosts, ServerlessComputeService *service);

        // set of Registered functions
        std::set<std::shared_ptr<RegisteredFunction>> _registered_functions;

        // queue of function invocations waiting to be processed
        std::queue<std::shared_ptr<Invocation>> _new_invocations;
        // queues of function invocations whose images are being downloaded
        std::map<std::shared_ptr<Image>, std::queue<std::shared_ptr<Invocation>>> _admitted_invocations;
        // queue of function invocations whose images have been downloaded
        std::vector<std::shared_ptr<Invocation>> _schedulable_invocations;
        // set of function invocations currently running
        std::unordered_set<std::shared_ptr<Invocation>> _running_invocations;

        std::string _head_storage_service_mount_point;
        std::shared_ptr<StorageService> _head_storage_service;
        std::set<std::shared_ptr<Image>> _being_downloaded_images;
        sg_size_t _free_space_on_head_storage; // We keep track of it ourselves to avoid concurrency shenanigans

        // list of compute nodes
        std::vector<std::shared_ptr<ServerlessComputeNode>> _compute_nodes;

        // The compute service this is for
        ServerlessComputeService *_serverless_compute_service;

    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_SERVERLESSSTATEOFTHESYSTEM_H
