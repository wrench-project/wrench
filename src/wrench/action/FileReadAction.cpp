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
#include <wrench/action/FileReadAction.h>
#include <wrench/workflow/WorkflowFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/exceptions/ExecutionException.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_read_action, "Log category for FileReadAction");


namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param job: the job this action belongs to
    * @param file: the file
    * @param file_locations: the locations to read the file from (will be tried in order until one succeeds)
    */
    FileReadAction::FileReadAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                WorkflowFile *file,
                                   std::vector<std::shared_ptr<FileLocation>> file_locations) : Action(name, "file_read_", job),
                                file(std::move(file)), file_locations(std::move(file_locations)) {
    }

    /**
     * @brief Returns the action's file
     * @return the file
     */
    WorkflowFile *FileReadAction::getFile() const {
        return this->file;
    }

    /**
     * @brief Returns the action's file locations
     * @return A vector of file locations
     */
    std::vector<std::shared_ptr<FileLocation>> FileReadAction::getFileLocations() const {
        return this->file_locations;
    }


    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void FileReadAction::execute(std::shared_ptr<ActionExecutor> action_executor) {
        // Thread overhead
        Simulation::sleep(this->thread_creation_overhead);
        // File read
        for (unsigned long i=0; i < this->file_locations.size(); i++) {
            try {
                this->used_location = this->file_locations[i];
                StorageService::readFile(this->getFile(), this->file_locations[i]);
                continue;
            } catch (ExecutionException &e) {
                if (i == this->file_locations.size() -1) {
                    throw e;
                } else {
                    continue;
                }
            }
        }
    }

    /**
     * @brief Method to terminate the action
     * @param action_executor: the executor that executes this action
     */
    void FileReadAction::terminate(std::shared_ptr<ActionExecutor> action_executor) {
        // Nothing to do
    }

    /**
     * @brief Return the file location used by the action (or nullptr if action
     *        has not started, failed, etc.)
     * @return A storage service
     */
    std::shared_ptr<FileLocation> FileReadAction::getUsedFileLocation() const {
        return this->used_location;
    }

    /**
      * @brief Determine whether the action uses scratch
      * @return true if the action uses scratch, false otherwise
      */
    bool FileReadAction::usesScratch() const {
        for (auto const &fl : this->file_locations) {
            if (fl == FileLocation::SCRATCH) {
                return true;
            }
        }
        return false;
    }

}
