/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/InvalidDirectoryPath.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/failure_causes/FailureCause.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/job/Job.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/compute/ComputeService.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_invalid_directory_path, "Log category for InvalidDirectoryPath");

namespace wrench {


    /**
     * @brief Constructor
     * @param location: the location with the invalid path
     */
    InvalidDirectoryPath::InvalidDirectoryPath(const std::shared_ptr<FileLocation> &location) {
        this->location = location;
    }

    /**
     * @brief Get the location with the invalid path
     * @return a storage service
     */
    std::shared_ptr<FileLocation> InvalidDirectoryPath::getLocation() {
        return this->location;
    }


    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string InvalidDirectoryPath::toString() {
        return "Storage service " + this->location->getStorageService()->getName() + " doesn't have a " +
                this->getLocation()->getDirectoryPath() + " path";
    }

}// namespace wrench
