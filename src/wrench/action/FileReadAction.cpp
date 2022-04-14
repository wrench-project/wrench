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
#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/exceptions/ExecutionException.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_read_action, "Log category for FileReadAction");


namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file: the file
    * @param file_locations: the locations to read the file from (will be tried in order until one succeeds)
    * @param num_bytes_to_read: the number of bytes to read (if < 0: read the whole file)
    */
    FileReadAction::FileReadAction(const std::string &name,
                                   std::shared_ptr<DataFile> file,
                                   std::vector<std::shared_ptr<FileLocation>> file_locations,
                                   double num_bytes_to_read) : Action(name, "file_read_"),
                                                               file(std::move(file)),
                                                               file_locations(std::move(file_locations)) {
        if (num_bytes_to_read < 0.0) {
            this->num_bytes_to_read = this->file->getSize();
        } else if (num_bytes_to_read <= this->file->getSize()) {
            this->num_bytes_to_read = num_bytes_to_read;
        } else {
            throw std::invalid_argument("FileReadAction::FileReadAction(): cannot create a file read action that would read more bytes than the file size");
        }
    }

    /**
     * @brief Returns the action's file
     * @return the file
     */
    std::shared_ptr<DataFile> FileReadAction::getFile() const {
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
    void FileReadAction::execute(const std::shared_ptr<ActionExecutor> &action_executor) {
        // Thread overhead
        Simulation::sleep(action_executor->getThreadCreationOverhead());
        // File read
        for (unsigned long i = 0; i < this->file_locations.size(); i++) {
            try {
                this->used_location = this->file_locations[i];
                StorageService::readFile(this->getFile(), this->file_locations[i], this->num_bytes_to_read);
                continue;
            } catch (ExecutionException &e) {
                if (i == this->file_locations.size() - 1) {
                    throw e;
                } else {
                    continue;
                }
            }
        }
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void FileReadAction::terminate(const std::shared_ptr<ActionExecutor> &action_executor) {
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
        return std::any_of(this->file_locations.begin(),
                           this->file_locations.end(),
                           [](const std::shared_ptr<FileLocation> &fl) {
            return (fl == FileLocation::SCRATCH);
        });

//        for (auto const &fl: this->file_locations) {
//            if (fl == FileLocation::SCRATCH) {
//                return true;
//            }
//        }
//        return false;
    }

}// namespace wrench
