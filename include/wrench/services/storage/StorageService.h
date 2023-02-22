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

#include "wrench/services/Service.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/job/StandardJob.h"
#include "wrench/services/storage/storage_helpers/LogicalFileSystem.h"
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
            return this->lookupFile(file, "/");
        }
        /**
         * @brief Lookup whether a file exists on the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         * @return true if the file is present, or false
         */
        virtual bool lookupFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->lookupFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
        }
        /**
         * @brief Lookup whether a file exists at a location on the storage service (incurs simulated overheads)
         * @param location a location
         * @return true if the file is present, or false
         */
        virtual bool lookupFile(const std::shared_ptr<FileLocation> &location) {
            return this->lookupFile(S4U_Daemon::getRunningActorRecvMailbox(), location);
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
            this->deleteFile(file, "/");
        }
        /**
         * @brief Delete a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         */
        virtual void deleteFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->deleteFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
        }
        /**
         * @brief Delete a file at the storage service (incurs simulated overheads)
         * @param file a location
         */
        virtual void deleteFile(const std::shared_ptr<FileLocation> &location) {
            this->deleteFile(S4U_Daemon::getRunningActorRecvMailbox(), location, true);
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
        static void readFileAtLocation(const std::shared_ptr<FileLocation> &location, double num_bytes) {
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
            this->readFile(file, "/", file->getSize());
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param num_bytes a number of bytes to read
         */
        void readFile(const std::shared_ptr<DataFile> &file, double num_bytes) {
            this->readFile(file, "/", num_bytes);
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         */
        virtual void readFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->readFile(file, path, file->getSize());
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         * @param num_bytes a number of bytes to read
         */
        virtual void readFile(const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes) {
            this->readFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file), num_bytes);
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param location a location
         */
        void readFile(const std::shared_ptr<FileLocation> &location) {
            this->readFile(S4U_Daemon::getRunningActorRecvMailbox(), location, location->getFile()->getSize(), true);
        }
        /**
         * @brief Read a file at the storage service (incurs simulated overheads)
         * @param location a location
         * @param num_bytes a number of bytes to read
         */
        virtual void readFile(const std::shared_ptr<FileLocation> &location, double num_bytes) {
            this->readFile(S4U_Daemon::getRunningActorRecvMailbox(), location, num_bytes, true);
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
            this->writeFile(file, "/");
        }
        /**
         * @brief Write a file at the storage service (incurs simulated overheads)
         * @param file a file
         * @param path a path
         */
        virtual void writeFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->writeFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
        }
        /**
         * @brief Write a file at the storage service (incurs simulated overheads)
         * @param location a location
         */
        virtual void writeFile(const std::shared_ptr<FileLocation> &location) {
            this->writeFile(S4U_Daemon::getRunningActorRecvMailbox(), location, true);
        }

        /** Non-Simulation methods **/

        /** File lookup methods */
        static bool hasFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::hasFileAtLocation(): invalid argument argument");
            }
            return location->getStorageService()->hasFile(location);
        }
        bool hasFile(const std::shared_ptr<DataFile> &file) {
            return this->hasFile(file, "/");
        }
        virtual bool hasFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->hasFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
        }
        virtual bool hasFile(const std::shared_ptr<FileLocation> &location) = 0;

        /** File creation methods */
        static void createFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::createFileAtLocation(): invalid argument argument");
            }
            location->getStorageService()->createFile(location);
        }
        void createFile(const std::shared_ptr<DataFile> &file) {
            this->createFile(file, "/");
        }
        virtual void createFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->createFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
        }
        virtual void createFile(const std::shared_ptr<FileLocation> &location) = 0;

        /** File write date methods */
        void getFileLocationLastWriteDate(const std::shared_ptr<FileLocation> &location) {
            if (location == nullptr) {
                throw std::invalid_argument("StorageService::getFileLocationLastWriteDate(): invalid argument argument");
            }
            this->getFileLastWriteDate(location);
        }
        double getFileLastWriteDate(const std::shared_ptr<DataFile> &file) {
            return this->getFileLastWriteDate(file, "/");
        }
        virtual double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->getFileLastWriteDate(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), path, file));
        }
        virtual double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) = 0;

        /** Service load methods */
        virtual double getLoad() = 0;

        /** Service total space method */
        virtual double getTotalSpace() = 0;

        /** Service free space method */
        double getTotalFreeSpace();

        /** Service free space method */
        virtual double getTotalFreeSpaceAtPath(const std::string &path);

        virtual std::string getBaseRootPath() {
            return "/";
        }

        static void copyFile(const std::shared_ptr<FileLocation> &src_location,
                             const std::shared_ptr<FileLocation> &dst_location);

        static void initiateFileCopy(simgrid::s4u::Mailbox *answer_mailbox,
                                     const std::shared_ptr<FileLocation> &src_location,
                                     const std::shared_ptr<FileLocation> &dst_location);

        static void readFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        static void writeFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);


        virtual bool isBufferized() const = 0;
        virtual double getBufferSize() const = 0;

        virtual bool reserveSpace(std::shared_ptr<FileLocation> &location) = 0;
        virtual void unreserveSpace(std::shared_ptr<FileLocation> &location) = 0;

        /**
         * @brief Determines whether the storage service is a scratch service of a ComputeService
         * @return true if it is, false otherwise
         */
        bool isScratch() const {
            return this->is_scratch;
        }

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        virtual void deleteFile(simgrid::s4u::Mailbox *answer_mailbox,
                                const std::shared_ptr<FileLocation> &location,
                                bool wait_for_answer);

        virtual bool lookupFile(simgrid::s4u::Mailbox *answer_mailbox,
                                const std::shared_ptr<FileLocation> &location);

        virtual void readFile(simgrid::s4u::Mailbox *answer_mailbox,
                              const std::shared_ptr<FileLocation> &location,
                              double num_bytes,
                              bool wait_for_answer);

        virtual void writeFile(simgrid::s4u::Mailbox *answer_mailbox,
                               const std::shared_ptr<FileLocation> &location,
                               bool wait_for_answer);

        virtual void decrementNumRunningOperationsForLocation(const std::shared_ptr<FileLocation> &location) {
            // do nothing
        }

        virtual void incrementNumRunningOperationsForLocation(const std::shared_ptr<FileLocation> &location) {
            // no nothing
        }

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:

        friend class ComputeService;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        StorageService(const std::string &hostname,
                       const std::string &service_name);

        virtual void setIsScratch(bool is_scratch);

        /***********************/
        /** \endcond          **/
        /***********************/

    private:
        enum FileOperation {
            READ,
            WRITE,
        };

        static void writeOrReadFiles(FileOperation action,
                                     std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        bool is_scratch;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_STORAGESERVICE_H
