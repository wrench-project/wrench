/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_STORAGE_SERVICE_NOT_ENOUGH_SPACE_H
#define WRENCH_STORAGE_SERVICE_NOT_ENOUGH_SPACE_H

#include <set>
#include <string>

#include "FailureCause.h"
#include "wrench/services/storage/StorageService.h"

namespace wrench {

    class DataFile;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A "not enough space on storage service" failure cause
     */
    class StorageServiceNotEnoughSpace : public FailureCause {

    public:
        /***********************/
        /** \cond INTERNAL     */
        /***********************/
        StorageServiceNotEnoughSpace(std::shared_ptr<DataFile>file, std::shared_ptr<StorageService>  storage_service);
        /***********************/
        /** \endcond           */
        /***********************/

        std::shared_ptr<DataFile>getFile();
        std::shared_ptr<StorageService>  getStorageService();
        std::string toString() override;


    private:
        std::shared_ptr<DataFile>file;
        std::shared_ptr<StorageService>  storage_service;
    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_STORAGE_SERVICE_NOT_ENOUGH_SPACE_H
