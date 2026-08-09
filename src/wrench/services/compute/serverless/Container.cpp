/**
* Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <fsmod/File.hpp>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/failure_causes/NotEnoughResources.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>
#include <wrench/services/compute/serverless/Container.h>
#include <wrench/services/compute/serverless/ServerlessComputeService.h>
#include <wrench/services/storage/simple/SimpleStorageService.h>
#include <wrench/simulation/Simulation.h>

#include "wrench/failure_causes/FatalFailure.h"
#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(Container, "Log category for Container");


namespace wrench {

    /**
     * @brief Make the container idle
     */
    void Container::makeIdle() {
        _state = State::IDLE;
    }

    /**
     * @brief Make the container busy
     */
    void Container::makeBusy() {
        _state = State::BUSY;
        _idle_sequence += 1;
    }



    /**
     * @brief Method to spawn a container
     *
     */
    void Container::spawn() {
        // WRENCH_INFO("Spawning a new container for an invocation of function %s",
        //             _registered_function->getFunction()->getName().c_str());

        /** This method's implementation is overly paranoid exception-wise, but it's likely a good thing **/

        // Reserve disk space on the compute node's storage service
        try {
            _tmp_file_location = wrench::FileLocation::LOCATION(_compute_node->disk,
                                                      Simulation::addFile(
                                                          "tmp_" + std::to_string(
                                                              ++ServerlessComputeService::sequence_number),
                                                          _registered_function->getDiskSpaceLimit()));
            StorageService::createFileAtLocation(_tmp_file_location);
            _opened_tmp_file = _compute_node->disk->openFile(_tmp_file_location);
        } catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            auto cause = std::dynamic_pointer_cast<StorageServiceNotEnoughSpace>(e.getCause());
            if (cause) {
                throw ExecutionException(std::make_shared<NotEnoughResources>("Not enough storage space on compute node for container"));
            }
            throw;
        } catch (simgrid::Exception& e) {
            this->freeDiskAndMemoryResources();
            throw ExecutionException(std::make_shared<FatalFailure>("Can't reserve disk space for the container"));
        }

        // Create a tmp file system for the invocation
        std::shared_ptr<simgrid::fsmod::FileSystem> fs;
        try {
            const auto disk = S4U_Simulation::hostHasMountPoint(_compute_node->hostname, "/");
            if (not disk) {
                throw ExecutionException(std::make_shared<FatalFailure>("Compute node should have a '/' mount point!"));
            }
            const auto ods = simgrid::fsmod::OneDiskStorage::create(
                "is_" + std::to_string(ServerlessComputeService::sequence_number), disk);
            fs = simgrid::fsmod::FileSystem::create(
                "fs" + std::to_string(ServerlessComputeService::sequence_number));
            fs->mount_partition("/", ods, _registered_function->getDiskSpaceLimit());
        } catch (simgrid::Exception& e) {
            this->freeDiskAndMemoryResources();
            throw ExecutionException(std::make_shared<NotEnoughResources>(e.what()));
        } catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            throw;
        }

        // Create a tmp storage service for the invocation
        try {
            _tmp_storage_service = std::shared_ptr<SimpleStorageService>(
                SimpleStorageService::createSimpleStorageServiceWithExistingFileSystem(
                    _compute_node->hostname, fs, {}, {}));
            _tmp_storage_service->setSimulation(_serverless_compute_service->getSimulation());
            _tmp_storage_service->setNetworkTimeoutValue(_serverless_compute_service->getNetworkTimeoutValue());
            _tmp_storage_service->start(_tmp_storage_service, true, false);
        } catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            throw;
        }

        // Create and open a tmp memory file in RAM for the invocation's RAM space
        auto tmp_memory_file = Simulation::addFile(
            "tmp_ram_file_" + std::to_string(++ServerlessComputeService::sequence_number),
            _registered_function->getRAMLimit());
        auto compute_ram_ss = _compute_node->memory;
        try {
            auto file_location = FileLocation::LOCATION(compute_ram_ss, tmp_memory_file);
            StorageService::createFileAtLocation(file_location);
            _tmp_ram_file_location = file_location;
            _opened_tmp_ram_file = compute_ram_ss->openFile(_tmp_ram_file_location);
        } catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            auto cause = std::dynamic_pointer_cast<StorageServiceNotEnoughSpace>(e.getCause());
            if (cause) {
                throw ExecutionException(std::make_shared<NotEnoughResources>("Not enough RAM space on compute node for container"));
            }
            throw;
        } catch (simgrid::Exception& e) {
            this->freeDiskAndMemoryResources();
            throw ExecutionException(std::make_shared<FatalFailure>("Can't create container's RAM space"));
        }

        // Open the image memory file
        try {
            _opened_image_ram_file = compute_ram_ss->openFile(
                FileLocation::LOCATION(compute_ram_ss, _registered_function->getOriginalImageLocation()->getFile()));
        } catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            throw;
        } catch (simgrid::Exception& e) {
            this->freeDiskAndMemoryResources();
            throw ExecutionException(std::make_shared<FatalFailure>("Can't open image in RAM"));
        }
    }

    /**
     * @brief Shutdown the container
     */
    void Container::shutdown() {
        _idle_sequence += 1; // to invalidate any future timeouts
        this->freeDiskAndMemoryResources();
    }

    /**
     * @brief Helper method to release the disk and memory resource of the container
     */
    void Container::freeDiskAndMemoryResources() {
        // Clearing disk space
        if (_tmp_storage_service and (_tmp_storage_service->getState() == Service::State::UP)) {
            _tmp_storage_service->stop();
            _tmp_storage_service = nullptr;
        }
        if (_opened_tmp_file) {
            _opened_tmp_file->close();
        }
        if (_tmp_file_location) {
            if (StorageService::hasFileAtLocation(_tmp_file_location)) {
                StorageService::removeFileAtLocation(_tmp_file_location);
            }
        }

        // Clearing RAM space
        if (_opened_tmp_ram_file) {
            _opened_tmp_ram_file->close();
        }
        if (_tmp_ram_file_location) {
            if (StorageService::hasFileAtLocation(_tmp_ram_file_location)) {
                StorageService::removeFileAtLocation(_tmp_ram_file_location);
            }
        }

        // Close the image ram file
        if (_opened_image_ram_file) {
            _opened_image_ram_file->close();
        }
    }

}
