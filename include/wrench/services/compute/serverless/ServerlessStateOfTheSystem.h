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
#include <memory>
#include <string>
#include <wrench/services/compute/serverless/Invocation.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/data_file/DataFile.h>

namespace wrench
{

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class ServerlessStateOfTheSystem {

    public:
        const std::vector<std::string>& getComputeHosts();
        std::map<std::string, unsigned long> getAvailableCores();
        std::map<std::string, sg_size_t> getAvailableRAM();
        std::map<std::string, sg_size_t> getAvailableDiskSpace();

        std::set<std::shared_ptr<DataFile>> getImagesBeingCopiedToNode(const std::string &node);
        bool isImageOnNode(const std::string &node, const std::shared_ptr<DataFile> &image);
        bool isImageBeingCopiedToNode(const std::string& node, const std::shared_ptr<DataFile>& image);

        std::set<std::shared_ptr<DataFile>> getImagesBeingLoadedAtNode(const std::string &node);
        bool isImageInRAMAtNode(const std::string &node, const std::shared_ptr<DataFile> &image);
        bool isImageBeingLoadedAtNode(const std::string &node, const std::shared_ptr<DataFile> &image);

        ~ServerlessStateOfTheSystem() = default;

    private:
        friend class ServerlessComputeService;

        explicit ServerlessStateOfTheSystem(const std::vector<std::string>& compute_hosts);

        // set of Registered functions
        std::set<std::shared_ptr<RegisteredFunction>> _registered_functions;
        // vector of compute host names
        std::vector<std::string> _compute_hosts;

        // map of available cores on each compute host
        std::map<std::string, unsigned long> _available_cores;
        // map of available RAM on each compute host
        std::map<std::string, sg_size_t> _available_ram;

        // queue of function invocations waiting to be processed
        std::queue<std::shared_ptr<Invocation>> _new_invocations;
        // queues of function invocations whose images are being downloaded
        std::map<std::shared_ptr<DataFile>, std::queue<std::shared_ptr<Invocation>>> _admitted_invocations;
        // queue of function invocations whose images have been downloaded
        std::vector<std::shared_ptr<Invocation>> _schedulable_invocations;
        // queue of function invocations currently running
        std::queue<std::shared_ptr<Invocation>> _running_invocations;
        // queue of function invocations that have finished executing
        std::queue<std::shared_ptr<Invocation>> _finished_invocations;

        std::string _head_storage_service_mount_point;
        // std::vector<std::shared_ptr<BareMetalComputeService>> _compute_services;
        std::unordered_map<std::string, std::shared_ptr<SimpleStorageService>> _compute_storages;
        std::unordered_map<std::string, std::shared_ptr<SimpleStorageService>> _compute_memories;
        std::shared_ptr<StorageService> _head_storage_service;
        std::set<std::shared_ptr<DataFile>> _being_downloaded_image_files;
        sg_size_t _free_space_on_head_storage; // We keep track of it ourselves to avoid concurrency shenanigans

        std::unordered_map<std::string, std::set<std::shared_ptr<DataFile>>> _being_copied_images;
        std::unordered_map<std::string, std::set<std::shared_ptr<DataFile>>> _being_loaded_images;
    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_SERVERLESSSTATEOFTHESYSTEM_H
