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

    /**
     * @brief  A class that encodes  a file location
     */
    class FileLocation {

    public:

        /**
         * @brief Static location that denotes a compute service's scratch space
         */
        static std::shared_ptr<FileLocation> SCRATCH;

        static std::shared_ptr<FileLocation> LOCATION(std::shared_ptr<StorageService> ss);

        static std::shared_ptr<FileLocation> LOCATION(std::shared_ptr<StorageService> ss,
                std::shared_ptr<StorageService> server_ss);

        static std::shared_ptr<FileLocation> LOCATION(std::shared_ptr<StorageService> ss,
                                                      std::string absolute_path);

        std::shared_ptr<StorageService> getStorageService();
        std::shared_ptr<StorageService> getServerStorageService();
        std::string getMountPoint();
        std::string getAbsolutePathAtMountPoint();
        std::string getFullAbsolutePath();

        std::string toString();

        /**
         * @brief Method to compare two file locations
         *
         * @param lhs: a file location
         * @param rhs: a file location
         *
         * @return true if both locations are equivalent
         *
         */
        static bool equal(const std::shared_ptr<FileLocation> &lhs,
            const std::shared_ptr<FileLocation> &rhs) {
          return ((lhs->getStorageService() == rhs->getStorageService()) and
                  (lhs->getFullAbsolutePath() == rhs->getFullAbsolutePath()));
        }


        static std::string sanitizePath(std::string path);
        static bool properPathPrefix(std::string path1, std::string path2);

    private:

        friend class LogicalFileSystem;

        /**
         * @brief Constructor
         * @param ss: the storage service
         * @param ap: the absolute path
         */
        FileLocation(std::shared_ptr<StorageService> ss, std::string mp, std::string apamp) :
                storage_service(ss), mount_point(mp), absolute_path_at_mount_point(apamp) { }

        std::shared_ptr<StorageService> storage_service;
        std::shared_ptr<StorageService> server_storage_service;
        std::string mount_point;
        std::string absolute_path_at_mount_point;


    };

    /***********************/
    /** \endcond           */
    /***********************/


}


#endif //WRENCH_FILELOCATION_H
