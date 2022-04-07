/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_READ_ACTION_H
#define WRENCH_FILE_READ_ACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {


    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class DataFile;
    class FileLocation;

    /**
     * @brief A class that implements a file read action
     */
    class FileReadAction : public Action {

    public:
        std::shared_ptr<DataFile> getFile() const;
        std::vector<std::shared_ptr<FileLocation>> getFileLocations() const;
        std::shared_ptr<FileLocation> getUsedFileLocation() const;
        bool usesScratch() const override;

    protected:
        friend class CompoundJob;

        FileReadAction(const std::string &name, std::shared_ptr<CompoundJob> job,
                       std::shared_ptr<DataFile> file, std::vector<std::shared_ptr<FileLocation>> file_locations,
                       double num_bytes_to_read);


        void execute(std::shared_ptr<ActionExecutor> action_executor) override;
        void terminate(std::shared_ptr<ActionExecutor> action_executor) override;


    private:
        std::shared_ptr<DataFile> file;
        std::vector<std::shared_ptr<FileLocation>> file_locations;
        std::shared_ptr<FileLocation> used_location;
        double num_bytes_to_read;
    };


    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#endif//WRENCH_FILE_READ_ACTION_H
