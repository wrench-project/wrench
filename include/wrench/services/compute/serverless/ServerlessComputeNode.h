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
        ServerlessComputeNode(std::string h, unsigned int num_cores, ServerlessComputeService* service);

        std::shared_ptr<Container> spawnContainer(const Function* function);
        void makeContainerIdle(const std::shared_ptr<Container>& container);
        void makeContainerBusy(const std::shared_ptr<Container>& container);
        void shutdownContainer(const std::shared_ptr<Container>& container);

        [[nodiscard]] unsigned int getNumCores() const;
        [[nodiscard]] unsigned int getNumIdleCores() const;
        [[nodiscard]] sg_size_t getFreeDiskSpace() const;
        [[nodiscard]] sg_size_t getFreeRAMSpace() const;

        std::shared_ptr<Container> findIdleContainer(
            const Function* function,
            const std::set<std::shared_ptr<Container>>& excluded_container) const;
        [[nodiscard]] std::set<std::shared_ptr<Container>>& getIdleContainers();
        [[nodiscard]] std::set<std::shared_ptr<Container>>& getBusyContainers();

        [[nodiscard]] bool isImageLayerOnDisk(const std::shared_ptr<ImageLayer>& layer) const;

        [[nodiscard]] bool isImageLayerInRAM(const std::shared_ptr<ImageLayer>& layer) const;

        [[nodiscard]] bool isInvocationFeasible(const std::shared_ptr<Invocation>& invocation,
                                                const std::shared_ptr<Container>& target_container) const;

        [[nodiscard]] std::shared_ptr<SimpleStorageService> getDiskStorage() const;
        [[nodiscard]] std::shared_ptr<SimpleStorageService> getMemoryStorage() const;

        [[nodiscard]] double getLayerLastAccessDateInRAM(const std::shared_ptr<ImageLayer>& layer) const;
        [[nodiscard]] double getLayerLastAccessDateOnDisk(const std::shared_ptr<ImageLayer>& layer) const;

	/** @brief The hostname of the compute node */
        const std::string hostname;

    private:
        friend class ServerlessComputeService;

        ServerlessComputeService* _serverless_compute_service;

        unsigned int _total_cores;
        unsigned int _available_cores;

        std::shared_ptr<SimpleStorageService> _disk;
        std::shared_ptr<SimpleStorageService> _memory;

        std::set<std::shared_ptr<Container>> _busy_containers;
        std::set<std::shared_ptr<Container>> _idle_containers;

         void killAllContainers();

    };

    /***********************/
    /** \endcond           */
    /***********************/
} // namespace wrench

#endif // WRENCH_SERVERLESSCOMPUTENODE_H
