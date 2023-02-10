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
#include <wrench/services/storage/compound/CompoundStorageService.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <iostream>

#if (__cpluplus >= 201703L)
#include <filesystem>
#endif

WRENCH_LOG_CATEGORY(wrench_core_file_location, "Log category for FileLocation");

#define RECLAIM_TRIGGER 10000

namespace wrench {

    std::unordered_map<std::string, std::shared_ptr<FileLocation>> FileLocation::file_location_map;
    size_t FileLocation::file_location_map_previous_size = 0;

    FileLocation::~FileLocation() {
    }

    /**
     * @brief Factory to create a new file location
     * @param ss: a storage service
     * @param mp: a mount point
     * @param apamp: an path at the mount point
     * @param file: a file
     * @param is_scratch: whether scratch or not
     * @return a shared pointer to a file location
     */
    std::shared_ptr<FileLocation> FileLocation::createFileLocation(const std::shared_ptr<StorageService> &ss,
                                                                   const std::string &mp,
                                                                   const std::string &apamp,
                                                                   const std::shared_ptr<DataFile> &file,
                                                                   bool is_scratch) {
        // TODO: Find a more efficiency key?
        std::string key = (ss ? ss->getName() : "") + "|" + mp + "|" + apamp + "|" + file->getID() + "|" + (is_scratch ? "1" : "0");
        if (FileLocation::file_location_map.find(key) != FileLocation::file_location_map.end()) {
            return FileLocation::file_location_map[key];
        }
        auto new_location = std::shared_ptr<FileLocation>(new FileLocation(ss, mp, apamp, file, is_scratch));

        if (FileLocation::file_location_map.size() - FileLocation::file_location_map_previous_size > RECLAIM_TRIGGER) {
            FileLocation::reclaimFileLocations();
            FileLocation::file_location_map_previous_size = FileLocation::file_location_map.size();
        }
        FileLocation::file_location_map[key] = new_location;
        return new_location;
    }


