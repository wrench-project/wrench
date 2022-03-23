/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILENOTFOUND_H
#define WRENCH_FILENOTFOUND_H

#include <set>
#include <string>

#include "FailureCause.h"

namespace wrench {

    class DataFile;
    class FileLocation;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "file was not found" failure cause
     */
    class FileNotFound : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FileNotFound(std::shared_ptr<DataFile> file, std::shared_ptr<FileLocation> location);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<DataFile> getFile();
        std::shared_ptr<FileLocation> getLocation();
        std::string toString() override;

    private:
        std::shared_ptr<DataFile> file;
        std::shared_ptr<FileLocation> location;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};// namespace wrench


#endif//WRENCH_FILENOTFOUND_H
