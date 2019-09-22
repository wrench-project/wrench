/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>


namespace wrench {

    /**
     * @brief File location specifier (directory "/")
     *
     * @param ss: a storage service (that has a single mount point)
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(std::shared_ptr<StorageService> ss) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }
        if (ss->hasMultipleMountPoints()) {
            throw std::invalid_argument("FileLocation::LOCATION(): Storage Service has multiple mount points. "
                                        "Call the version of this method that takes a mount point argument");
        }
        return LOCATION(ss, *(ss->getMountPoints().begin()), "/");
    }

    /**
     * @brief File location specifier (directory = "/")
     *
     * @param ss: a storage service
     * @param mp: a mount point of the storage service
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(std::shared_ptr<StorageService> ss,
                                                         std::string mp) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }

        return LOCATION(ss, mp, "/");
    }

    /**
     * @brief File location specifier
     *
     * @param ss: a storage service
     * @param mp: a mount point of the storage service
     * @param dir: a directory
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(std::shared_ptr<StorageService> ss,
                                                         std::string mp,
                                                         std::string dir) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }
        if (not ss->hasMountPoint(mp)) {
            throw std::invalid_argument("FileLocation::LOCATION(): Storage service " + ss->getName() +
                                        " does not have mount point " + mp);
        }
        if (dir.empty()) {
            dir = "/";
        }

        return std::shared_ptr<FileLocation>(new FileLocation(ss, mp, dir));
    }

    /**
     * @brief Give a <ss name>:<mount point>:<dir>" string for the location
     */
    std::string FileLocation::toString() {
        return this->storage_service->getName() + ":" + this->mount_point + ":" + this->directory;
    }

    /**
     * @brief Get the location's storage service
     * @return a storage service
     */
    std::shared_ptr<StorageService> FileLocation::getStorageService() {
        return this->storage_service;
    }

    /**
     * @brief Get the location's storage service's mount point
     * @return
     */
    std::string FileLocation::getMountPoint() {
        return this->mount_point;
    }


    /**
     * @brief Get the location's directory
     * @return
     */
    std::string FileLocation::getDirectory() {
        return this->directory;
    }


}

