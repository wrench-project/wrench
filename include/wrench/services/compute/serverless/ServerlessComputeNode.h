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
#include <wrench/data_file/DataFile.h>
#include <wrench/services/compute/serverless/Container.h>

namespace wrench
{

    class SimpleStorageService;

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

        std::shared_ptr<Container> findIdleContainer(
            const RegisteredFunction *registered_function,
            const std::set<std::shared_ptr<Container>>& excluded_container) const;

        [[nodiscard]] bool isImageBeingCopied(const std::shared_ptr<DataFile>& image) const;
        [[nodiscard]] std::set<std::shared_ptr<DataFile>> getImagesBeingCopied() const;
        [[nodiscard]] bool isImageOnDisk(const std::shared_ptr<DataFile>& image) const;

        [[nodiscard]] bool isImageBeingLoaded(const std::shared_ptr<DataFile>& image) const;
        [[nodiscard]] std::set<std::shared_ptr<DataFile>> getImagesBeingLoaded() const;
        [[nodiscard]] bool isImageInRAM(const std::shared_ptr<DataFile>& image) const;

        [[nodiscard]] bool isInvocationFeasible(const std::shared_ptr<Invocation>& invocation, const std::shared_ptr<Container>& container) const;

        [[nodiscard]] std::shared_ptr<SimpleStorageService> getDiskStorage() const { return _disk; }
        [[nodiscard]] std::shared_ptr<SimpleStorageService> getMemoryStorage() const { return _memory; }

        [[nodiscard]] unsigned int getNumCores() const {return _total_cores; }
        [[nodiscard]] unsigned int getNumIdleCores() const {return _available_cores; }

        const std::string hostname;

    private:
        unsigned int _total_cores;
        unsigned int _available_cores;

        std::shared_ptr<SimpleStorageService> _disk;
        std::shared_ptr<SimpleStorageService> _memory;


        friend class ServerlessComputeService;
        std::set<std::shared_ptr<DataFile>> _images_being_copied;
        std::set<std::shared_ptr<DataFile>> _images_being_loaded;

        std::set<std::shared_ptr<Container>> _busy_containers;
        std::set<std::shared_ptr<Container>> _idle_containers;

        ServerlessComputeService *_serverless_compute_service;
    };

    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_SERVERLESSCOMPUTENODE_H
