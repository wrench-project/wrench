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

#include <limits>
#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_logical_file_system, "Log category for Logical File System");


namespace wrench {
    const std::string LogicalFileSystem::DEV_NULL = "/dev/null";
    std::map<std::string, StorageService *> LogicalFileSystem::mount_points;

    /**
     * @brief Constructor
     * @param hostname: the host on which the file system is located
     * @param storage_service: the storage service this file system is for
     * @param mount_point: the mount point, defaults to /dev/null
     */
    LogicalFileSystem::LogicalFileSystem(const std::string &hostname, StorageService *storage_service, std::string mount_point) {
        if (storage_service == nullptr) {
            throw std::invalid_argument("LogicalFileSystem::LogicalFileSystem(): nullptr storage_service argument");
        }
        this->hostname = hostname;
        this->storage_service = storage_service;
        this->content["/"] = {};

        this->occupied_space = 0;
        this->initialized = false;
        if (mount_point == DEV_NULL) {
            devnull = true;
            this->total_capacity = std::numeric_limits<double>::infinity();
        } else {
            devnull = false;
            mount_point = FileLocation::sanitizePath("/" + mount_point + "/");
            // Check validity
            this->disk = S4U_Simulation::hostHasMountPoint(hostname, mount_point);
            if (not this->disk) {
                throw std::invalid_argument("LogicalFileSystem::LogicalFileSystem(): Host " +
                                            hostname + " does not have a disk mounted at " + mount_point);
            }
            this->total_capacity = S4U_Simulation::getDiskCapacity(hostname, mount_point);
        }
        this->mount_point = mount_point;
    }


    /**
     * @brief Initializes the Logical File System (must be called before any other operation on this file system)
     */
    void LogicalFileSystem::init() {
        // Check uniqueness
        auto lfs = LogicalFileSystem::mount_points.find(this->hostname + ":" + this->mount_point);

        if (lfs != LogicalFileSystem::mount_points.end() && mount_point != DEV_NULL) {
            if (lfs->second != this->storage_service) {
                throw std::invalid_argument("LogicalFileSystem::init(): A FileSystem with mount point " + this->mount_point + " at host " + this->hostname + " already exists");
            } else {
                return;
            }
        } else {
            LogicalFileSystem::mount_points[this->hostname + ":" + this->mount_point] = this->storage_service;
            this->initialized = true;
        }
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
        assertInitHasBeenCalled();
        assertDirectoryDoesNotExist(absolute_path);
        this->content[absolute_path] = {};
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
        assertInitHasBeenCalled();
        return (this->content.find(absolute_path) != this->content.end());
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
        assertInitHasBeenCalled();
        assertDirectoryExist(absolute_path);
        return (this->content[absolute_path].empty());
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
        assertInitHasBeenCalled();
        assertDirectoryExist(absolute_path);
        assertDirectoryIsEmpty(absolute_path);
        this->content.erase(absolute_path);
    }

    /**
 * @brief Store file in directory
 *
 * @param file: the file to store
 * @param absolute_path: the directory's absolute path (at the mount point)
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::storeFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        // If directory does not exit, create it
        if (not doesDirectoryExist(absolute_path)) {
            createDirectory(absolute_path);
        }

        this->content[absolute_path][file] = S4U_Simulation::getClock();
        std::string key = FileLocation::sanitizePath(absolute_path) + file->getID();
        if (this->reserved_space.find(key) == this->reserved_space.end()) {
            this->occupied_space += file->getSize();
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
    void LogicalFileSystem::removeFileFromDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        assertDirectoryExist(absolute_path);
        assertFileIsInDirectory(file, absolute_path);
        this->content[absolute_path].erase(file);
        this->occupied_space -= file->getSize();
    }

    /**
 * @brief Remove all files in a directory
 * @param absolute_path: the directory's absolute path
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::removeAllFilesInDirectory(const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        assertDirectoryExist(absolute_path);
        this->content[absolute_path].clear();
    }

    /**
 * @brief Checks whether a file is in a directory
 * @param file: the file
 * @param absolute_path: the directory's absolute path
 *
 * @return true if the file is present
 *
 * @throw std::invalid_argument
 */
    bool LogicalFileSystem::isFileInDirectory(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return false;
        }
        assertInitHasBeenCalled();
        std::cerr << "IN isFileInDirectory() , looking for file " << file->getID() << " in " << absolute_path << "\n";
        // If directory does not exist, say "no"
        if (not doesDirectoryExist(absolute_path)) {
            std::cerr << "DIRECTORY DOES NOT EXIST\n";
            return false;
        }
        std::cerr << "DIRECTORY EXISTS!\n";

