/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_REGISTRY_ACTION_H
#define WRENCH_FILE_REGISTRY_ACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {

    class DataFile;
    class FileLocation;
    class FileRegistryService;

    /**
     * @brief A class that implements a file registry (abstract) action
     */
    class FileRegistryAction : public Action {

    public:
        std::shared_ptr<DataFile>getFile() const;
        std::shared_ptr<FileLocation> getFileLocation() const;
        std::shared_ptr<FileRegistryService> getFileRegistryService() const;

    protected:
        friend class CompoundJob;

        /**
         * @brief File registry action type enum
         */
        enum Type {
            ADD,
            DELETE
        };

        FileRegistryAction(FileRegistryAction::Type type,
                           const std::string& name,
                           std::shared_ptr<CompoundJob> job,
                           std::shared_ptr<FileRegistryService> file_registry_service,
                           std::shared_ptr<DataFile>file,
                           std::shared_ptr<FileLocation> file_location);


        void execute(std::shared_ptr<ActionExecutor> action_executor) override;
        void terminate(std::shared_ptr<ActionExecutor> action_executor) override;

    private:
        FileRegistryAction::Type type;
        std::shared_ptr<FileRegistryService> file_registry_service;
        std::shared_ptr<DataFile>file;
        std::shared_ptr<FileLocation> file_location;

    };
}

#endif //WRENCH_FILE_REGISTRY_ACTION_H
