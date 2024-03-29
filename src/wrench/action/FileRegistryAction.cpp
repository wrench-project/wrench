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
#include <wrench/action/FileRegistryAction.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/file_registry/FileRegistryService.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>


#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_registry_action, "Log category for FileRegistryAction");


namespace wrench {

    /**
    * @brief Constructor
    * @param type: the action's type
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file_registry_service: the file registry service to update
    * @param file_location: the location where the file should be deleted
    */
    FileRegistryAction::FileRegistryAction(FileRegistryAction::Type type,
                                           const std::string &name,
                                           std::shared_ptr<FileRegistryService> file_registry_service,
                                           std::shared_ptr<FileLocation> file_location) : Action(name, "file_registry"),
                                                                                          type(type), file_registry_service(std::move(file_registry_service)), file_location(std::move(file_location)) {
    }

    /**
     * @brief Returns the action's file registry service
     * @return the file registry service
     */
    std::shared_ptr<FileRegistryService> FileRegistryAction::getFileRegistryService() const {
        return this->file_registry_service;
    }


    /**
     * @brief Returns the action's file
     * @return the file
     */
    std::shared_ptr<DataFile> FileRegistryAction::getFile() const {
        return this->file_location->getFile();
    }

    /**
     * @brief Returns the action's file locations
     * @return A file location
     */
    std::shared_ptr<FileLocation> FileRegistryAction::getFileLocation() const {
        return this->file_location;
    }


    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void FileRegistryAction::execute(const std::shared_ptr<ActionExecutor> &action_executor) {
        // Thread overhead
        Simulation::sleep(action_executor->getThreadCreationOverhead());
        // File write
        if (this->type == FileRegistryAction::ADD) {
            this->file_registry_service->addEntry(this->file_location);
        } else if (this->type == FileRegistryAction::DELETE) {
            this->file_registry_service->removeEntry(this->file_location);
        }
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void FileRegistryAction::terminate(const std::shared_ptr<ActionExecutor> &action_executor) {
        // Nothing to do
    }

}// namespace wrench
