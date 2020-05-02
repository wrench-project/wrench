/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_INVALIDDIRECTORYPATH_H
#define WRENCH_INVALIDDIRECTORYPATH_H

#include <set>
#include <string>

#include "wrench/services/Service.h"
#include "wrench/workflow/failure_causes/FailureCause.h"
#include "wrench/workflow/failure_causes/InvalidDirectoryPath.h"

namespace wrench {

    class StorageService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "unknown mount point storage service" failure cause
      */
    class InvalidDirectoryPath : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        InvalidDirectoryPath(
                std::shared_ptr<StorageService> storage_service,
                std::string invalid_path);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<StorageService>  getStorageService();
        std::string  getInvalidPath();
        std::string toString();

    private:
        std::shared_ptr<StorageService>  storage_service;
        std::string  invalid_path;
    };


    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_INVALIDDIRECTORYPATH_H
