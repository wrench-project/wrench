/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/logging/TerminalOutput.h>
#include <wrench/failure_causes/FileAlreadyBeingCopied.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/StorageService.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_already_being_copied, "Log category for FileAlreadyBeingCopied");

namespace wrench {

    /**
     * @brief Constructor
     * @param file: the file that is already being copied
     * @param src: the source location
     * @param dst: the destination location
     */
    FileAlreadyBeingCopied::FileAlreadyBeingCopied(std::shared_ptr<DataFile> file,
                                                   std::shared_ptr<FileLocation> src,
                                                   std::shared_ptr<FileLocation> dst) {
        this->file = std::move(file);
        this->src_location = std::move(src);
        this->dst_location = std::move(dst);
    }

    /**
     * @brief Getter
     * @return the file
     */
    std::shared_ptr<DataFile> FileAlreadyBeingCopied::getFile() {
        return this->file;
    }

    /**
     * @brief Getter
     * @return the source location
     */
    std::shared_ptr<FileLocation> FileAlreadyBeingCopied::getSourceLocation() {
        return this->src_location;
    }

    /**
    * @brief Getter
    * @return the source location
    */
    std::shared_ptr<FileLocation> FileAlreadyBeingCopied::getDestinationLocation() {
        return this->dst_location;
    }

    /**
     * @brief Get the human-readable failure message
     * @return the message
     */
    std::string FileAlreadyBeingCopied::toString() {
        return "File " + this->file->getID() + " is already being copied (" +
               "src = " + this->src_location->toString() + "; " +
               "dst = " + this->dst_location->toString() + ")";
    }


}// namespace wrench
