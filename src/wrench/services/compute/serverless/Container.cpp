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

#include "wrench/logging/TerminalOutput.h"

WRENCH_LOG_CATEGORY(Container, "Log category for Container");


namespace wrench {

    void Container::makeIdle() {
        _state = State::IDLE;
        _idle_date = S4U_Simulation::getClock();
    }

    void Container::makeBusy() {
        _state = State::BUSY;
        _idle_date = -1;
    }

    double Container::getIdleTime() const {
        return S4U_Simulation::getClock() - _idle_date;
    }

    /**
     * @brief Method to spawn a container
     *
     */
    void Container::spawn() {
        // WRENCH_INFO("Starting a new container for an invocation of function %s...",
        //             _registered_function->getFunction()->getName().c_str());

        // Reserve space on the compute node's storage service
        try {
            _tmp_file_location = wrench::FileLocation::LOCATION(_compute_node->disk,
                                                      Simulation::addFile(
                                                          "tmp_" + std::to_string(
                                                              ++ServerlessComputeService::sequence_number),
                                                          _registered_function->getDiskSpaceLimit()));
            StorageService::createFileAtLocation(_tmp_file_location);
            _opened_tmp_file = _compute_node->disk->openFile(_tmp_file_location);
        }
        catch (ExecutionException& e) {
            auto cause = std::dynamic_pointer_cast<StorageServiceNotEnoughSpace>(e.getCause());
            if (cause) {
                throw ExecutionException(std::make_shared<NotEnoughResources>("Not enough storage space on compute node for container"));
            }
            throw;
        }

        // Create a tmp file system
        const auto disk = S4U_Simulation::hostHasMountPoint(_compute_node->hostname, "/");
        const auto ods = simgrid::fsmod::OneDiskStorage::create(
            "is_" + std::to_string(ServerlessComputeService::sequence_number), disk);
        const auto fs = simgrid::fsmod::FileSystem::create(
            "fs" + std::to_string(ServerlessComputeService::sequence_number));
        fs->mount_partition("/", ods, _registered_function->getDiskSpaceLimit());

        // Create a tmp storage service
        _tmp_storage_service = std::shared_ptr<SimpleStorageService>(
            SimpleStorageService::createSimpleStorageServiceWithExistingFileSystem(
                _compute_node->hostname, fs, {}, {}));
        _tmp_storage_service->setSimulation(_serverless_compute_service->getSimulation());
        _tmp_storage_service->setNetworkTimeoutValue(_serverless_compute_service->getNetworkTimeoutValue());
        _tmp_storage_service->start(_tmp_storage_service, true, false);

        // Create and open a tmp memory file in RAM and open it, if possible
        auto tmp_memory_file = Simulation::addFile(
            "tmp_ram_file_" + std::to_string(++ServerlessComputeService::sequence_number),
            _registered_function->getRAMLimit());
        auto compute_ram_ss = _compute_node->memory;
        try {
            auto file_location = FileLocation::LOCATION(compute_ram_ss, tmp_memory_file);
            StorageService::createFileAtLocation(file_location);
            _tmp_ram_file_location = file_location;
            _opened_tmp_ram_file = compute_ram_ss->openFile(_tmp_ram_file_location);
        }
        catch (ExecutionException& e) {
            _tmp_storage_service->stop();
            auto cause = std::dynamic_pointer_cast<StorageServiceNotEnoughSpace>(e.getCause());
            if (cause) {
                throw ExecutionException(std::make_shared<NotEnoughResources>("Not enough RAM space on compute node for container"));
            }
        }

        // Open the image memory file
        _opened_image_ram_file = compute_ram_ss->openFile(
            FileLocation::LOCATION(compute_ram_ss,
                                   _registered_function->getOriginalImageLocation()->getFile()));
    }


    void Container::shutdown() {
        // Clearing disk space
        _tmp_storage_service->stop();
        _opened_tmp_file->close();
        StorageService::removeFileAtLocation(_tmp_file_location);

        // Clearing RAM space
        _opened_tmp_ram_file->close();
        StorageService::removeFileAtLocation(_tmp_ram_file_location);

        // Close the image ram file
        _opened_image_ram_file->close();
    }
}
