/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <filesystem>
#include <iostream>

WRENCH_LOG_CATEGORY(wrench_core_file_location, "Log category for FileLocation");

namespace wrench {

    std::shared_ptr<FileLocation> FileLocation::SCRATCH = std::shared_ptr<FileLocation>(new FileLocation(nullptr, "", ""));

    FileLocation::~FileLocation() {
    }

    /**
     * @brief File location specifier for a storage service's (single) mount point root
     *
     * @param ss: a storage service (that has a single mount point)
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(std::shared_ptr<StorageService> ss) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }

        if (ss->hasMultipleMountPoints()) {
            throw std::invalid_argument("FileLocation::LOCATION(): Storage Service has multiple mount points. "
                                        "Call the version of this method that takes a mount point argument");
        }
        return LOCATION(ss, *(ss->getMountPoints().begin()));
    }

    /**
     * @brief File location specifier for a storage service's (single) mount point root
     * Used in case of NFS with page cache.
     * @param ss: storage service on the client (whose the page cache that data is written to)
     * @param server_ss: a server storage service in NFS that stores the file on disk
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(std::shared_ptr<StorageService> ss,
                                                         std::shared_ptr<StorageService> server_ss) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }

        if (ss->hasMultipleMountPoints()) {
            throw std::invalid_argument("FileLocation::LOCATION(): Storage Service has multiple mount points. "
                                        "Call the version of this method that takes a mount point argument");
        }
        std::shared_ptr<FileLocation> location = LOCATION(ss, *(ss->getMountPoints().begin()));
        location->server_storage_service = server_ss;
        return location;
    }

    /**
     * @brief File location specifier given an absolute path at a storage service
     *
     * @param ss: a storage service or ComputeService::SCRATCH
     * @param absolute_path: an absolute path at the storage service to a directory (that may contain files)
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(std::shared_ptr<StorageService> ss,
                                                         std::string absolute_path) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Invalid storage service argument");
        }
        if (absolute_path.empty()) {
            throw std::invalid_argument("FileLocation::LOCATION(): must specify a non-empty path");
        }
        absolute_path = FileLocation::sanitizePath(absolute_path);

        std::string mount_point = "";
        for (auto const &mp : ss->getMountPoints()) {
            // This works because we disallowed two mounts from being proper prefixes of each other
            if ((mp != "/") and (absolute_path.find(mp) == 0)) {
                mount_point = mp;
                break;
            }
        }
        if (mount_point.empty()) {
            if (ss->hasMountPoint("/")) {
                mount_point = "/";
            } else {
                throw std::invalid_argument("FileLocation::LOCATION(): Invalid path '" +
                                            absolute_path + "' at storage service '" + ss->getName() + "'");
            }
        }

        absolute_path.replace(0, mount_point.length(), "/");
        absolute_path = sanitizePath(absolute_path);

        return std::shared_ptr<FileLocation>(new FileLocation(ss, mount_point, absolute_path));
    }

    /**
     * @brief Give a <ss1 name>:<mount point>:<dir>" string for the location
     *
     * @return A string
     */
    std::string FileLocation::toString() {
        if (this == FileLocation::SCRATCH.get()) {
            return "scratch";
        } else {
            return this->storage_service->getName() + ":" +
                   sanitizePath(this->mount_point + this->absolute_path_at_mount_point);
        }
    }

    /**
     * @brief Get the location's storage service
     * @return a storage service
     */
    std::shared_ptr<StorageService> FileLocation::getStorageService() {
        if (this == FileLocation::SCRATCH.get()) {
            throw std::invalid_argument("FileLocation::getStorageService(): Method cannot be called on FileLocation::SCRATCH");
        }
        return this->storage_service;
    }

    /**
     * @brief Get the location's server storage service
     * @return a storage service
     */
    std::shared_ptr<StorageService> FileLocation::getServerStorageService() {
        return this->server_storage_service;
    }

    /**
     * @brief Get the location's mount point
     * @return a mount point
     */
    std::string FileLocation::getMountPoint() {
        if (this == FileLocation::SCRATCH.get()) {
            throw std::invalid_argument("FileLocation::getMountPoint(): Method cannot be called on FileLocation::SCRATCH");
        }
        return this->mount_point;
    }