        return (this->content[absolute_path].find(file) != this->content[absolute_path].end());
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
        assertInitHasBeenCalled();
        assertDirectoryExist(absolute_path);
        for (auto const &f: this->content[absolute_path]) {
            to_return.insert(f.first);
        }
        return to_return;
    }

    /**
 * @brief Get the total capacity
 * @return the total capacity
 */
    double LogicalFileSystem::getTotalCapacity() {
        assertInitHasBeenCalled();
        return this->total_capacity;
    }

    /**
 * @brief Checks whether there is enough space to store some number of bytes
 * @param bytes: a number of bytes
 * @return true if the number of bytes can fit
 */
    bool LogicalFileSystem::hasEnoughFreeSpace(double bytes) {
        assertInitHasBeenCalled();
        return (this->total_capacity - this->occupied_space) >= bytes;
    }

    /**
 * @brief Get the file system's free space
 * @return the free space in bytes
 */
    double LogicalFileSystem::getFreeSpace() {
        assertInitHasBeenCalled();
        return (this->total_capacity - this->occupied_space);
    }

    /**
 * @brief Reserve space for a file that will be stored
 * @param file: the file
 * @param absolute_path: the path where it will be written
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::reserveSpace(const std::shared_ptr<DataFile> &file, const std::string &absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        std::string key = FileLocation::sanitizePath(absolute_path) + file->getID();

        if (this->total_capacity - this->occupied_space < file->getSize()) {
            throw std::invalid_argument("LogicalFileSystem::reserveSpace(): Not enough free space");
        }
        if (this->reserved_space.find(key) != this->reserved_space.end()) {
            WRENCH_WARN("LogicalFileSystem::reserveSpace(): Space was already being reserved for storing file %s at path %s:%s. This is likely a redundant copy",
                        file->getID().c_str(), this->hostname.c_str(), absolute_path.c_str());
        } else {
            this->reserved_space[key] = file->getSize();
            this->occupied_space += file->getSize();
        }
    }

    /**
 * @brief Unreserve space that was saved for a file (likely a failed transfer)
 * @param file: the file
 * @param absolute_path: the path where it would have been written
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::unreserveSpace(const std::shared_ptr<DataFile> &file, std::string absolute_path) {
        if (devnull) {
            return;
        }
        assertInitHasBeenCalled();
        std::string key = FileLocation::sanitizePath(std::move(absolute_path)) + file->getID();

        if (this->reserved_space.find(key) == this->reserved_space.end()) {
            return;// oh well, the transfer was cancelled/terminated/whatever
        }

        //         This will never happen
        //        if (this->occupied_space <  file->getSize()) {
        //            throw std::invalid_argument("LogicalFileSystem::unreserveSpace(): Occupied space is less than the file size... should not happen");
        //        }

        this->reserved_space.erase(key);
        this->occupied_space -= file->getSize();
    }


    /**
     * @brief Stage file in directory
     * @param file: the file to stage
     * @param absolute_path: the directory's absolute path (at the mount point)
     *
     * @throw std::invalid_argument
     */
    void LogicalFileSystem::stageFile(const std::shared_ptr<DataFile> &file, std::string absolute_path) {
        std::cerr << "IN STAGE FILE LFS: " << absolute_path << "\n";
        if (devnull) {
            return;
        }
        std::cerr << "NOT DEVNULL\n";
        // If Space is not sufficient, forget it
        if (this->occupied_space + file->getSize() > this->total_capacity) {
            throw std::invalid_argument("LogicalFileSystem::stageFile(): Insufficient space to store file " +
                                        file->getID() + " at " + this->hostname + ":" + absolute_path);
        }

        absolute_path = wrench::FileLocation::sanitizePath(absolute_path);

        // If directory does not exit, create it
        if (this->content.find(absolute_path) == this->content.end()) {
            this->content[absolute_path] = {};
        }

        // If file does  not already exist, create it
        if (this->content.find(absolute_path) != this->content.end()) {
            std::cerr << "DOING IT!\n";
            std::cerr << "THIS->CONTENT[" << absolute_path << "][" << file->getID() << "] is set!\n";
            this->content[absolute_path][file] = S4U_Simulation::getClock();
            this->occupied_space += file->getSize();
        } else {
            std::cerr << "NOT DOING IT!\n";
            return;
        }
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
        assertInitHasBeenCalled();
        // If directory does not exist, say "no"
        if (not doesDirectoryExist(absolute_path)) {
            return -1;
        }

        if (this->content[absolute_path].find(file) != this->content[absolute_path].end()) {
            return this->content[absolute_path][file];
        } else {
            return -1;
        }
    }

    /**
     * @brief
     * @return The SimGrid disk on which this file system is mounted
     */
    simgrid::s4u::Disk *LogicalFileSystem::getDisk() {
        return this->disk;
    }


}// namespace wrench
