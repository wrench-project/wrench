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

namespace wrench {
    class ServerlessComputeService;
    class Function;
    class SimpleStorageService;
    class Image;
    class ImageLayer;
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

        ServerlessComputeNode(std::string h, ServerlessComputeService* service);

        std::shared_ptr<Container> spawnContainer(const Function* function);
        void acknowledgeInvocationStart(const std::shared_ptr<Invocation>& invocation);
        void acknowledgeInvocationCompletion(const std::shared_ptr<Invocation>& invocation);
        void shutdownContainer(const std::shared_ptr<Container>& container);

        [[nodiscard]] unsigned long getMaxNumInvocations() const;
        [[nodiscard]] unsigned long getMaxNumContainers() const;
        [[nodiscard]] unsigned long getTotalNumContainers() const;

        [[nodiscard]] sg_size_t getFreeDiskSpace() const;
        [[nodiscard]] sg_size_t getFreeRAMSpace() const;

        [[nodiscard]] const std::set<std::shared_ptr<Container>>& getInitializingContainers() const;
        [[nodiscard]] const std::set<std::shared_ptr<Container>>& getInitializedContainers() const;

        [[nodiscard]] bool isImageLayerOnDisk(const std::shared_ptr<ImageLayer>& layer) const;

        [[nodiscard]] bool isImageLayerInRAM(const std::shared_ptr<ImageLayer>& layer) const;

        [[nodiscard]] bool couldInvocationRunRightNow(
            const std::shared_ptr<Invocation>& invocation,
            const std::shared_ptr<Container>& target_container) const;

        [[nodiscard]] std::shared_ptr<SimpleStorageService> getDiskStorage() const;
        [[nodiscard]] std::shared_ptr<SimpleStorageService> getMemoryStorage() const;

        [[nodiscard]] double getLayerLastAccessDateInRAM(const std::shared_ptr<ImageLayer>& layer) const;
        [[nodiscard]] double getLayerLastAccessDateOnDisk(const std::shared_ptr<ImageLayer>& layer) const;

        [[nodiscard]] bool isAtPerNodeContainerLimit() const;
        [[nodiscard]] bool isAtPerNodeInvocationLimit() const;
        [[nodiscard]] bool areAllContainersAtInvocationLimit() const;

	/** @brief The hostname of the compute node */
        const std::string hostname;

    private:
        friend class ServerlessComputeService;

        ServerlessComputeService* _serverless_compute_service;

        std::shared_ptr<SimpleStorageService> _disk;
        std::shared_ptr<SimpleStorageService> _memory;

        std::set<std::shared_ptr<Container>> _initializing_containers;
        std::set<std::shared_ptr<Container>> _initialized_containers;

        unsigned long _max_num_containers_per_compute_node;
        unsigned long _max_num_invocations_per_compute_node;
        unsigned long _max_num_invocations_per_container;

         void killAllContainers();

    };

    /***********************/
    /** \endcond           */
    /***********************/
} // namespace wrench

#endif // WRENCH_SERVERLESSCOMPUTENODE_H
