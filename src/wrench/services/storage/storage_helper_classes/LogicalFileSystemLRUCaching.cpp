/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/services/storage/storage_helpers/LogicalFileSystemLRUCaching.h>

#include <limits>
#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_logical_file_system_lru_caching, "Log category for Logical File System LRU Caching");


namespace wrench {

    /**
     * @brief Constructor
     * @param hostname: the host on which the file system is located
     * @param storage_service: the storage service this file system is for
     * @param mount_point: the mount point, defaults to /dev/null
     */
    LogicalFileSystemLRUCaching::LogicalFileSystemLRUCaching(const std::string &hostname, StorageService *storage_service, const std::string &mount_point)
            : LogicalFileSystem(hostname, storage_service, mount_point) {
    }


/**
 * @brief Store file in directory
 *
 * @param file: the file to store
 * @param absolute_path: the directory's absolute path (at the mount point)
 * @param must_be_initialized: whether the file system is initialized
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystemLRUCaching::storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path, bool must_be_initialized) {
        if (devnull) {
            return;
        }
        if (must_be_initialized) {
            assertInitHasBeenCalled();
        }
        // If directory does not exit, create it
        if (not doesDirectoryExist(absolute_path)) {
            createDirectory(absolute_path);
        }

        this->content[absolute_path][file] = std::make_shared<FileOnDiskLRUCaching>(S4U_Simulation::getClock(), this->next_lru_sequence_number++, 0);
        this->lru_list[this->next_lru_sequence_number - 1] = std::make_tuple(absolute_path, file);

        std::string key = FileLocation::sanitizePath(absolute_path) + file->getID();
        if (this->reserved_space.find(key) == this->reserved_space.end()) {
            this->free_space -= file->getSize();
        } else {
            this->reserved_space.erase(key);
        }
    }

    /**
 * @brief Remove a file in a directory
 * @param file: the file to remove
 * @param absolute_path: the directory's absolute path
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystemLRUCaching::removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        assertDirectoryExist(absolute_path);
        assertFileIsInDirectory(file, absolute_path);
        auto seq = std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[absolute_path][file])->lru_sequence_number;
        this->content[absolute_path].erase(file);
        this->lru_list.erase(seq);
        this->free_space += file->getSize();
    }

    /**
 * @brief Remove all files in a directory
 * @param absolute_path: the directory's absolute path
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystemLRUCaching::removeAllFilesInDirectory(const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        assertDirectoryExist(absolute_path);
        double freed_space = 0;
        for (auto const &s : this->content[absolute_path]) {
            freed_space += s.first->getSize();
        }
        this->content[absolute_path].clear();
        this->free_space += freed_space;

        for (auto const &c : this->content[absolute_path]) {
            this->lru_list.erase(std::static_pointer_cast<FileOnDiskLRUCaching>(c.second)->lru_sequence_number);
        }
        this->content[absolute_path].clear();
    }


    /**
     * @brief Update a file's read date
     * @param file: the file
     * @param absolute_path: the path
     */
    void LogicalFileSystemLRUCaching::updateReadDate(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        // If directory does not exist, do nothing
        if (not doesDirectoryExist(absolute_path)) {
            return;
        }

        if (this->content[absolute_path].find(file) != this->content[absolute_path].end()) {
            std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[absolute_path][file])->lru_sequence_number++;
        }
    }

    /**
     * @brief Evict LRU files to create some free space
     * @param needed_free_space: the needed free space
     * @return true on success, false on failure
     */
    bool LogicalFileSystemLRUCaching::evictFiles(double needed_free_space) {

        double total_reserved_space = 0;
        for (auto const &x : this->reserved_space) {
            total_reserved_space += x.second;
        }

        if (this->total_capacity - total_reserved_space < needed_free_space) return false;

        while(this->free_space < needed_free_space) {
            unsigned int key = this->lru_list.begin()->first;
            auto path = std::get<0>(this->lru_list.begin()->second);
            auto file = std::get<1>(this->lru_list.begin()->second);
            this->lru_list.erase(key);
            this->content[path].erase(file);
            this->free_space += file->getSize();
        }
        return true;

    }

}// namespace wrench
