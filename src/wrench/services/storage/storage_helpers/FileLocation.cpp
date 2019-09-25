/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <memory>
#include <wrench/services/storage/StorageService.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


namespace wrench {


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
     * @brief File location specifier given an absolute path at a storage service
     *
     * @param ss: a storage service
     * @param absolute_path: an absolute path at the storage service to a directory (that may contain files)
     * @return a file location specification
     *
     * @throw std::invalid_argument
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(std::shared_ptr<StorageService> ss,
                                                         std::string absolute_path) {
        if ((ss == nullptr) or (absolute_path.empty())) {
            throw std::invalid_argument("FileLocation::LOCATION(): Invalid arguments");
        }

        try {
            absolute_path = FileLocation::sanitizePath(absolute_path);
        } catch (std::invalid_argument &e) {
            throw;
        }


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
                                            absolute_path +"' at storage service '" + ss->getName() + "'");
            }
        }

        absolute_path.replace(0, mount_point.length(), "/");

        return std::shared_ptr<FileLocation>(new FileLocation(ss, mount_point, absolute_path));
    }

    /**
     * @brief Give a <ss name>:<mount point>:<dir>" string for the location
     */
    std::string FileLocation::toString() {
        return this->storage_service->getName() + ":" + this->mount_point + this->absolute_path_at_mount_point;
    }

    /**
     * @brief Get the location's storage service
     * @return a storage service
     */
    std::shared_ptr<StorageService> FileLocation::getStorageService() {
        return this->storage_service;
    }

    /**
     * @brief Get the location's mount point
     * @return a mount point
     */
    std::string FileLocation::getMountPoint() {
        return this->mount_point;
    }

    /**
     * @brief Get the location's storage service's mount point
     * @return
     */
    std::string FileLocation::getAbsolutePathAtMountPoint() {
        return this->absolute_path_at_mount_point;
    }


    /**
     * @brief Method to sanitize an absolute path
     * @param path
     * @return
     */
    std::string FileLocation::sanitizePath(std::string path) {


        if (path.empty()) {
            throw std::invalid_argument("FileLocation::sanitizePath(): path cannot be empty");
        }

        // Must terminate with a /
        if (path.at(0) != '/') {
            throw std::invalid_argument("FileLocation::sanitizePath(): An absolute path must start with '/' (" + path + ")");
        }

        // Cannot have certain substring (why not)
        std::string unallowed_characters[] = {"\\", " ", "~", "`", "\"", "&", "*", "?"};
        for (auto const &c : unallowed_characters) {
            if (path.find(c) != std::string::npos) {
                throw std::invalid_argument("FileLocation::sanitizePath(): Unallowed character '" + c + "' in path (" + path + ")");
            }
        }

        // Make it /-terminated
        if (path.at(path.length()-1) != '/') {
            path = path + "/";
        }

        // Deal with "", "." and ".."
        std::vector<std::string> tokens;
        boost::split(tokens, path, boost::is_any_of("/"));
        tokens.erase(tokens.begin());
        tokens.pop_back();
        std::vector<std::string> new_tokens;

        for (auto t : tokens) {
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
        for  (auto const & t : new_tokens) {
            sanitized += "/"  + t;
        }
        sanitized += "/";

        return sanitized;
    }


}

