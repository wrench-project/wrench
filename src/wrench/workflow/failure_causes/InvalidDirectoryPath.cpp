/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/workflow/failure_causes/InvalidDirectoryPath.h>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/failure_causes/FailureCause.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/workflow/job/WorkflowJob.h"
#include "wrench/services/storage/StorageService.h"
#include "wrench/services/compute/ComputeService.h"

WRENCH_LOG_CATEGORY(wrench_core_invalid_directory_path, "Log category for InvalidDirectoryPath");

namespace wrench {


    /**
     * @brief Constructor
     * @param storage_service: the storage service at which the mount point was unknown
     * @param invalid_path: the invalid path
     */
    InvalidDirectoryPath::InvalidDirectoryPath(std::shared_ptr<StorageService> storage_service,
                                               std::string invalid_path) {
        this->storage_service = storage_service;
        this->invalid_path = invalid_path;
    }

    /**
     * @brief Get the storage service at which the path was invalid
     * @return a storage service
     */
    std::shared_ptr<StorageService> InvalidDirectoryPath::getStorageService() {
        return this->storage_service;
    }

    /**
     * @brief Get the invalid path
     * @return a path
     */
    std::string InvalidDirectoryPath::getInvalidPath() {
        return this->invalid_path;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string InvalidDirectoryPath::toString() {
        return "Storage service " + this->storage_service->getName() + " doesn't have a " +
               this->getInvalidPath() + " path";
    }

}
