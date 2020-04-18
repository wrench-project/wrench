/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/logging/TerminalOutput.h"
#include "wrench/workflow/failure_causes/FileAlreadyBeingCopied.h"
#include "wrench/workflow/WorkflowFile.h"
#include "wrench/services/storage/StorageService.h"

WRENCH_LOG_NEW_DEFAULT_CATEGORY(file_already_being_copied, "Log category for FileAlreadyBeingCopied");

namespace wrench {

    /**
     * @brief Constructor
     * @param file: the file that is already being copied
     * @param src: the source location
     * @param dst: the destination location
     */
    FileAlreadyBeingCopied::FileAlreadyBeingCopied(WorkflowFile *file,
                                                   std::shared_ptr<FileLocation> src,
                                                   std::shared_ptr<FileLocation> dst) {
        this->file = file;
        this->src_location = src;
        this->dst_location = dst;
    }

    /**
     * @brief Getter
     * @return the file
     */
    WorkflowFile *FileAlreadyBeingCopied::getFile() {
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
        return "File " + this->file->getID() + " is already being copied ("+
               "src = " + this->src_location->toString() + "; " +
               "dst = " + this->dst_location->toString() + ")";
    }


};
