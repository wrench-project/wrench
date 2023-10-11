/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_ALREADY_BEING_WRITTEN_H
#define WRENCH_FILE_ALREADY_BEING_WRITTEN_H

#include <set>
#include <string>

#include "FailureCause.h"

namespace wrench {

    class DataFile;
    class FileLocation;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class FileLocation;

    /**
     * @brief A "file is already being copied" failure cause
     */
    class FileAlreadyBeingWritten : public FailureCause {


    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        explicit FileAlreadyBeingWritten(std::shared_ptr<FileLocation> location);

        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<FileLocation> getLocation();
        std::string toString() override;

    private:
        std::shared_ptr<FileLocation> location;
    };


    /***********************/
    /** \endcond           */
    /***********************/
}// namespace wrench


#endif//WRENCH_FILE_ALREADY_BEING_WRITTEN_H
