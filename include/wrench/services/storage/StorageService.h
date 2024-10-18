/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_STORAGESERVICE_H
#define WRENCH_STORAGESERVICE_H

#include <string>
#include <set>

#include <fsmod.hpp>

#include "wrench/services/Service.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/job/StandardJob.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench {

    class Simulation;
    class DataFile;

    /**
     * @brief The storage service base class
     */
    class StorageService : public Service {

    public:
        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        /**
         * @brief Stop the servier
         */
        void stop() override;



        /** File Lookup methods (in simulation) **/

        /**
         * @brief Lookup whether a file exists at a location (incurs simulated overheads)
         * @param location a location
         * @return true if the file is present at the location, or false
         */
        static bool lookupFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::lookupFileAtLocation(): invalid argument argument");
            }
            return location->getStorageService()->lookupFile(location);
        }
        /**
         * @brief Lookup whether a file exists on the storage service (incurs simulated overheads)
         * @param file a file
         * @return true if the file is present, or false
         */
        bool lookupFile(const std::shared_ptr<DataFile> &file) {
            return this->lookupFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file));
        }
        /**
         * @brief Lookup whether a file exists on the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         * @return true if the file is present, or false
         */
        virtual bool lookupFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->lookupFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file));
        }
        /**
         * @brief Lookup whether a file exists at a location on the storage service (incurs simulated overheads)
         * @param location a location
         * @return true if the file is present, or false
         */
        virtual bool lookupFile(const std::shared_ptr<FileLocation> &location) {
            return this->lookupFile(S4U_Daemon::getRunningActorRecvCommPort(), location);
        }

        /** File deletion methods **/

        /**
         * @brief Delete a file at a location (incurs simulated overheads)
         * @param location a location
         */
        static void deleteFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::deleteFileAtLocation(): invalid argument argument");
            }
            location->getStorageService()->deleteFile(location);
        }
        /**
         * @brief Delete a file at the storage service (incurs simulated overheads)
         * @param file a file
         */
        void deleteFile(const std::shared_ptr<DataFile> &file) {
            this->deleteFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file));
        }
        /**
         * @brief Delete a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         */
        virtual void deleteFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->deleteFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file));
        }
        /**
         * @brief Delete a file at the storage service (incurs simulated overheads)
         * @param location a location
         */
        virtual void deleteFile(const std::shared_ptr<FileLocation> &location) {
            this->deleteFile(S4U_Daemon::getRunningActorRecvCommPort(), location, true);
        }

        /** File read methods **/
        /**
         * @brief Read a file at a location (incurs simulated overheads)
         * @param location a location
         */
        static void readFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::readFileAtLocation(): invalid argument argument");
            }
            location->getStorageService()->readFile(location, location->getFile()->getSize());
        }
        /**
         * @brief Read a file at a location (incurs simulated overheads)
         * @param location a location
         * @param num_bytes a number of bytes to read
         */
        static void readFileAtLocation(const std::shared_ptr<FileLocation> &location, sg_size_t num_bytes) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::readFileAtLocation(): invalid argument argument");
            }
            location->getStorageService()->readFile(location, num_bytes);
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param file a file
         */
        void readFile(const std::shared_ptr<DataFile> &file) {
            this->readFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file), file->getSize());
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param num_bytes a number of bytes to read
         */
        void readFile(const std::shared_ptr<DataFile> &file, sg_size_t num_bytes) {
            this->readFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file), num_bytes);
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         */
        virtual void readFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->readFile(file, FileLocation::sanitizePath(path), file->getSize());
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         * @param num_bytes a number of bytes to read
         */
        virtual void readFile(const std::shared_ptr<DataFile> &file, const std::string &path, sg_size_t num_bytes) {
            this->readFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file), num_bytes);
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param location a location
         */
        void readFile(const std::shared_ptr<FileLocation> &location) {
            this->readFile(S4U_Daemon::getRunningActorRecvCommPort(), location, location->getFile()->getSize(), true);
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param location a location
         * @param num_bytes a number of bytes to read
         */
        virtual void readFile(const std::shared_ptr<FileLocation> &location, sg_size_t num_bytes) {
            this->readFile(S4U_Daemon::getRunningActorRecvCommPort(), location, num_bytes, true);
        }

        /** File write methods **/
        /**
         * @brief Write a file at a location (incurs simulated overheads)
         * @param location a location
         */
        static void writeFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::writeFileAtLocation(): invalid argument argument");
            }
            location->getStorageService()->writeFile(location);
        }

        /**
         * @brief Write a file at the storage service (incurs simulated overheads)
         * @param file a file
         */
        void writeFile(const std::shared_ptr<DataFile> &file) {
            this->writeFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file));
        }

        /**
         * @brief Write a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         */
        virtual void writeFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->writeFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file));
        }

        /**
         * @brief Write a file at the storage service (incurs simulated overheads)
         * @param location a location
         */
        virtual void writeFile(const std::shared_ptr<FileLocation> &location) {
            this->writeFile(S4U_Daemon::getRunningActorRecvCommPort(), location, location->getFile()->getSize(), true);
        }

        /** Non-Simulation methods **/

        /** File lookup methods */

        /**
         * @brief Determines whether a file is present at a location (in zero simulated time)
         * @param location: a location
         * @return true if the file is present, false otherwise
         */
        static bool hasFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::hasFileAtLocation(): invalid argument argument");
            }
            return location->getStorageService()->hasFile(location);
        }

        /**
         * @brief Determines whether a file is present at the storage service (in zero simulated time)
         * @param file: a file
         * @return true if the file is present, false otherwise
         */
        bool hasFile(const std::shared_ptr<DataFile> &file) {
            return this->hasFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file));
        }

        /**
         * @brief Determines whether a file is present at the storage service (in zero simulated time)
         * @param file: a file
         * @param path: a path
         * @return true if the file is present, false otherwise
         */
        virtual bool hasFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->hasFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file));
        }

        /**
         * @brief Determines whether a file is present at the storage service (in zero simulated time)
         * @param location: a location
         * @return true if the file is present, false otherwise
         */
        virtual bool hasFile(const std::shared_ptr<FileLocation> &location) = 0;

        /** File creation methods */
        /**
         * @brief Create a file at a location (in zero simulated time)
         * @param location: a location
         */
        static void createFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::createFileAtLocation(): invalid argument argument");
            }
            location->getStorageService()->createFile(location);
        }

        /**
         * @brief Return the storage service's default mountpoint, if any.
         * If none, throws an std::runtime_error exception
         */
        virtual std::string getMountPoint() {
            throw std::runtime_error("StorageService::getMountPoint(): no mount point!");
        }

        /**
         * @brief Return the storage service's mountpoints. If none, throws
         * an std::runtime_error exception
         */
        virtual std::set<std::string> getMountPoints() {
            throw std::runtime_error("StorageService::getMountPoints(): no mount points!");
        }

        /**
         * @brief Return the storage service's file system
         * If none, throws an std::runtime_error exception
         */
        virtual std::shared_ptr<simgrid::fsmod::FileSystem> getFileSystem() {
            throw std::runtime_error("StorageService::getFileSystem(): no file system!");
        }


        /**
         * @brief Create a file at the storage service (in zero simulated time)
         * @param file: a file
         */
        virtual void createFile(const std::shared_ptr<DataFile> &file) {
            this->createFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file));
        }

        /**
         * @brief Create a file at the storage service (in zero simulated time)
         * @param file: a file
         * @param path: a path
         */
        virtual void createFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->createFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file));
        }

        /**
         * @brief Create a file at the storage service (in zero simulated time)
         * @param location: a location
         */
        virtual void createFile(const std::shared_ptr<FileLocation> &location) = 0;


        /** File removal methods */
        /**
         * @brief Remove a file at a location (in zero simulated time)
         * @param location: a location
         */
        static void removeFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::removeFileAtLocation(): invalid argument argument");
            }
            location->getStorageService()->removeFile(location);
        }
        /**
         * @brief Remove a file at the storage service (in zero simulated time)
         * @param file: a file
         */
        void removeFile(const std::shared_ptr<DataFile> &file) {
            this->removeFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file));
        }
        /**
         * @brief Remove a file at the storage service (in zero simulated time)
         * @param file: a file
         * @param path: a path
         */
        virtual void removeFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->removeFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file));
        }
        /**
         * @brief Remove a file at the storage service (in zero simulated time)
         * @param location: a location
         */
        virtual void removeFile(const std::shared_ptr<FileLocation> &location) = 0;


        /**
         * @brief Remove a directory and all files at the storage service (in zero simulated time)
         * @param path a path
         */
        virtual void removeDirectory(const std::string &path) = 0;

        /** File write date methods */
        /**
         * @brief Get a file's last write date at a location (in zero simulated time)
         * @param location:  a location
         * @return a date in seconds, or -1 if the file is not found
         */
        double getFileLocationLastWriteDate(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::getFileLocationLastWriteDate(): invalid argument argument");
            }
            return this->getFileLastWriteDate(location);
        }
        /**
         * @brief Get a file's last write date at the storage service (in zero simulated time)
         * @param file: a file
         * @return a date in seconds
         */
        double getFileLastWriteDate(const std::shared_ptr<DataFile> &file) {
            return this->getFileLastWriteDate(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file));
        }
        /**
         * @brief Get a file's last write date at the storage service (in zero simulated time)
         * @param file: a file
         * @param path: a path
         * @return a date in seconds
         */
        virtual double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->getFileLastWriteDate(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), FileLocation::sanitizePath(path), file));
        }
        /**
         * @brief Get a file's last write date at the storage service (in zero simulated time)
         * @param location: a location
         * @return a date in seconds
         */
        virtual double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) = 0;

        /** Service load methods */
        /**
         * @brief Get the storage service's load
         * @return a load metric
         */
        virtual double getLoad() = 0;

        /** Service total space method */
        /**
         * @brief Get the storage service's total space (in zero simulated time)
         * @return a capacity in bytes
         */
        virtual sg_size_t getTotalSpace() = 0;

        /** Service free space method */
        /**
         * @brief Get the storage service's total free space (incurs simulated overhead)
         * @return a capacity in bytes
         */
        virtual sg_size_t getTotalFreeSpace();

        /** Service free space method */
        /**
         * @brief Get the storage service's free space at a path (incurs simulated overhead)
         * @brief path a path
         * @return a capacity in bytes
         */
        virtual sg_size_t getTotalFreeSpaceAtPath(const std::string &path);

        /** Service free space tracing (doesn't incur simulated overhead) */
        /**
         *  @brief Get the storage service's total free space (no simulated overhead)
         *  @return Current free space in bytes
         *
         */
        virtual sg_size_t getTotalFreeSpaceZeroTime() {
            throw std::runtime_error("StorageService::traceTotalFreeSpace: should have been overridden by derived class");
        }

        /** Service number of allocated files tracing (doesn't incur simulated overhead) */
        /**
         *  @brief Get the number of files registered to the filesystem(s) associated with this service (no simulated overhead)
         *  @return Current number of registered Datafile for all filesystem(s) from this service
         *
         */
        virtual unsigned long getTotalFilesZeroTime() {
            throw std::runtime_error("StorageService::traceTotalFiles: should have been overridden by derived class");
        }

        /**
         * @brief Get the storage's service base root path
         * @return a path
         */
        virtual std::string getBaseRootPath() {
            throw std::runtime_error("StorageService::traceTotalFiles: should have been overridden by derived class");
        }

        /**
         * @brief Copy a file from one location to another
         * @param src_location: a source location
         * @param dst_location: a destination location
         */
        static void copyFile(const std::shared_ptr<FileLocation> &src_location,
                             const std::shared_ptr<FileLocation> &dst_location);

        /**
         * @brief Helper method to read multiple files
         *
         * @param locations: a map of files to locations
         */
        static void readFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        /**
         * @brief Helper method to write multiple files
         *
         * @param locations: a map of files to locations
         */
        static void writeFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        /**
         * @brief Determine whether the storage service is bufferized
         * @return true if bufferized, false otherwise
         */
        virtual bool isBufferized() const = 0;
        /**
         * @brief Determine the storage service's buffer size
         * @return a size in bytes
         */
        virtual sg_size_t getBufferSize() const = 0;

        /**
         * @brief Determines whether the storage service is a scratch service of a ComputeService
         * @return true if it is, false otherwise
         */
        bool isScratch() const {
            return this->is_scratch_;
        }

        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        /**
         * @brief Initiate a file copy from one location to another
         * @param answer_commport: a commport_name on which to receive completion/failure notification
         * @param src_location: a source location
         * @param dst_location: a destination location
         */
        static void initiateFileCopy(S4U_CommPort *answer_commport,
                                     const std::shared_ptr<FileLocation> &src_location,
                                     const std::shared_ptr<FileLocation> &dst_location);

        /**
         * @brief Reserve space at the storage service
         * @param location: a location
         * @return true if success, false otherwise
         */
        virtual bool reserveSpace(std::shared_ptr<FileLocation> &location) = 0;

        /**
         * @brief Unreserve space at the storage service
         * @param location: a location
         */
        virtual void unreserveSpace(std::shared_ptr<FileLocation> &location) = 0;

        virtual void deleteFile(S4U_CommPort *answer_commport,
                                const std::shared_ptr<FileLocation> &location,
                                bool wait_for_answer);

        virtual bool lookupFile(S4U_CommPort *answer_commport,
                                const std::shared_ptr<FileLocation> &location);

        virtual void readFile(S4U_CommPort *answer_commport,
                              const std::shared_ptr<FileLocation> &location,
                              sg_size_t num_bytes,
                              bool wait_for_answer);

        virtual void writeFile(S4U_CommPort *answer_commport,
                               const std::shared_ptr<FileLocation> &location,
                               sg_size_t num_bytes_to_write,
                               bool wait_for_answer);

        /**
         * @brief Decrement the number of operations for a location
         * @param location: a location
         **/
        virtual void decrementNumRunningOperationsForLocation(const std::shared_ptr<FileLocation> &location) {
            // do nothing
        }

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:
        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        friend class ComputeService;

        StorageService(const std::string &hostname,
                       const std::string &service_name);

        virtual void setIsScratch(bool is_scratch);

        /** Fast-Access common message payloads! **/
        //        double StorageServiceMessagePayload_FILE_READ_REQUEST_MESSAGE_PAYLOAD;
        //        double StorageServiceMessagePayload_FILE_READ_ANSWER_MESSAGE_PAYLOAD;
        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        enum FileOperation {
            READ,
            WRITE,
        };


        virtual void writeFilePartial(const std::shared_ptr<FileLocation> &location, sg_size_t num_bytes_to_write) {
            this->writeFile(S4U_Daemon::getRunningActorRecvCommPort(), location, num_bytes_to_write, true);
        }

        static void writeOrReadFiles(FileOperation action,
                                     std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        bool is_scratch_;
    };


}// namespace wrench


#endif//WRENCH_STORAGESERVICE_H
