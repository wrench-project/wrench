/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench-dev.h>
#include "LogicalFileSystem.h"

namespace wrench {

    std::set<std::string> LogicalFileSystem::mount_points;

    /**
     * @brief Constructor
     * @param mount_point: the mount point
     */
    LogicalFileSystem::LogicalFileSystem(std::string hostname, std::string mount_point) {

        // Sanitize the mount_point
        if (mount_point.at(0) != '/') {
            mount_point = "/" + mount_point;
        }
        if (mount_point.at(mount_point.length()) != '/') {
            mount_point += "/";
        }

        mount_point = FileLocation::sanitizePath(mount_point);

        // Check uniqueness
        if (LogicalFileSystem::mount_points.find(hostname+":"+mount_point) != LogicalFileSystem::mount_points.end()) {
            throw std::invalid_argument("LogicalFileSystem::LogicalFileSystem(): A FileSystem with mount point " +
                                        mount_point + " at host " + hostname + " already exists");
        }
        // Check non-proper-prefixness
        for (auto const &mp : LogicalFileSystem::mount_points) {
            if ((mp.find(hostname+":"+mount_point) == 0) or ((hostname+":"+mount_point).find(mp) == 0)) {
                throw std::invalid_argument("LogicalFileSystem::LogicalFileSystem(): An existing mount point that "
                                            "has as prefix or is a prefix of '" + mount_point +
                                            "' already exists at" + "host " + hostname);
            }
        }


        LogicalFileSystem::mount_points.insert(hostname+":"+mount_point);

        this->mount_point = mount_point;
        this->content["/"] = {};
        this->total_capacity = S4U_Simulation::getDiskCapacity(hostname, mount_point);
        this->occupied_space = 0;
    }


/**
 * @brief Create a new directory
 *
 * @param absolute_path: the directory's absolute path
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::createDirectory(std::string absolute_path) {
        assertDirectoryDoesNotExist(absolute_path);
        this->content[absolute_path] = {};
    }

/**
 * @brief Checks whether a directory exists
 * @param absolute_path the directory's absolute path
 * @return true if the directory exists
 */
    bool LogicalFileSystem::doesDirectoryExist(std::string absolute_path) {
        return (this->content.find(absolute_path) != this->content.end());
    }

/**
 * @brief Checks whether a directory is empty
 * @param absolute_path: the directory's absolute path
 * @return true if the directory is empty
 *
 * @throw std::invalid_argument
 */
    bool LogicalFileSystem::isDirectoryEmpty(std::string absolute_path) {
        assertDirectoryExist(absolute_path);
        return (this->content[absolute_path].empty());
    }

/**
 * @brief Remove an empty directory
 * @param absolute_path: the directory's absolute path
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::removeEmptyDirectory(std::string absolute_path) {
        assertDirectoryExist(absolute_path);
        assertDirectoryIsEmpty(absolute_path);
        this->content.erase(absolute_path);
    }

/**
 * @brief Store file in directory
 * @param absolute_path: the directory's absolute path
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::storeFileInDirectory(WorkflowFile *file, std::string absolute_path) {
        assertDirectoryExist(absolute_path);
        this->content[absolute_path].insert(file);
    }

/**
 * @brief Remove a file in a directory
 * @param file: the file to remove
 * @param absolute_path: the directory's absolute path
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::removeFileFromDirectory(WorkflowFile *file, std::string absolute_path) {
        assertDirectoryExist(absolute_path);
        assertFileIsInDirectory(file, absolute_path);
    }

/**
 * @brief Remove all files in a directory
 * @param absolute_path: the directory's absolute path
 *
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::removeAllFilesInDirectory(std::string absolute_path) {
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
    bool LogicalFileSystem::isFileInDirectory(WorkflowFile *file, std::string absolute_path) {
        assertDirectoryExist(absolute_path);
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
    std::set<WorkflowFile *> LogicalFileSystem::listFilesInDirectory(std::string absolute_path) {
        assertDirectoryExist(absolute_path);
        return this->content[absolute_path];
    }

/**
 * @brief Get the total capacity
 * @return the total capacity
 */
    double LogicalFileSystem::getTotalCapacity() {
        return this->total_capacity;
    }

/**
 * @brief Checks whether there is enough space to store some number of bytes
 * @param bytes: a number of bytes
 * @return true if the number of bytes can fit
 */
    bool LogicalFileSystem::hasEnoughFreeSpace(double bytes) {
        return (this->total_capacity - this->occupied_space) >= bytes;
    }

/**
 * @brief Get the file system's free space
 * @return the free space in bytes
 */
    double LogicalFileSystem::getFreeSpace() {
        return (this->total_capacity - this->occupied_space);
    }

/**
 * @brief Decrease the amount of free space the service "thinks" it has
 * @param num_bytes: the number of bytes by which to decrease
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::decreaseFreeSpace(double num_bytes) {
        if (this->total_capacity - this->occupied_space < num_bytes) {
            throw std::invalid_argument("LogicalFileSystem::decrementFreeSpace(): Not enough free space");
        }
        this->occupied_space += num_bytes;
    }

/**
 * @brief Increase the amount of free space the service "thinks" it has
 * @param num_bytes: the number of bytes by which to increase
 * @throw std::invalid_argument
 */
    void LogicalFileSystem::increaseFreeSpace(double num_bytes) {
        if (this->occupied_space + num_bytes > this->total_capacity) {
            throw std::invalid_argument("LogicalFileSystem::decrementFreeSpace(): No enough capacity");
        }
        this->occupied_space -= num_bytes;
    }

}
