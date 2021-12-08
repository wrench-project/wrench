/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/logging/TerminalOutput.h>
#include <wrench/simulation/Simulation.h>
#include <wrench/action/Action.h>
#include <wrench/action/FileCopyAction.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/exceptions/ExecutionException.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_copy_action, "Log category for FileCopyAction");


namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param job: the job this action belongs to
    * @param file: the file
    * @param src_file_location: the location from which the file should be read
    * @param dst_file_location: the location to which the file should be written
    */
    FileCopyAction::FileCopyAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                     std::shared_ptr<DataFile>file,
                                     std::shared_ptr<FileLocation> src_file_location,
                                     std::shared_ptr<FileLocation> dst_file_location) :
            Action(name, "file_copy_", std::move(job)),
            file(file),
            src_file_location(std::move(src_file_location)),
            dst_file_location(std::move(dst_file_location)) {
    }

    /**
     * @brief Returns the action's file
     * @return the file
     */
    std::shared_ptr<DataFile>FileCopyAction::getFile() const {
        return this->file;
    }

    /**
     * @brief Returns the action's source file location
     * @return A file location
     */
    std::shared_ptr<FileLocation> FileCopyAction::getSourceFileLocation() const {
        return this->src_file_location;
    }

    /**
     * @brief Returns the action's destination file location
     * @return A file location
     */
    std::shared_ptr<FileLocation> FileCopyAction::getDestinationFileLocation() const {
        return this->dst_file_location;
    }

    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void FileCopyAction::execute(std::shared_ptr<ActionExecutor> action_executor) {
        // Thread overhead
        Simulation::sleep(this->thread_creation_overhead);
        // File copy
        StorageService::copyFile(
                this->file, this->src_file_location, this->dst_file_location);
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void FileCopyAction::terminate(std::shared_ptr<ActionExecutor> action_executor) {
        // Nothing to do
    }


    /**
     * @brief Determine whether the action uses scratch
     * @return true if the action uses scratch, false otherwise
     */
    bool FileCopyAction::usesScratch() const {
        return (this->src_file_location == FileLocation::SCRATCH or this->dst_file_location == FileLocation::SCRATCH);
    }


}