    /**
     * @brief Get the location's path at mount point
     * @return
     */
    std::string FileLocation::getAbsolutePathAtMountPoint() {
        if (this == FileLocation::SCRATCH.get()) {
            throw std::invalid_argument("FileLocation::getAbsolutePathAtMountPoint(): Method cannot be called on FileLocation::SCRATCH");
        }
        return this->absolute_path_at_mount_point;
    }

    /**
     * @brief Get the location's full absolute path
     * @return
     */
    std::string FileLocation::getFullAbsolutePath() {
        if (this == FileLocation::SCRATCH.get()) {
            throw std::invalid_argument("FileLocation::getFullAbsolutePath(): Method cannot be called on FileLocation::SCRATCH");
        }
        return FileLocation::sanitizePath(this->mount_point + "/" + this->absolute_path_at_mount_point);
    }


    /**
     * @brief Method to sanitize an absolute path (and make it absolute if it's not)
     * @param path: an absolute path
     * @return
     */
    std::string FileLocation::sanitizePath(std::string path) {

        if (path.empty()) {
            throw std::invalid_argument("FileLocation::sanitizePath(): path cannot be empty");
        }

        std::filesystem::path sanitized_path = " /" + path + "/";
        std::string to_return = sanitized_path.lexically_normal();
        to_return.erase(0,1);

        return to_return;

//        // Cannot have certain substring (why not)
////        std::string unallowed_characters[] = {"\\", " ", "~", "`", "\"", "&", "*", "?"};
//        char unallowed_characters[] = {'\\', ' ', '~', '`', '\'', '&', '*', '?'};
//        for (auto const &c : unallowed_characters) {
//            if (path.find(c) != std::string::npos) {
//                throw std::invalid_argument("FileLocation::sanitizePath(): Unallowed character '" + std::to_string(c) + "' in path (" + path + ")");
//            }
//        }
//
//        // Make it /-started and /-terminated
//        if (path.at(path.length()-1) != '/') {
//            path = "/" + path + "/";
//        }
//
//        // Deal with "", "." and ".."
//        std::vector<std::string> tokens;
//        boost::split(tokens, path, boost::is_any_of("/"));
//        tokens.erase(tokens.begin());
//        tokens.pop_back();
//        std::vector<std::string> new_tokens;
//
//        for (auto t : tokens) {
//            if ((t == ".") or t.empty()) {
//                // do nothing
//            } else if (t == "..") {
//                if (new_tokens.empty()) {
//                    throw std::invalid_argument("FileLocation::sanitizePath(): Invalid path (" + path + ")");
//                } else {
//                    new_tokens.pop_back();
//                }
//            } else {
//                new_tokens.push_back(t);
//            }
//        }
//
//        // Reconstruct sanitized path
//        std::string sanitized = "";
//        for  (auto const & t : new_tokens) {
//            sanitized += "/"  + t;
//        }
//        sanitized += "/";
//
//        return sanitized;
    }

    /**
     * @brief Helper method to find if a path is a proper prefix of another path
     * @param path1: a path
     * @param path2: another path
     * @return true if one of the two paths is a proper prefix of the other
     */
    bool FileLocation::properPathPrefix(std::string path1, std::string path2) {
        // Sanitize paths
        path1 = sanitizePath(path1);
        path2 = sanitizePath(path2);

        // Split into tokens
        std::vector<std::string> tokens1, tokens2, shorter, longer;
        boost::split(tokens1, path1, boost::is_any_of("/"));
        boost::split(tokens2, path2, boost::is_any_of("/"));


        if (tokens1.size() < tokens2.size()) {
            shorter = tokens1;
            longer = tokens2;
        } else {
            shorter = tokens2;
            longer = tokens1;
        }

        for (unsigned int i=1; i < shorter.size()-1; i++) {
            if (shorter.at(i) != longer.at(i)) {
                return false;
            }
        }

        return true;
    }
}

