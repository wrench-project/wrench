/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_ALREADY_BEING_READ_H
#define WRENCH_FILE_ALREADY_BEING_READ_H

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
    class FileAlreadyBeingRead : public FailureCause {


    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FileAlreadyBeingRead(std::shared_ptr<FileLocation> location);

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


#endif//WRENCH_FILE_ALREADY_BEING_READ_H
