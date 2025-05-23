/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/logging/TerminalOutput.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_storage_service_not_enough_space, "Log category for StorageServiceNotEnoughSpace");

namespace wrench {

    /**
     * @brief Constructor
     * @param file: the file that could not be written
     * @param storage_service:  the storage service that ran out of space
     */
    StorageServiceNotEnoughSpace::StorageServiceNotEnoughSpace(std::shared_ptr<DataFile> file,
                                                               std::shared_ptr<StorageService> storage_service) {
        this->file = std::move(file);
        this->storage_service = std::move(storage_service);
    }

    /**
     * @brief Getter
     * @return the file
     */
    std::shared_ptr<DataFile> StorageServiceNotEnoughSpace::getFile() {
        return this->file;
    }

    /**
     * @brief Getter
     * @return the storage service
     */
    std::shared_ptr<StorageService> StorageServiceNotEnoughSpace::getStorageService() {
        return this->storage_service;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string StorageServiceNotEnoughSpace::toString() {
        return "Cannot write file " + this->file->getID() + " to Storage Service " +
               this->storage_service->getName() + " due to lack of storage space";
    }

}// namespace wrench
