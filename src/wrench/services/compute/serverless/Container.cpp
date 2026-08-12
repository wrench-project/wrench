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
     * @brief Constructor
     * @param registered_function
     * @param compute_node
     * @param serverless_compute_service
     * @param initial_state
     * @return
     */
    Container::Container(const RegisteredFunction* registered_function,
                         const ServerlessComputeNode* compute_node,
                         const ServerlessComputeService* serverless_compute_service,
                         State initial_state) {
        _registered_function = registered_function;
        _compute_node = compute_node;
        _serverless_compute_service = serverless_compute_service;
        _state = initial_state;
    }

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
     */
    void Container::spawn() {
        // WRENCH_INFO("Spawning a new container for an invocation of function %s",
        //             _registered_function->getFunction()->getName().c_str());

        /** This method's implementation is overly paranoid exception-wise, but it's likely a good thing **/

        // Open the image disk file (do this first to pin it to RAM - would
        // be weird if, due to LRU, the container itself kicked out the image!)
        auto compute_disk_ss = _compute_node->getDiskStorage();
        try {
            _opened_image_disk_file = compute_disk_ss->openFile(
                FileLocation::LOCATION(compute_disk_ss, _registered_function->getImageFile()));
        }
        catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            throw;
        } catch (simgrid::Exception& e) {
            this->freeDiskAndMemoryResources();
            throw ExecutionException(std::make_shared<FatalFailure>("Can't open image in RAM"));
        }

        // Open the image memory file (do this first to pin it to RAM - would
        // be weird if, due to LRU, the container itself kicked out the image!)
        auto compute_ram_ss = _compute_node->getMemoryStorage();
        try {
            _opened_image_ram_file = compute_ram_ss->openFile(
                FileLocation::LOCATION(compute_ram_ss, _registered_function->getImage()->getRAMFile()));
        }
        catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            throw;
        } catch (simgrid::Exception& e) {
            this->freeDiskAndMemoryResources();
            throw ExecutionException(std::make_shared<FatalFailure>("Can't open image in RAM"));
        }

        // Reserve disk space on the compute node's storage service
        try {
            _tmp_file_location = wrench::FileLocation::LOCATION(
                _compute_node->getDiskStorage(),
                Simulation::addTmpFile(
                    _registered_function->getDiskSpaceLimit()));
            StorageService::createFileAtLocation(_tmp_file_location);
            _opened_tmp_file = _compute_node->getDiskStorage()->openFile(_tmp_file_location);
        }
        catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            auto cause = std::dynamic_pointer_cast<StorageServiceNotEnoughSpace>(e.getCause());
            if (cause) {
                throw ExecutionException(
                    std::make_shared<NotEnoughResources>("Not enough storage space on compute node for container"));
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
                "is_" + std::to_string(ServerlessComputeService::_sequence_number), disk);
            fs = simgrid::fsmod::FileSystem::create(
                "fs" + std::to_string(ServerlessComputeService::_sequence_number));
            fs->mount_partition("/", ods, _registered_function->getDiskSpaceLimit());
        }
        catch (simgrid::Exception& e) {
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
                    _compute_node->hostname, fs, {
                        {
                            SimpleStorageServiceProperty::BUFFER_SIZE,
                            _serverless_compute_service->getPropertyValueAsString(
                                ServerlessComputeServiceProperty::STORAGE_SERVICES_BUFFER_SIZE)
                        }
                    }, {}));
            _tmp_storage_service->setSimulation(_serverless_compute_service->getSimulation());
            _tmp_storage_service->setNetworkTimeoutValue(_serverless_compute_service->getNetworkTimeoutValue());
            _tmp_storage_service->start(_tmp_storage_service, true, false);
        }
        catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            throw;
        }

        // Create and open a tmp memory file in RAM for the invocation's RAM space
        auto tmp_memory_file = Simulation::addTmpFile(_registered_function->getRAMLimit());
        try {
            auto file_location = FileLocation::LOCATION(compute_ram_ss, tmp_memory_file);
            StorageService::createFileAtLocation(file_location);
            _tmp_ram_file_location = file_location;
            _opened_tmp_ram_file = compute_ram_ss->openFile(_tmp_ram_file_location);
        }
        catch (ExecutionException& e) {
            this->freeDiskAndMemoryResources();
            auto cause = std::dynamic_pointer_cast<StorageServiceNotEnoughSpace>(e.getCause());
            if (cause) {
                throw ExecutionException(
                    std::make_shared<NotEnoughResources>("Not enough RAM space on compute node for container"));
            }
            throw;
        } catch (simgrid::Exception& e) {
            this->freeDiskAndMemoryResources();
            throw ExecutionException(std::make_shared<FatalFailure>("Can't create container's RAM space"));
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
     * @brief Clear the private storage's content
     */
    void Container::clearPrivateStorage() {
        for (auto const& partition :
             _tmp_storage_service->getFileSystem()->get_partitions()) {
            partition->erase_all_content();
        }
    }

    /**
     * @brief Helper method to release the disk and memory resource of the container
     */
    void Container::freeDiskAndMemoryResources() {
        // Clearing disk space
        try {
            if (_tmp_storage_service and (_tmp_storage_service->getState() == Service::State::UP)) {
                _tmp_storage_service->stop();
            }
        }
        catch (ExecutionException& ignore) {
        }

        try {
            if (_opened_tmp_file) {
                _opened_tmp_file->close();
            }
        }
        catch (simgrid::Exception& ignore) {
        }

        try {
            if (_tmp_file_location) {
                if (StorageService::hasFileAtLocation(_tmp_file_location)) {
                    StorageService::removeFileAtLocation(_tmp_file_location);
                }
            }
        }
        catch (ExecutionException& ignore) {
        }

        // Clearing RAM space
        try {
            if (_opened_tmp_ram_file) {
                _opened_tmp_ram_file->close();
            }
        }
        catch (simgrid::Exception& ignore) {
        }

        try {
            if (_tmp_ram_file_location) {
                if (StorageService::hasFileAtLocation(_tmp_ram_file_location)) {
                    StorageService::removeFileAtLocation(_tmp_ram_file_location);
                }
            }
        }
        catch (ExecutionException& ignore) {
        }

        // Close the image disk file
        try {
            if (_opened_image_disk_file) {
                _opened_image_disk_file->close();
            }
        }
        catch (simgrid::Exception& ignore) {
        }

        // Close the image ram file
        try {
            if (_opened_image_ram_file) {
                _opened_image_ram_file->close();
            }
        }
        catch (simgrid::Exception& ignore) {
        }

        // Reset all pointers, just to be safe
        _tmp_storage_service.reset();
        _opened_tmp_file.reset();
        _tmp_file_location.reset();
        _opened_tmp_ram_file.reset();
        _tmp_ram_file_location.reset();
        _opened_image_disk_file.reset();
        _opened_image_ram_file.reset();
    }
}
