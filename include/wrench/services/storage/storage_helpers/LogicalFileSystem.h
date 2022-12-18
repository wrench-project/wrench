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
#include <unordered_map>
#include <set>
#include <memory>
#include <iostream>

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
        virtual ~LogicalFileSystem() = default;

        class FileOnDisk {
        public:
            explicit FileOnDisk(double last_write_date) : last_write_date(last_write_date) {}

            double last_write_date;
        };

        /**
         * @brief A constant that signifies /dev/null, when the actual location/path/mountpoint/etc. is unknown
         */
        const static std::string DEV_NULL;

        static std::unique_ptr<LogicalFileSystem> createLogicalFileSystem(const std::string &hostname,
                                                                          StorageService *storage_service,
                                                                          const std::string &mount_point = DEV_NULL,
                                                                          const std::string &eviction_policy = "NONE");

        void init();
        bool isInitialized();

        double getTotalCapacity();
        double getFreeSpace();
        //        bool hasEnoughFreeSpace(double bytes);

        void stageFile(const std::shared_ptr<DataFile> &file, std::string absolute_path);

        bool reserveSpace(const std::shared_ptr<DataFile> &file,
                          const std::string &absolute_path);
        void unreserveSpace(const std::shared_ptr<DataFile> &file, std::string absolute_path);

        void createDirectory(const std::string &absolute_path);
        bool doesDirectoryExist(const std::string &absolute_path);
        bool isDirectoryEmpty(const std::string &absolute_path);
        void removeEmptyDirectory(const std::string &absolute_path);

        bool isFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        std::set<std::shared_ptr<DataFile>> listFilesInDirectory(const std::string &absolute_path);
        simgrid::s4u::Disk *getDisk();

        double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);

        virtual void storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path, bool must_be_initialized) = 0;
        virtual void removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;
        virtual void removeAllFilesInDirectory(const std::string &absolute_path) = 0;
        virtual void updateReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;
        virtual void incrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;
        virtual void decrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;

    protected:
        friend class StorageService;
        friend class Simulation;

        LogicalFileSystem(const std::string &hostname, StorageService *storage_service,
                          const std::string &mount_point);

        bool devnull = false;
        virtual bool evictFiles(double needed_free_space) = 0;
        static std::map<std::string, StorageService *> mount_points;
        simgrid::s4u::Disk *disk;
        std::string hostname;
        StorageService *storage_service;
        std::string mount_point;
        double total_capacity;
        double free_space = 0;
        std::map<std::string, double> reserved_space;

        bool initialized;
        std::unordered_map<std::string, std::map<std::shared_ptr<DataFile>, std::shared_ptr<LogicalFileSystem::FileOnDisk>>> content;

        void assertInitHasBeenCalled() const {
            if (not this->initialized) {
                std::cerr << "IN ASSERT; " << this->hostname << ":" << this->mount_point << "\n";
                throw std::runtime_error("LogicalFileSystem::assertInitHasBeenCalled(): A logical file system needs to be initialized before it's called");
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
