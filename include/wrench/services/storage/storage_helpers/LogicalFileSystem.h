/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_LOGICALFILESYSTEM_H
#define WRENCH_LOGICALFILESYSTEM_H

#include <stdexcept>
#include <string>
#include <map>
#include <set>
#include <memory>

#include <simgrid/disk.h>


#include <wrench/data_file/DataFile.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/


    class StorageService;

    /**
     * @brief  A class that implements a weak file system abstraction
     */
    class LogicalFileSystem {

    public:
        /**
         * @brief A constant that signifies /dev/null, when the actual location/path/mountpoint/etc. is unknown
         */
        const static std::string DEV_NULL;

        explicit LogicalFileSystem(const std::string &hostname, StorageService *storage_service, std::string mount_point = DEV_NULL);

        void init();

        double getTotalCapacity();
        bool hasEnoughFreeSpace(double bytes);
        double getFreeSpace();

        bool reserveSpace(const std::shared_ptr<DataFile> &file,
                          const std::string &absolute_path,
                          const std::string &eviction_policy);
        void unreserveSpace(const std::shared_ptr<DataFile> &file, std::string absolute_path);

        void createDirectory(const std::string &absolute_path);
        bool doesDirectoryExist(const std::string &absolute_path);
        bool isDirectoryEmpty(const std::string &absolute_path);
        void removeEmptyDirectory(const std::string &absolute_path);
        void storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        void removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        void removeAllFilesInDirectory(const std::string &absolute_path);
        bool isFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        void updateReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        double getFileLastReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        std::set<std::shared_ptr<DataFile>> listFilesInDirectory(const std::string &absolute_path);

        simgrid::s4u::Disk *getDisk();


    private:
        friend class StorageService;
        void stageFile(const std::shared_ptr<DataFile> &file, std::string absolute_path);

        bool evictLRUFiles(double needed_free_space);

        static std::map<std::string, StorageService *> mount_points;

        std::map<std::string, std::map<std::shared_ptr<DataFile>, std::pair<double, double>>> content;

        simgrid::s4u::Disk *disk;

        std::string hostname;
        StorageService *storage_service;
        std::string mount_point;
        double total_capacity;
        double occupied_space;
        bool devnull = false;
        std::map<std::string, double> reserved_space;

        bool initialized;

        void assertInitHasBeenCalled() const {
            if (not this->initialized) {
                throw std::runtime_error("LogicalFileSystem::assertInitHasBeenCalled(): A logical file system needs to be initialized before it's been called");
            }
        }
        void assertDirectoryExist(const std::string &absolute_path) {
            if (not this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " does not exist");
            }
        }

        void assertDirectoryDoesNotExist(const std::string &absolute_path) {
            if (this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " already exists");
            }
        }

        void assertDirectoryIsEmpty(const std::string &absolute_path) {
            assertDirectoryExist(absolute_path);
            if (not this->isDirectoryEmpty(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryIsEmpty(): directory " + absolute_path + "is not empty");
            }
        }

        void assertFileIsInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
            assertDirectoryExist(absolute_path);
            if (this->content[absolute_path].find(file) == this->content[absolute_path].end()) {
                throw std::invalid_argument("LogicalFileSystem::assertFileIsInDirectory(): File " + file->getID() +
                                            " is not in directory " + absolute_path);
            }
        }
    };


    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_LOGICALFILESYSTEM_H
