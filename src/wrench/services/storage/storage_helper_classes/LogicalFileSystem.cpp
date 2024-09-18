/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include <wrench/services/storage/storage_helpers/LogicalFileSystem.h>
#include <wrench/services/storage/storage_helpers/LogicalFileSystemNoCaching.h>
#include <wrench/services/storage/storage_helpers/LogicalFileSystemLRUCaching.h>

#include <limits>
#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_logical_file_system, "Log category for Logical File System");


namespace wrench {

    const std::string LogicalFileSystem::DEV_NULL = "/dev/null";
    std::set<std::string> LogicalFileSystem::mount_points;

    /**
     * @brief Method to create a LogicalFileSystem instance
     * @param hostname hostname on which the disk is mounted
     * @param storage_service storage service for which this file system is created
     * @param mount_point the mountpoint
     * @param eviction_policy the cache eviction policy ("NONE", "LRU")
     * @return a shared pointer to a LogicalFileSystem instance
     */
    std::unique_ptr<LogicalFileSystem> LogicalFileSystem::createLogicalFileSystem(const std::string &hostname,
                                                                                  StorageService *storage_service,
                                                                                  const std::string &mount_point,
                                                                                  const std::string &eviction_policy) {

        if (storage_service == nullptr) {
            throw std::invalid_argument("LogicalFileSystem::createLogicalFileSystem(): nullptr storage_service argument");
        }

        // Check for uniqueness
        auto key = hostname + ":" + mount_point;
        if (mount_point != DEV_NULL) {
            auto lfs = LogicalFileSystem::mount_points.find(key);

            if (lfs != LogicalFileSystem::mount_points.end()) {
                throw std::invalid_argument("LogicalFileSystem::createLogicalFileSystem(): A logical file system, used by some storage service, "
                                            "already exist on host " +
                                            hostname + " at mount point " + mount_point);
            }
        }

        //        std::cerr << "CREATING LOGICAL FS " << hostname << ":" << mount_point << "\n";
        std::unique_ptr<LogicalFileSystem> to_return;
        if (eviction_policy == "NONE") {
            to_return = std::unique_ptr<LogicalFileSystem>(new LogicalFileSystemNoCaching(hostname, storage_service, mount_point));
        } else if (eviction_policy == "LRU") {
            to_return = std::unique_ptr<LogicalFileSystem>(new LogicalFileSystemLRUCaching(hostname, storage_service, mount_point));
        } else {
            throw std::invalid_argument("LogicalFileSystem::createLogicalFileSystem(): Unknown cache eviction policy " + eviction_policy);
        }
        LogicalFileSystem::mount_points.insert(key);

        //        std::cerr << "CREATED LOGICAL FS " << hostname << ":" << mount_point << "\n";
        return to_return;
    }


    /**
     * @brief Constructor
     * @param hostname the host on which this file system runs
     * @param storage_service the storage service for which this file system runs
     * @param mount_point the mount point at the host
     */
    LogicalFileSystem::LogicalFileSystem(const std::string &hostname, StorageService *storage_service,
                                         const std::string &mount_point) {

        this->hostname = hostname;
        this->storage_service = storage_service;
        this->content["/"] = {};

        if (mount_point == DEV_NULL) {
            devnull = true;
            this->total_capacity = std::numeric_limits<double>::infinity();
        } else {
            devnull = false;
            this->mount_point = FileLocation::sanitizePath("/" + mount_point + "/");
            // Check validity
            this->disk = S4U_Simulation::hostHasMountPoint(hostname, mount_point);
            if (not this->disk) {
                throw std::invalid_argument("LogicalFileSystem::LogicalFileSystem(): Host " +
                                            hostname + " does not have a disk mounted at " + mount_point);
            }
            this->total_capacity = S4U_Simulation::getDiskCapacity(hostname, mount_point);
        }
        this->free_space = this->total_capacity;
        this->mount_point = mount_point;
    }

    /**
     * @brief Create a new directory
     *
     * @param absolute_path: the directory's absolute path
     * @throw std::invalid_argument
     */
    void LogicalFileSystem::createDirectory(const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");
        assertDirectoryDoesNotExist(fixed_path);
        this->content[fixed_path] = {};
    }

    /**
 * @brief Checks whether a directory exists
 * @param absolute_path the directory's absolute path
 * @return true if the directory exists
 */
    bool LogicalFileSystem::doesDirectoryExist(const std::string &absolute_path) {
        if (devnull) {
            return false;
        }
        if (absolute_path == "/") {// Common case
            return true;
        } else {
            auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");
            return (this->content.find(fixed_path) != this->content.end());
        }
    }

    /**
 * @brief Checks whether a directory is empty
 * @param absolute_path: the directory's absolute path
 * @return true if the directory is empty
 *
 * @throw std::invalid_argument
 */
    bool LogicalFileSystem::isDirectoryEmpty(const std::string &absolute_path) {
        if (devnull) {
            return true;
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");
        assertDirectoryExist(fixed_path);
        return (this->content[fixed_path].empty());
    }

    /**
     * @brief Remove an empty directory
     * @param absolute_path: the directory's absolute path
     *
     * @throw std::invalid_argument
     */
    void LogicalFileSystem::removeEmptyDirectory(const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");
        assertDirectoryExist(fixed_path);
        assertDirectoryIsEmpty(fixed_path);
        this->content.erase(fixed_path);
    }


    /**
 * @brief Checks whether a file is in a directory
 * @param file: the file
 * @param absolute_path: the directory's absolute path
 *
 * @return true if the file is present, false if not (or if directory does not exist)
 *
 * @throw std::invalid_argument
 */
    bool LogicalFileSystem::isFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return false;
        }
        if (absolute_path == "/") {
            return (this->content["/"].find(file) != this->content["/"].end());
        }

        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        // If directory does not exist, say "no"
        if (not doesDirectoryExist(fixed_path)) {
            return false;
        }

