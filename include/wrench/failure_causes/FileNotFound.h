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

    class WorkflowFile;
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
        FileNotFound(WorkflowFile *file, std::shared_ptr<FileLocation>  location);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowFile *getFile();
        std::shared_ptr<FileLocation>  getLocation();
        std::string toString();

    private:
        WorkflowFile *file;
        std::shared_ptr<FileLocation> location;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_FILENOTFOUND_H
