/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_REGISTRY_ADD_ENTRY_ACTION_H
#define WRENCH_FILE_REGISTRY_ADD_ENTRY_ACTION_H

#include <string>

#include "wrench/action/FileRegistryAction.h"

namespace wrench {


    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class DataFile;
    class FileLocation;
    class FileRegistryService;

    /**
     * @brief A class that implements a file registry (add entry) action
     */
    class FileRegistryAddEntryAction : public FileRegistryAction {

    public:
    protected:
        friend class CompoundJob;

        /**
        * @brief Constructor
        * @param name: the action's name
        * @param file_registry_service: the file registry service to update
        * @param file_location: the file location
        */
        FileRegistryAddEntryAction(const std::string &name,
                                   std::shared_ptr<FileRegistryService> file_registry_service,
                                   std::shared_ptr<FileLocation> file_location) : FileRegistryAction(FileRegistryAction::ADD, name, std::move(file_registry_service), std::move(file_location)) {}


    private:
        std::shared_ptr<FileRegistryService> file_registry_service;
        std::shared_ptr<FileLocation> file_location;
    };


    /***********************/
    /** \endcond           */
    /***********************/


}// namespace wrench

#endif//WRENCH_FILE_REGISTRY_ADD_ENTRY_ACTION_H