        return (this->content[fixed_path].find(file) != this->content[fixed_path].end());
    }

    /**
    * @brief Get the files in a directory as a set
    * @param absolute_path: the directory's absolute path
    *
    * @return a set of files
    *
    * @throw std::invalid_argument
    */
    std::set<std::shared_ptr<DataFile>> LogicalFileSystem::listFilesInDirectory(const std::string &absolute_path) {
        std::set<std::shared_ptr<DataFile>> to_return;
        if (devnull) {
            return {};
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        assertDirectoryExist(fixed_path);
        for (auto const &f: this->content[fixed_path]) {
            to_return.insert(f.first);
        }
        return to_return;
    }

    /**
    * @brief
    * @return Total number of files currently stored in Filesytem
    *
    */
    double LogicalFileSystem::getTotalNumFiles() const {

        double files = 0;

        for (const auto &item: this->content) {
            files += item.second.size();
        }

        return files;
    }

    /**
    * @brief Get the total capacity
    * @return the total capacity
    */
    double LogicalFileSystem::getTotalCapacity() const {
        return this->total_capacity;
    }

    /**
     * @brief Get the file system's free space
     * @return the free space in bytes
     */
    double LogicalFileSystem::getFreeSpace() const {
        return this->free_space;
    }

    /**
    * @brief Reserve space for a file that will be stored
    * @param file: the file
    * @param absolute_path: the path where it will be written
    *
    * @return true on success, false on failure
    */
    bool LogicalFileSystem::reserveSpace(const std::shared_ptr<DataFile> &file,
                                         const std::string &absolute_path) {
        if (devnull) {
            return true;
        }

        std::string fixed_path = "/";
        if (absolute_path != "/") {
            fixed_path = FileLocation::sanitizePath(absolute_path + "/");
        }


        std::string key = fixed_path + file->getID();
        if (this->reserved_space.find(key) != this->reserved_space.end()) {
            WRENCH_WARN("LogicalFileSystem::reserveSpace(): Space was already being reserved for storing file %s at path %s:%s. "
                        "This is likely a redundant copy, and nothing needs to be done",
                        file->getID().c_str(), this->hostname.c_str(), fixed_path.c_str());
            return true;
        }

        if (this->free_space < file->getSize()) {
            if (!this->evictFiles(file->getSize())) {
                return false;
            }
        }

        this->reserved_space[key] = file->getSize();
        this->free_space -= file->getSize();
        return true;
    }

    /**
     * @brief Unreserved space that was saved for a file (likely a failed transfer)
     * @param file: the file
     * @param absolute_path: the path where it would have been written
     * @throw std::invalid_argument
     */
    void LogicalFileSystem::unreserveSpace(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {

        if (this->devnull) {
            return;
        }

        std::string fixed_path = "/";
        if (absolute_path != "/") {
            fixed_path = FileLocation::sanitizePath(absolute_path + "/");
        }

        std::string key = fixed_path + file->getID();

        if (this->reserved_space.find(key) == this->reserved_space.end()) {
            return;// oh well, the transfer was cancelled/terminated/whatever
        }

        this->reserved_space.erase(key);
        this->free_space += file->getSize();
    }


    /**
     * @brief Retrieve the file's last write date
     * @param file: the file
     * @param absolute_path: the file path
     * @return a date in seconds (returns -1.0) if file in not found
     */
    double LogicalFileSystem::getFileLastWriteDate(const shared_ptr<DataFile> &file, const string &absolute_path) {

        if (devnull) {
            return -1;
        }
        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        // If directory does not exist, say "no"
        if (not doesDirectoryExist(fixed_path)) {
            return -1;
        }

        if (this->content[fixed_path].find(file) != this->content[fixed_path].end()) {
            return (this->content[fixed_path][file]->last_write_date);
        } else {
            return -1;
        }
    }


    /**
     * @brief Get the disk on which this file system runs
     * @return The SimGrid disk on which this file system is mounted
     */
    simgrid::s4u::Disk *LogicalFileSystem::getDisk() {
        return this->disk;
    }


    /**
     * @brief Stage file in directory
     * @param file: the file to stage
     * @param absolute_path: the directory's absolute path (at the mount point)
     *
     * @throw std::invalid_argument
     */
    void LogicalFileSystem::stageFile(const std::shared_ptr<DataFile> &file, std::string absolute_path) {
        if (devnull) {
            return;
        }

        //        std::cerr << "LOGICAL FILE SYSTEM: STAGING FILE\n";
        // If Space is not sufficient, forget it
        if (this->free_space < file->getSize()) {
            //            std::cerr << "FREE SPACE = " << this->free_space << "\n";
            throw std::invalid_argument("LogicalFileSystem::stageFile(): Insufficient space to store file " +
                                        file->getID() + " at " + this->hostname + ":" + absolute_path);
        }

        auto fixed_path = FileLocation::sanitizePath(absolute_path + "/");

        this->storeFileInDirectory(file, fixed_path);
    }

}// namespace wrench
