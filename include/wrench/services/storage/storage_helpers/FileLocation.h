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
#include <iostream>
#include <utility>
#include <unordered_map>


namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/


    class StorageService;
    class DataFile;

    /**
     * @brief  A class that encodes  a file location
     */
    class FileLocation {

    public:
        ~FileLocation();

        /**
         * @brief Static location that denotes a compute service's scratch space
         */
        static std::shared_ptr<FileLocation> LOCATION(const std::shared_ptr<StorageService> &ss, const std::shared_ptr<DataFile> &file);

        static std::shared_ptr<FileLocation> LOCATION(const std::shared_ptr<StorageService> &ss,
                                                      std::shared_ptr<StorageService> server_ss,
                                                      const std::shared_ptr<DataFile> &file);

        static std::shared_ptr<FileLocation> LOCATION(const std::shared_ptr<StorageService> &ss,
                                                      std::string absolute_path,
                                                      const std::shared_ptr<DataFile> &file);

        static std::shared_ptr<FileLocation> SCRATCH(const std::shared_ptr<DataFile> &file);

        std::shared_ptr<DataFile> getFile();
        std::shared_ptr<StorageService> getStorageService();
        std::shared_ptr<StorageService> getServerStorageService();
        std::string getMountPoint();
        std::string getAbsolutePathAtMountPoint();
        std::string getFullAbsolutePath();
        bool isScratch() const;
        std::string toString();


        /**
         * @brief Method to compare two file locations
         *
         * @param lhs: a file location
         * @param rhs: a file location
         *
         * @return true if both locations are equivalent (always returns false if at least one location is SCRATCH)
         *
         */
        static bool equal(const std::shared_ptr<FileLocation> &lhs,
                          const std::shared_ptr<FileLocation> &rhs) {
            return ((not lhs->is_scratch) and
                    (not rhs->is_scratch) and
                    (lhs->storage_service == rhs->storage_service) and
                    (lhs->getFullAbsolutePath() == rhs->getFullAbsolutePath()) and
                    (lhs->file == rhs->file));
        }
        /**
         * @brief Method to compare a file location with another
         *
         * @param other: a file location
         *
         * @return true if both locations are equivalent (always returns false if at least one location is SCRATCH)
         *
         */
        bool equal(const std::shared_ptr<FileLocation> &other) {
            return ((not this->is_scratch) and
                    (not other->is_scratch) and
                    (this->getStorageService() == other->getStorageService()) and
                    (this->getFullAbsolutePath() == other->getFullAbsolutePath()) and
                    (this->getFile() == other->getFile()));
        }


        static std::string sanitizePath(std::string path);
        static bool properPathPrefix(std::string path1, std::string path2);

    private:
        friend class LogicalFileSystem;


        /**
         * @brief Constructor
         * @param ss: the storage service
         * @param mp: the mount point path
         * @param apamp: the absolute path
         * @param file: the file
	 * @param is_scratch: whether the location is a SCRATCH location
         */
        FileLocation(std::shared_ptr<StorageService> ss, std::string mp, std::string apamp, std::shared_ptr<DataFile> file, bool is_scratch) : storage_service(std::move(ss)),
                                                                                                                                               mount_point(std::move(mp)),
                                                                                                                                               absolute_path_at_mount_point(std::move(apamp)),
                                                                                                                                               file(std::move(std::move(file))),
                                                                                                                                               is_scratch(is_scratch) {}

        std::shared_ptr<StorageService> storage_service;
        std::shared_ptr<StorageService> server_storage_service;
        std::string mount_point;
        std::string absolute_path_at_mount_point;
        std::shared_ptr<DataFile> file;
        bool is_scratch;

        static std::shared_ptr<FileLocation> createFileLocation(const std::shared_ptr<StorageService> &ss,
                                                                const std::string &mp,
                                                                const std::string &apamp,
                                                                const std::shared_ptr<DataFile> &file,
                                                                bool is_scratch);

        static void reclaimFileLocations();

        static std::unordered_map<std::string, std::shared_ptr<FileLocation>> file_location_map;
        static size_t file_location_map_previous_size;
    };

    /***********************/
    /** \endcond           */
    /***********************/


}// namespace wrench


#endif//WRENCH_FILELOCATION_H
