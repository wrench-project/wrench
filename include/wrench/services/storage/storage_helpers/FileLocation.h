/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILELOCATION_H
#define WRENCH_FILELOCATION_H

#include <memory>


namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class StorageService;

    class FileLocation {

    public:


        static std::shared_ptr<FileLocation> LOCATION(std::shared_ptr<StorageService> ss);

        static std::shared_ptr<FileLocation> LOCATION(std::shared_ptr<StorageService> ss,
                                                      std::string mp);

        static std::shared_ptr<FileLocation> LOCATION(std::shared_ptr<StorageService> ss,
                                                      std::string mp,
                                                      std::string dir);

        std::shared_ptr<StorageService> getStorageService();
        std::string getMountPoint();
        std::string getDirectory();

        std::string toString();


        bool operator==(const std::shared_ptr<FileLocation> &rhs) {
            return ((this->getStorageService() == rhs->getStorageService()) and
            (this->getMountPoint() == rhs->getMountPoint()) and
            (this->getDirectory() == rhs->getDirectory()));
        }

        /**
         * @brief Overloaded "not equal" operator
         * @param rhs: right-hand side of the comparison
         * @return true if different, false otherwise
         */
        bool operator!=(const std::shared_ptr<FileLocation> &rhs) {
            return not FileLocation::operator==(rhs);
        }

    private:

        /**
         * @brief Constructor
         * @param ss: the storage service
         * @param mp: the mount point
         * @param dir: the directory
         */
        FileLocation(std::shared_ptr<StorageService> ss, std::string mp, std::string dir) :
                storage_service(ss), mount_point(mp), directory(dir) { }

        std::shared_ptr<StorageService> storage_service;
        std::string mount_point;
        std::string directory;

    };


    /***********************/
    /** \endcond           */
    /***********************/


}


#endif //WRENCH_FILELOCATION_H
