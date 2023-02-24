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
#include <wrench/action/FileWriteAction.h>
#include <wrench/data_file/DataFile.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/helper_services/action_executor/ActionExecutor.h>
#include <wrench/services/helper_services/action_execution_service/ActionExecutionService.h>


#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_file_write_action, "Log category for FileWriteAction");


namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param file_location: the location where the file should be written
    */
    FileWriteAction::FileWriteAction(const std::string &name,
                                     std::shared_ptr<FileLocation> file_location) : Action(name, "file_write_"),
                                                                                    file_location(std::move(file_location)) {
    }

    /**
     * @brief Returns the action's file
     * @return the file
     */
    std::shared_ptr<DataFile> FileWriteAction::getFile() const {
        return this->file_location->getFile();
    }

    /**
     * @brief Returns the action's file location
     * @return A file location
     */
    std::shared_ptr<FileLocation> FileWriteAction::getFileLocation() const {
        return this->file_location;
    }


    /**
     * @brief Method to execute the action
     * @param action_executor: the executor that executes this action
     */
    void FileWriteAction::execute(const std::shared_ptr<ActionExecutor> &action_executor) {
        // Thread overhead
        Simulation::sleep(action_executor->getThreadCreationOverhead());
        // Fix location if scratch
        if (this->file_location->isScratch()) {
            auto cs = std::dynamic_pointer_cast<ComputeService>(action_executor->getActionExecutionService()->getParentService());
            this->file_location = FileLocation::LOCATION(cs->getScratch(), cs->getScratch()->getBaseRootPath() + this->getJob()->getName(), this->file_location->getFile());
        }
        // File write
        StorageService::writeFileAtLocation(this->file_location);
    }

    /**
     * @brief Method called when the action terminates
     * @param action_executor: the executor that executes this action
     */
    void FileWriteAction::terminate(const std::shared_ptr<ActionExecutor> &action_executor) {
        // Nothing to do
    }

    /**
  * @brief Determine whether the action uses scratch
  * @return true if the action uses scratch, false otherwise
  */
    bool FileWriteAction::usesScratch() const {
        return (this->file_location->isScratch());
    }


}// namespace wrench
