/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SERVERLESSCOMPUTENODE_H
#define WRENCH_SERVERLESSCOMPUTENODE_H

#include <set>
#include <memory>
#include <string>
#include <simgrid/forward.h>

#include "wrench/services/storage/simple/SimpleStorageService.h"

namespace simgrid::fsmod {
    class File;
}

namespace wrench
{

    class ServerlessComputeService;
    class RegisteredFunction;
    class SimpleStorageService;
    class Image;
    class Invocation;
    class Container;

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A class that stores the state of a serverless compute node
     */
    class ServerlessComputeNode {

    public:
        ServerlessComputeNode(std::string h, unsigned int num_cores, ServerlessComputeService *service);

        std::shared_ptr<Container> spawnContainer(RegisteredFunction *registered_function);
        void makeContainerIdle(const std::shared_ptr<Container>& container);
        void makeContainerBusy(const std::shared_ptr<Container>& container);
        void shutdownContainer(const std::shared_ptr<Container>& container);


        [[nodiscard]] unsigned int getNumCores() const {return _total_cores; }
        [[nodiscard]] unsigned int getNumIdleCores() const {return _available_cores; }
        [[nodiscard]] sg_size_t getFreeDiskSpace() const { return _disk->getTotalFreeSpaceZeroTime(); }
        [[nodiscard]] sg_size_t getFreeRAMSpace() const { return _memory->getTotalFreeSpaceZeroTime(); }

        std::shared_ptr<Container> findIdleContainer(
            const RegisteredFunction *registered_function,
            const std::set<std::shared_ptr<Container>>& excluded_container) const;

        [[nodiscard]] bool isImageBeingCopied(const std::shared_ptr<Image>& image) const;
        [[nodiscard]] std::set<std::shared_ptr<Image>> getImagesBeingCopied() const;
        [[nodiscard]] bool isImageOnDisk(const std::shared_ptr<Image>& image) const;

        [[nodiscard]] bool isImageBeingLoaded(const std::shared_ptr<Image>& image) const;
        [[nodiscard]] std::set<std::shared_ptr<Image>> getImagesBeingLoaded() const;
        [[nodiscard]] bool isImageInRAM(const std::shared_ptr<Image>& image) const;

        [[nodiscard]] bool isInvocationFeasible(const std::shared_ptr<Invocation>& invocation, const std::shared_ptr<Container>& container) const;

        [[nodiscard]] std::shared_ptr<SimpleStorageService> getDiskStorage() const { return _disk; }
        [[nodiscard]] std::shared_ptr<SimpleStorageService> getMemoryStorage() const { return _memory; }

        const std::string hostname;

    private:
        ServerlessComputeService *_serverless_compute_service;

        unsigned int _total_cores;
        unsigned int _available_cores;

        std::shared_ptr<SimpleStorageService> _disk;
        std::shared_ptr<SimpleStorageService> _memory;

        friend class ServerlessComputeService;
        std::set<std::shared_ptr<Image>> _images_being_copied;
        std::set<std::shared_ptr<Image>> _images_being_loaded;

        std::set<std::shared_ptr<Container>> _busy_containers;
        std::set<std::shared_ptr<Container>> _idle_containers;

        std::unordered_map<std::shared_ptr<Image>, std::shared_ptr<simgrid::fsmod::File>> _disk_image_pins;
        std::unordered_map<std::shared_ptr<Image>, std::shared_ptr<simgrid::fsmod::File>> _ram_image_pins;

    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_SERVERLESSCOMPUTENODE_H