    /**
     * @brief Reclaim file locations that are no longer used
     */
    void FileLocation::reclaimFileLocations() {
        for (auto it = FileLocation::file_location_map.cbegin(); it != FileLocation::file_location_map.cend();) {
            if ((*it).second.use_count() == 1) {
                it = FileLocation::file_location_map.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief File location specifier for a scratch storage
     *
     * @param file: a file
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::SCRATCH(const std::shared_ptr<DataFile> &file) {
        if (file == nullptr) {
            throw std::invalid_argument("FileLocation::SCRATCH(): Cannot pass nullptr file");
        }

        return FileLocation::createFileLocation(nullptr, "", "", file, true);
    }

    /**
     * @brief File location specifier for a storage service's (single) mount point root
     *
     * @param ss: a storage service (that has a single mount point)
     * @param file: a file
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(const std::shared_ptr<StorageService> &ss, const std::shared_ptr<DataFile> &file) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }
        if (file == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr file");
        }

        if (ss->hasMultipleMountPoints()) {
            throw std::invalid_argument("FileLocation::LOCATION(): Storage Service has multiple mount points. "
                                        "Call the version of this method that takes a mount point argument");
        }
        return LOCATION(ss, ss->getMountPoint(), file);
    }

    /**
     * @brief File location specifier for a storage service's (single) mount point root
     * Used in case of NFS with page cache.
     * @param ss: storage service on the client (whose the page cache that data is written to)
     * @param server_ss: a server storage service in NFS that stores the file on disk
     * @param file: a file
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(const std::shared_ptr<StorageService> &ss,
                                                         std::shared_ptr<StorageService> server_ss,
                                                         const std::shared_ptr<DataFile> &file) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }

        if (ss->hasMultipleMountPoints()) {
            throw std::invalid_argument("FileLocation::LOCATION(): Storage Service has multiple mount points. "
                                        "Call the version of this method that takes a mount point argument");
        }
        std::shared_ptr<FileLocation> location = LOCATION(ss, *(ss->getMountPoints().begin()), file);
        location->server_storage_service = server_ss;
        return location;
    }

    /**
     * @brief File location specifier given an absolute path at a storage service
     *
     * @param ss: a storage service
     * @param absolute_path: an absolute path at the storage service to a directory (that may contain files)
     * @param file: a file
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(const std::shared_ptr<StorageService> &ss,
                                                         std::string absolute_path,
                                                         const std::shared_ptr<DataFile> &file) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }
        if (file == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr file");
        }
        if (absolute_path.empty()) {
            throw std::invalid_argument("FileLocation::LOCATION(): must specify a non-empty path");
        }
        absolute_path = FileLocation::sanitizePath(absolute_path + "/");

        std::string mount_point = "";
        for (auto const &mp: ss->getMountPoints()) {
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

        return FileLocation::createFileLocation(ss, mount_point, absolute_path, file, false);
    }

    /**
     * @brief Give a <ss1 name>:<mount point>:<dir>" string for the location
     *
     * @return A string
     */
    std::string FileLocation::toString() {
        if (this->is_scratch) {
            return "SCRATCH:" + this->file->getID();
        } else {
            return this->storage_service->getName() + ":" +
                   sanitizePath(this->mount_point + this->absolute_path_at_mount_point) + ":" + this->file->getID();
        }
    }

    /**
     * @brief Get the location's storage service
     * @return a storage service
     */
    std::shared_ptr<StorageService> FileLocation::getStorageService() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getStorageService(): No storage service for a SCRATCH location");
        }
        return this->storage_service;
    }

    /**
     * @brief Update storage service
     * @return The updated storage service
     */
    std::shared_ptr<StorageService> FileLocation::setStorageService(std::shared_ptr<StorageService> storage_service) {
        this->storage_service = std::move(storage_service);
        return this->storage_service;
    }


    /**
     * @brief Get the location's file
     * @return a file
     */
    std::shared_ptr<DataFile> FileLocation::getFile() {
        return this->file;
    }

    /**
     * @brief Get the location's server storage service
     * @return a storage service
     */
    std::shared_ptr<StorageService> FileLocation::getServerStorageService() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getServerStorageService(): No server storage service for a SCRATCH location");
        }
        return this->server_storage_service;
    }

    /**
     * @brief Get the location's mount point
     * @return a mount point
     */
    std::string FileLocation::getMountPoint() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getMountPoint(): No mount point for a SCRATCH location");
        }
        return this->mount_point;
    }

    std::string FileLocation::setMountPoint(std::string mount_point) {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getMountPoint(): No mount point for a SCRATCH location");
        }
        this->mount_point = std::move(mount_point);
        return this->mount_point;
    }


    /**
     * @brief Get the location's path at mount point
     * @return a path
     */
    std::string FileLocation::getAbsolutePathAtMountPoint() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getAbsolutePathAtMountPoint(): No path at mount point for a SCRATCH location");
        }
        return this->absolute_path_at_mount_point;
    }

    /**
     * @brief Get the location's full absolute path
     * @return a path
     */
    std::string FileLocation::getFullAbsolutePath() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getFullAbsolutePath(): No full absolute path for a SCRATCH location");
        }
        return FileLocation::sanitizePath(this->mount_point + "/" + this->absolute_path_at_mount_point);
    }

    /**
     * @brief Get the location's scratch-ness
     * @return true if the location is SCRATCH
     */
    bool FileLocation::isScratch() const {
        return this->is_scratch;
    }

    /**
     * @brief Method to sanitize an absolute path (and make it absolute if it's not)
     * @param path: an absolute path
     * @return a sanitized path
     */
    std::string FileLocation::sanitizePath(std::string path) {
        if (path == "/") return "/";// make the common case fast

        if (path.empty()) {
            throw std::invalid_argument("FileLocation::sanitizePath(): path cannot be empty");
        }

#if (__cpluplus >= 201703L)
        // Adding a leading space because, weirdly, lexically_normal() doesn't behave
        // correctly on Linux without it (it doesn't reduce "////" to "/", but does
        // reduce " ////" to " /"
        std::filesystem::path sanitized_path = " /" + path + "/";
        std::string to_return = sanitized_path.lexically_normal();
        // Remove the extra space
        to_return.erase(0, 1);
        return to_return;
#else
        // Cannot have certain substring (why not)
        //        std::string disallowed_characters[] = {"\\", " ", "~", "`", "\"", "&", "*", "?"};
        char disallowed_characters[] = {'\\', ' ', '~', '`', '\'', '&', '*', '?'};
        for (auto const &c: disallowed_characters) {
            if (path.find(c) != std::string::npos) {
                throw std::invalid_argument("FileLocation::sanitizePath(): Disallowed character '" + std::to_string(c) + "' in path (" + path + ")");
            }
        }

        // Make it /-started and /-terminated
        if (path.at(path.length() - 1) != '/') {
            path = "/" + path + "/";
        }

        // Deal with "", "." and ".."
        std::vector<std::string> tokens;
        boost::split(tokens, path, boost::is_any_of("/"));
        tokens.erase(tokens.begin());
        tokens.pop_back();
        std::vector<std::string> new_tokens;

        for (auto t: tokens) {
            if ((t == ".") or t.empty()) {
                // do nothing
            } else if (t == "..") {
                if (new_tokens.empty()) {
                    throw std::invalid_argument("FileLocation::sanitizePath(): Invalid path (" + path + ")");
                } else {
                    new_tokens.pop_back();
                }
            } else {
                new_tokens.push_back(t);
            }
        }

        // Reconstruct sanitized path
        std::string sanitized = "";
        for (auto const &t: new_tokens) {
            sanitized += "/" + t;
        }
        sanitized += "/";

        return sanitized;
#endif
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

        for (unsigned int i = 1; i < shorter.size() - 1; i++) {
            if (shorter.at(i) != longer.at(i)) {
                return false;
            }
        }

        return true;
    }


}// namespace wrench
