/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_ALREADY_BEING_COPIED_H
#define WRENCH_FILE_ALREADY_BEING_COPIED_H

#include <set>
#include <string>

#include "wrench/workflow/failure_causes/FailureCause.h"

namespace wrench {

    class WorkflowFile;
    class FileLocation;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class FileLocation;

    /**
     * @brief A "file is already being copied" failure cause
     */
    class FileAlreadyBeingCopied : public FailureCause {


    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        FileAlreadyBeingCopied(WorkflowFile *file,
                               std::shared_ptr<FileLocation> src,
                               std::shared_ptr<FileLocation> dst);
        /***********************/
        /** \endcond           */
        /***********************/

        WorkflowFile *getFile();
        std::shared_ptr<FileLocation> getSourceLocation();
        std::shared_ptr<FileLocation> getDestinationLocation();
        std::string toString();

    private:
        WorkflowFile *file;
        std::shared_ptr<FileLocation> src_location;
        std::shared_ptr<FileLocation> dst_location;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_FILE_ALREADY_BEING_COPIED_H
