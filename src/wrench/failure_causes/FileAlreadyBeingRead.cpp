/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/logging/TerminalOutput.h>
#include <wrench/failure_causes/FileAlreadyBeingRead.h>
#include <wrench/services/storage/StorageService.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_already_being_read, "Log category for FileAlreadyBeingRead");

namespace wrench {

    /**
     * @brief Constructor
     * @param location: the location
     */
    FileAlreadyBeingRead::FileAlreadyBeingRead(std::shared_ptr<FileLocation> location) {
        this->location = std::move(location);
    }

    /**
     * @brief Getter
     * @return the file
     */
    std::shared_ptr<FileLocation> FileAlreadyBeingRead::getLocation() {
        return this->location;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FileAlreadyBeingRead::toString() {
        return "File " + this->location->getFile()->getID() + " is already being read (" +
               "location = " + this->location->toString() + ")";
    }


}// namespace wrench
