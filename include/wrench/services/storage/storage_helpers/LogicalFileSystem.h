/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_LOGICALFILESYSTEM_H
#define WRENCH_LOGICALFILESYSTEM_H

#include <stdexcept>
#include <string>
#include <map>
#include <set>


#include <wrench/data_file/DataFile.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/


    class StorageService;

    /**
     * @brief  A class that implements a weak file system abstraction
     */
    class LogicalFileSystem {

    public:

        explicit LogicalFileSystem(std::string hostname, StorageService *storage_service, std::string mount_point);

        void init();

        double getTotalCapacity();
        bool hasEnoughFreeSpace(double bytes);
        double getFreeSpace();
        void reserveSpace(std::shared_ptr<DataFile>file, std::string absolute_path);
        void unreserveSpace(std::shared_ptr<DataFile>file, std::string absolute_path);

        void createDirectory(std::string absolute_path);
        bool doesDirectoryExist(std::string absolute_path);
        bool isDirectoryEmpty(std::string absolute_path);
        void removeEmptyDirectory(std::string absolute_path);
        void storeFileInDirectory(std::shared_ptr<DataFile>file, std::string absolute_path);
        void removeFileFromDirectory(std::shared_ptr<DataFile>file, std::string absolute_path);
        void removeAllFilesInDirectory(std::string absolute_path);
        bool isFileInDirectory(std::shared_ptr<DataFile>file, std::string absolute_path);
        std::set<std::shared_ptr<DataFile>> listFilesInDirectory(std::string absolute_path);


    private:

        friend class StorageService;

        void stageFile(std::shared_ptr<DataFile> file, std::string absolute_path);

        static std::map<std::string, StorageService*> mount_points;

        std::map<std::string, std::set<std::shared_ptr<DataFile>>> content;

        std::string hostname;
        StorageService *storage_service;
        std::string mount_point;
        double total_capacity;
        double occupied_space;
        std::map<std::string, double> reserved_space;

        bool initialized;

        void assertInitHasBeenCalled() {
            if (not this->initialized) {
                throw std::runtime_error("LogicalFileSystem::assertInitHasBeenCalled(): A logical file system needs to be initialized before it's been called");
            }
        }
        void assertDirectoryExist(std::string absolute_path) {
            if (not this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " does not exist");
            }
        }

        void assertDirectoryDoesNotExist(std::string absolute_path) {
            if (this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " already exists");
            }
        }

        void assertDirectoryIsEmpty(std::string absolute_path) {
            assertDirectoryExist(absolute_path);
            if (not this->isDirectoryEmpty(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryIsEmpty(): directory " + absolute_path + "is not empty");
            }
        }

        void assertFileIsInDirectory(std::shared_ptr<DataFile>file, std::string absolute_path) {
            assertDirectoryExist(absolute_path);
            if (this->content[absolute_path].find(file) == this->content[absolute_path].end()) {
                throw std::invalid_argument("LogicalFileSystem::assertFileIsInDirectory(): File " + file->getID() +
                " is not in directory " + absolute_path);
            }
        }

    };


    /***********************/
    /** \endcond           */
    /***********************/

}


#endif //WRENCH_LOGICALFILESYSTEM_H
