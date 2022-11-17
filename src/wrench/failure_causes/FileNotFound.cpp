/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/failure_causes/FileNotFound.h>

#include <wrench/logging/TerminalOutput.h>
#include <wrench/failure_causes/FailureCause.h>
#include <wrench/data_file/DataFile.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_not_found, "Log category for FileNotFound");

namespace wrench {

    /**
     * @brief Constructor
     * @param location: the location at which the file could not be found (could be nullptr)
     */
    FileNotFound::FileNotFound(std::shared_ptr<FileLocation> location) {
        this->location = std::move(location);
    }

    /**
     * @brief Getter
     * @return the file
     */
    std::shared_ptr<DataFile> FileNotFound::getFile() {
        return this->location->getFile();
    }

    /**
     * @brief Getter
     * @return the storage service
     */
    std::shared_ptr<FileLocation> FileNotFound::getLocation() {
        return this->location;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FileNotFound::toString() {
        std::string msg = "Couldn't find file " + this->location->getFile()->getID();
        if (this->location) {
            msg += " at location " + this->location->toString();
        }
        return msg;
    }

}// namespace wrench
