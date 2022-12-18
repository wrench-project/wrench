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
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        // If directory does not exit, create it
        if (not doesDirectoryExist(fixed_path)) {
            createDirectory(fixed_path);
        }

        bool file_already_there = this->content[fixed_path].find(file) != this->content[fixed_path].end();

        // If the file was already there, remove its LRU entry
        if (file_already_there) {
            unsigned old_seq = std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[fixed_path][file])->lru_sequence_number;
            this->lru_list.erase(old_seq);
        }

        this->content[fixed_path][file] = std::make_shared<FileOnDiskLRUCaching>(S4U_Simulation::getClock(), this->next_lru_sequence_number++, 0);
        this->lru_list[this->next_lru_sequence_number - 1] = std::make_tuple(fixed_path, file);

        //        print_lru_list();

        std::string key = FileLocation::sanitizePath(fixed_path) + file->getID();
        if (this->reserved_space.find(key) != this->reserved_space.end()) {
            this->reserved_space.erase(key);
        } else if (not file_already_there) {
            this->free_space -= file->getSize();
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
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        assertDirectoryExist(fixed_path);
        assertFileIsInDirectory(file, fixed_path);
        auto seq = std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[fixed_path][file])->lru_sequence_number;
        this->content[fixed_path].erase(file);
        this->lru_list.erase(seq);
        //        print_lru_list();
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
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        assertDirectoryExist(fixed_path);
        double freed_space = 0;
        for (auto const &s: this->content[fixed_path]) {
            freed_space += s.first->getSize();
        }
        for (auto const &c: this->content[fixed_path]) {
            this->lru_list.erase(std::static_pointer_cast<FileOnDiskLRUCaching>(c.second)->lru_sequence_number);
        }

        this->content[fixed_path].clear();
        this->free_space += freed_space;
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
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        // If directory does not exist, do nothing
        if (not doesDirectoryExist(fixed_path)) {
            return;
        }

        if (this->content[fixed_path].find(file) != this->content[fixed_path].end()) {
            unsigned new_seq = this->next_lru_sequence_number++;
            unsigned old_seq = std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[fixed_path][file])->lru_sequence_number;
            this->lru_list.erase(old_seq);
            std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[fixed_path][file])->lru_sequence_number = new_seq;
            this->lru_list[new_seq] = std::make_tuple(fixed_path, file);
            //            print_lru_list();
        }
    }

    /**
     * @brief Evict LRU files to create some free space
     * @param needed_free_space: the needed free space
     * @return true on success, false on failure
     */
    bool LogicalFileSystemLRUCaching::evictFiles(double needed_free_space) {

        // Easy case: there is already enough space without evicting anything
        if (this->free_space >= needed_free_space) {
            return true;
        }

        // Otherwise, we have to try to evict evictable files
        // The worst-case complexity is O(n) here, but we expect
        // very few files to not be evictable. Besides, non-evictable
        // files are likely recently used anyway.
        std::vector<unsigned int> to_evict;
        double freeable_space = 0;
        for (auto const &lru: this->lru_list) {
            auto path = std::get<0>(lru.second);
            auto file = std::get<1>(lru.second);
            if (std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[path][file])->num_current_transactions > 0) {
                //                std::cerr << "PASSING OVER " << path << ":" << file->getID() << "\n";
                continue;
            }
            to_evict.push_back(lru.first);
            freeable_space += file->getSize();
            if (this->free_space + freeable_space >= needed_free_space) {
                break;
            }
        }

        // Perhaps that wasn't enough
        if (this->free_space + freeable_space < needed_free_space) {
            return false;
        }

        // If it was enough, simply remove files
        for (const unsigned int &key: to_evict) {
            auto path = std::get<0>(this->lru_list[key]);
            auto file = std::get<1>(this->lru_list[key]);
            //            std::cerr << "Evicting file " <<  path.c_str() << ":" <<  file->getID().c_str() << "\n";
            WRENCH_INFO("Evicting file %s:%s", path.c_str(), file->getID().c_str());
            this->lru_list.erase(key);
            this->content[path].erase(file);
            this->free_space += file->getSize();
        }
        //        print_lru_list();

        return true;
    }

    /**
     * @brief Increment the number of running transactions that have to do with a file
     * @param file: the file
     * @param absolute_path: the file path
     */
    void LogicalFileSystemLRUCaching::incrementNumRunningTransactionsForFileInDirectory(const shared_ptr<DataFile> &file, const string &absolute_path) {

        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        if ((this->content.find(fixed_path) != this->content.end()) and
            (this->content[fixed_path].find(file) != this->content[fixed_path].end())) {
            std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[fixed_path][file])->num_current_transactions++;
        }
    }

    /**
     * @brief Decrement the number of running transactions that have to do with a file
     * @param file: the file
     * @param absolute_path: the file path
     */
    void LogicalFileSystemLRUCaching::decrementNumRunningTransactionsForFileInDirectory(const shared_ptr<DataFile> &file, const string &absolute_path) {

        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        if ((this->content.find(fixed_path) != this->content.end()) and
            (this->content[fixed_path].find(file) != this->content[fixed_path].end())) {
            std::static_pointer_cast<FileOnDiskLRUCaching>(this->content[fixed_path][file])->num_current_transactions--;
        }
    }


}// namespace wrench
