/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/services/storage/storage_helpers/LogicalFileSystemNoCaching.h>

#include <limits>
#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_logical_file_system_no_caching, "Log category for Logical File System No caching");


namespace wrench {

    /**
     * @brief Constructor
     * @param hostname: the host on which the file system is located
     * @param storage_service: the storage service this file system is for
     * @param mount_point: the mount point, defaults to /dev/null
     */
    LogicalFileSystemNoCaching::LogicalFileSystemNoCaching(const std::string &hostname, StorageService *storage_service,
                                                           const std::string &mount_point) : LogicalFileSystem(hostname, storage_service, mount_point) {
    }


    /**
     * @brief Store file in directory
     *
     * @param file: the file to store
     * @param absolute_path: the directory's absolute path (at the mount point)
     *
     * @throw std::invalid_argument
     */
    void LogicalFileSystemNoCaching::storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        // If directory does not exit, create it
        if (not doesDirectoryExist(fixed_path)) {
            createDirectory(fixed_path);
        }

        bool file_already_there = this->content[fixed_path].find(file) != this->content[fixed_path].end();

        std::string key = FileLocation::sanitizePath(fixed_path) + file->getID();
        if (this->reserved_space.find(key) != this->reserved_space.end()) {
            this->reserved_space.erase(key);
        } else if (not file_already_there) {
            this->free_space -= file->getSize();
        }
        if (this->free_space < 0) {
            throw std::runtime_error("LogicalFileSystemNoCaching::storeFileInDirectory(): free space is <0, should never happen");
        }

        this->content[fixed_path][file] = std::make_shared<LogicalFileSystemNoCaching::FileOnDiskNoCaching>(S4U_Simulation::getClock());
    }

    /**
     * @brief Remove a file in a directory
     * @param file: the file to remove
     * @param absolute_path: the directory's absolute path
     *
     * @throw std::invalid_argument
     */
    void LogicalFileSystemNoCaching::removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        assertDirectoryExist(fixed_path);
        assertFileIsInDirectory(file, fixed_path);
        this->content[fixed_path].erase(file);
        this->free_space += file->getSize();
    }

    /**
     * @brief Remove all files in a directory
     * @param absolute_path: the directory's absolute path
     *
     * @throw std::invalid_argument
     */
    void LogicalFileSystemNoCaching::removeAllFilesInDirectory(const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        assertDirectoryExist(fixed_path);
        double freed_space = 0;
        for (auto const &s: this->content[fixed_path]) {
            freed_space += s.first->getSize();
        }
        this->content[fixed_path].clear();
        this->free_space += freed_space;
    }


    /**
     * @brief Update a file's read date
     * @param file: the file
     * @param absolute_path: the path
     */
    void LogicalFileSystemNoCaching::updateReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        // Do nothing, as with no caching we don't care about the read date (for now at least)
    }


    /**
     * @brief Evict files to create some free space
     * @param needed_free_space: the needed free space
     * @return always returns false since NoCaching means no evictions
     */
    bool LogicalFileSystemNoCaching::evictFiles(double needed_free_space) {
        return false;
    }

    /**
     * @brief Increment the number of running transactions that have to do with a file
     * @param file: the file
     * @param absolute_path: the file path
     */
    void LogicalFileSystemNoCaching::incrementNumRunningTransactionsForFileInDirectory(const shared_ptr<DataFile> &file, const string &absolute_path) {
        // Do nothing since no caching
    }

    /**
     * @brief Decrement the number of running transactions that have to do with a file
     * @param file: the file
     * @param absolute_path: the file path
     */
    void LogicalFileSystemNoCaching::decrementNumRunningTransactionsForFileInDirectory(const shared_ptr<DataFile> &file, const string &absolute_path) {
        // Do nothing since no caching
    }


}// namespace wrench
