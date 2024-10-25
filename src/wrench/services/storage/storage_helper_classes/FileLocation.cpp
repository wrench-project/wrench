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
#include <wrench/services/storage/simple/SimpleStorageService.h>
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
    long FileLocation::dot_file_sequence_number = 0;


    FileLocation::~FileLocation() = default;

    /**
     * @brief Factory to create a new file location
     * @param ss: a storage service
     * @param file: a file
     * @param path: a path
     * @param is_scratch: whether scratch or not
     * @return a shared pointer to a file location
     */
    std::shared_ptr<FileLocation> FileLocation::createFileLocation(const std::shared_ptr<StorageService> &ss,
                                                                   const std::shared_ptr<DataFile> &file,
                                                                   const std::string &path,
                                                                   bool is_scratch) {
        // This looks inefficient (compare to using std::hash<void*> for instance), but it has
        // no collisions and benchmarking showed that (with compiler optimizations), it makes
        // very little difference
        std::string key = (ss ? ss->getName() : "") + "|" + path + "|" + file->getID() + "|" + (is_scratch ? "1" : "0");
        if (FileLocation::file_location_map.find(key) != FileLocation::file_location_map.end()) {
            return FileLocation::file_location_map[key];
        }
        auto new_location = std::shared_ptr<FileLocation>(new FileLocation(ss, file, path, is_scratch));

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
     */
    std::shared_ptr<FileLocation> FileLocation::SCRATCH(const std::shared_ptr<DataFile> &file) {
        if (file == nullptr) {
            throw std::invalid_argument("FileLocation::SCRATCH(): Cannot pass nullptr file");
        }

        return FileLocation::createFileLocation(nullptr, file, "", true);
    }

    /**
     * @brief File location specifier for a storage service's (single) mount point root
     *
     * @param ss: a storage service (that has a single mount point)
     * @param file: a file
     * @return a file location specification
     *
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(const std::shared_ptr<StorageService> &ss, const std::shared_ptr<DataFile> &file) {
        if (!ss or !file) {
            throw std::invalid_argument("FileLocation::LOCATION(): invalid nullptr arguments");
        }
        return LOCATION(ss, ss->getMountPoint(), file);
    }

#ifdef PAGE_CACHE_SIMULATION
    /**
     * @brief File location specifier for a storage service's (single) mount point root
     * Used in case of NFS with page cache.
     * @param ss: storage service on the client (whose the page cache that data is written to)
     * @param server_ss: a server storage service in NFS that stores the file on disk
     * @param file: a file
     * @return a file location specification
     *
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(const std::shared_ptr<StorageService> &ss,
                                                         const std::shared_ptr<StorageService> &server_ss,
                                                         const std::shared_ptr<DataFile> &file) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }

        std::shared_ptr<FileLocation> location = LOCATION(ss, file, "/");
        location->server_storage_service = server_ss;
        return location;
    }
#endif

    /**
     * @brief File location specifier given an absolute path at a storage service
     *
     * @param ss: a storage service
     * @param file: a file
     * @param path: a path
     * @return a file location specification
     *
     */
    std::shared_ptr<FileLocation> FileLocation::LOCATION(const std::shared_ptr<StorageService> &ss,
                                                         const std::string &path,
                                                         const std::shared_ptr<DataFile> &file) {
        if (ss == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr storage service");
        }
        if (file == nullptr) {
            throw std::invalid_argument("FileLocation::LOCATION(): Cannot pass nullptr file");
        }
        if (path.empty()) {
            throw std::invalid_argument("FileLocation::LOCATION(): must specify a non-empty path");
        }

        return FileLocation::createFileLocation(ss, file, FileLocation::sanitizePath(path + "/"), false);
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
                   sanitizePath(this->directory_path) + ":" + this->file->getID();
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
     * @brief Set location's storage service
     *
     * @param storage_service: the storage service
     *
     * @return The updated storage service
     */
    std::shared_ptr<StorageService> FileLocation::setStorageService(std::shared_ptr<StorageService> &storage_service) {
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

#ifdef PAGE_CACHE_SIMULATION
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
#endif

    /**
     * @brief Get the location's directory path
     * @return a path
     */
    std::string FileLocation::getDirectoryPath() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getDirectoryPath(): No path for a SCRATCH location");
        }
        return this->directory_path;
    }

    /**
     * @brief Get the location's file path
     * @return a path
     */
    std::string FileLocation::getFilePath() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getFilePath(): No path for a SCRATCH location");
        }
        return this->directory_path + "/" + this->file->getID();
    }

    /**
     * @brief Get a path to a dot-file for the location
     * @return a path
     */
    std::string FileLocation::getADotFilePath() {
        if (this->is_scratch) {
            throw std::invalid_argument("FileLocation::getDotFilePath(): No path for a SCRATCH location");
        }
        return this->directory_path + "/" + this->file->getID() + ".wrench_tmp." + std::to_string(FileLocation::dot_file_sequence_number++);
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
    std::string FileLocation::sanitizePath(const std::string &path) {
        if (path == "/") return "/";// make the common case fast

        if (path.empty()) {
            throw std::invalid_argument("FileLocation::sanitizePath(): path cannot be empty");
        }

#if 0
        // THIS MAKES ONLY A TINY BIT OF A PERFORMANCE BOOST
        // (ONLY WORKS WITH C++17)
        std::filesystem::path sanitized_path =path + "/";
        std::string to_return = sanitized_path.lexically_normal();
        return to_return;
#else

        // Cannot have certain substring (why not)
        char disallowed_characters[] = {'\\', ' ', '~', '`', '\'', '"', '&', '*', '?', '.'};
        for (auto const &c: disallowed_characters) {
            if (path.find(c) != std::string::npos) {
                throw std::invalid_argument("FileLocation::sanitizePath(): Disallowed character '" + std::string(1, c) + "' in path (" + path + ")");
            }
        }

        std::string sanitized = "/";
        sanitized += path;
        sanitized += "/";
        // Make it /-started and /-terminated
        //        if (sanitized.at(sanitized.length() - 1) != '/') {
        //            sanitized += "/";
        //            sanitized += path;
        //            sanitized += "/";
        //        } else {
        //            sanitized = path;
        //        }


        // Deal with "", "." and ".."
        std::vector<std::string> tokens;
        boost::split(tokens, sanitized, boost::is_any_of("/"));
        tokens.erase(tokens.begin());
        tokens.pop_back();
        //        std::vector<std::string> new_tokens;

        //        for (auto const &t: tokens) {
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

        // Reconstruct sanitized path
        sanitized = "";
        for (auto const &t: tokens) {
            if (not t.empty()) {
                sanitized += "/";
                sanitized += t;
            }
        }
        sanitized += "/";

        return sanitized;
#endif
    }

    /**
     * @brief Helper method to find if a path is a proper prefix of another path
     * @param path1: a path (ALREADY SANITIZED)
     * @param path2: another path (ALREADY SANITIZED)
     * @return true if one of the two paths is a proper prefix of the other
     */
    bool FileLocation::properPathPrefix(const std::string &path1, const std::string &path2) {

        return ((path2.size() >= path1.size() && path2.compare(0, path1.size(), path1) == 0) or
                (path2.size() < path1.size() && path1.compare(0, path2.size(), path2) == 0));
    }

    /**
     * @brief Gets the simgrid disk associated to a location
     * @return a simgrid disk or nullpts if none
     */
    simgrid::s4u::Disk *FileLocation::getDiskOrNull() {
        if (this->is_scratch) {
            return nullptr;
        }
        auto sss = std::dynamic_pointer_cast<SimpleStorageService>(this->storage_service);
        if (not sss) {
            return nullptr;
        }
        return sss->getDiskForPathOrNull(this->directory_path);
    }


}// namespace wrench
