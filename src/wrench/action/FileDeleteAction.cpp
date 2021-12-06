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
#include <wrench/action/FileDeleteAction.h>
#include <wrench/workflow/WorkflowFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/exceptions/ExecutionException.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_delete_action, "Log category for FileDeleteAction");


namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param job: the job this action belongs to
    * @param file: the file
    * @param file_location: the location where the file should be deleted
    */
    FileDeleteAction::FileDeleteAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                     WorkflowFile *file,
                                     std::shared_ptr<FileLocation> file_location) :
            Action(name, "file_delete_", job),
            file(std::move(file)), file_location(std::move(file_location)) {
    }

    /**
     * @brief Returns the action's file
     * @return the file
     */
    WorkflowFile *FileDeleteAction::getFile() const {
        return this->file;
    }

    /**
     * @brief Returns the action's file locations
     * @return A file location
     */
    std::shared_ptr<FileLocation> FileDeleteAction::getFileLocation() const {
        return this->file_location;
    }


    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void FileDeleteAction::execute(std::shared_ptr<ActionExecutor> action_executor) {
        // Thread overhead
        Simulation::sleep(this->thread_creation_overhead);
        // File write
        StorageService::deleteFile(this->file, this->file_location);
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void FileDeleteAction::terminate(std::shared_ptr<ActionExecutor> action_executor) {
        // Nothing to do
    }

    /**
     * @brief Determine whether the action uses scratch
     * @return true if the action uses scratch, false otherwise
     */
    bool FileDeleteAction::usesScratch() const {
        return (this->file_location == FileLocation::SCRATCH);
    }

}
