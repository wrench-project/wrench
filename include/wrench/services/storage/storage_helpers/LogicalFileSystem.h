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

        /**
	 * @brief A helper class to describe a file instance on the file system
	 */
        class FileOnDisk {
        public:
            /**
	     * @brief Constructor
	     * @param last_write_date: the file's last write date
	     */
            explicit FileOnDisk(double last_write_date) : last_write_date(last_write_date) {}

            /**
	     * @brief the file's last write date
	     */
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

        double getTotalCapacity() const;
        double getFreeSpace() const;
        //        bool hasEnoughFreeSpace(double bytes);

        void stageFile(const std::shared_ptr<DataFile> &file, std::string absolute_path);

        bool reserveSpace(const std::shared_ptr<DataFile> &file,
                          const std::string &absolute_path);
        void unreserveSpace(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);

        void createDirectory(const std::string &absolute_path);
        bool doesDirectoryExist(const std::string &absolute_path);
        bool isDirectoryEmpty(const std::string &absolute_path);
        void removeEmptyDirectory(const std::string &absolute_path);

        bool isFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);
        std::set<std::shared_ptr<DataFile>> listFilesInDirectory(const std::string &absolute_path);
        simgrid::s4u::Disk *getDisk();

        double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path);

        /**
         * @brief Store file in directory
         *
         * @param file: the file to store
         * @param absolute_path: the directory's absolute path (at the mount point)
         *
         * @throw std::invalid_argument
         */
        virtual void storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;
        /**
         * @brief Remove a file in a directory
         * @param file: the file to remove
         * @param absolute_path: the directory's absolute path
         *
         * @throw std::invalid_argument
         */
        virtual void removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;
        /**
         * @brief Remove all files in a directory
         * @param absolute_path: the directory's absolute path
         *
         * @throw std::invalid_argument
         */
        virtual void removeAllFilesInDirectory(const std::string &absolute_path) = 0;
        /**
         * @brief Update a file's read date
         * @param file: the file
         * @param absolute_path: the path
         */
        virtual void updateReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;
        /**
         * @brief Increment the number of running transactions that have to do with a file
         * @param file: the file
         * @param absolute_path: the file path
         */
        virtual void incrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;
        /**
         * @brief Decrement the number of running transactions that have to do with a file
         * @param file: the file
         * @param absolute_path: the file path
         */
        virtual void decrementNumRunningTransactionsForFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) = 0;

    protected:
        friend class StorageService;
        friend class Simulation;

        LogicalFileSystem(const std::string &hostname, StorageService *storage_service,
                          const std::string &mount_point);

        /**
         * @brief Static "list" of mountpoints in the simulation
         */
        static std::set<std::string> mount_points;

        /**
         * @brief Whether this file system is /dev/null
         */
        bool devnull = false;
        /**
         * @brief Method to evict files (based on caching policy)
         * @param needed_free_space the amount of free space needed
         * @return true on success, false on failure
         */
        virtual bool evictFiles(double needed_free_space) = 0;

        /**
         * @brief The disk
         */
        simgrid::s4u::Disk *disk;
        /**
         * @brief The hostname
         */
        std::string hostname;
        /**
         * @brief The storage service
         */
        StorageService *storage_service;
        /**
         * @brief The mount point
         */
        std::string mount_point;
        /**
         * @brief The total capacity
         */
        double total_capacity;
        /**
         * @brief current amount of free space
         */
        double free_space = 0;
        /**
         * @brief list of space reservations
         */
        std::map<std::string, double> reserved_space;

        /**
         * @brief file system content
         */
        std::unordered_map<std::string, std::map<std::shared_ptr<DataFile>, std::shared_ptr<LogicalFileSystem::FileOnDisk>>> content;

        /**
         * @brief Assert that a directory exists
         * @param absolute_path: the path
         */
        void assertDirectoryExist(const std::string &absolute_path) {
            if (not this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " does not exist");
            }
        }

        /**
         * @brief Assert that a directory does not exist
         * @param absolute_path: the path
         */
        void assertDirectoryDoesNotExist(const std::string &absolute_path) {
            if (this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " already exists");
            }
        }

        /**
         * @brief Assert that a directory is empty
         * @param absolute_path: the path
         */
        void assertDirectoryIsEmpty(const std::string &absolute_path) {
            assertDirectoryExist(absolute_path);
            if (not this->isDirectoryEmpty(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryIsEmpty(): directory " + absolute_path + "is not empty");
            }
        }

        /**
         * @brief Assert that a file is in a directory
         * @param file: the file
         * @param absolute_path: the path
         */
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
