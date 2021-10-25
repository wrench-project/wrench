/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/simulation/Simulation.h>
#include <wrench/action/Action.h>
#include <wrench/action/FileReadAction.h>
#include <wrench/workflow/WorkflowFile.h>

#include <utility>

namespace wrench {

    /**
    * @brief Constructor
    * @param name: the action's name (if empty, a unique name will be picked for you)
    * @param job: the job this action belongs to
    * @param file: the file
    * @param file_location: the location to read the file from
    */
    FileReadAction::FileReadAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                std::shared_ptr<WorkflowFile> file,
                                std::shared_ptr<FileLocation> file_location) : Action(name, "file_read_", job),
                                file(std::move(file)), file_location(std::move(file_location)) {
    }

    /**
     * @brief Returns the action's file
     * @return the file
     */
    std::shared_ptr<WorkflowFile> FileReadAction::getFile() const {
        return this->file;
    }

    /**
     * @brief Returns the action's file location
     * @return the file location
     */
    std::shared_ptr<FileLocation> FileReadAction::getFileLocation() const {
        return this->file_location;
    }

}
