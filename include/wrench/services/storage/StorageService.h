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

        void stop() override;

        /** File Lookup methods (in simulation) **/
        static bool lookupFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            return location->getStorageService()->lookupFile(location);
        }
        bool lookupFile(const std::shared_ptr<DataFile> &file) {
            return this->lookupFile(file, "/");
        }
        virtual bool lookupFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->lookupFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file, path));
        }
        virtual bool lookupFile(const std::shared_ptr<FileLocation> &location) {
            return this->lookupFile(S4U_Daemon::getRunningActorRecvMailbox(), location, true);
        }

        /** File deletion methods **/
        static void deleteFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            location->getStorageService()->deleteFile(location);
        }
        virtual void deleteFile(const std::shared_ptr<DataFile> &file) {
            this->deleteFile(file, "/");
        }
        virtual void deleteFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->deleteFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file, path));
        }
        virtual void deleteFile(const std::shared_ptr<FileLocation> &location) {
            this->deleteFile(S4U_Daemon::getRunningActorRecvMailbox(), location, true);
        }

        /** File read methods **/
        static void readFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            location->getStorageService()->readFile(location, location->getFile()->getSize());
        }
        static void readFileAtLocation(const std::shared_ptr<FileLocation> &location, double num_bytes) {
            location->getStorageService()->readFile(location, num_bytes);
        }
        virtual void readFile(const std::shared_ptr<DataFile> &file) {
            this->readFile(file, "/", file->getSize());
        }
        virtual void readFile(const std::shared_ptr<DataFile> &file, double num_bytes) {
            this->readFile(file, "/", num_bytes);
        }
        virtual void readFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->readFile(file, path, file->getSize());
        }
        virtual void readFile(const std::shared_ptr<DataFile> &file, const std::string &path, double num_bytes) {
            this->readFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file, path), num_bytes);
        }
        virtual void readFile(const std::shared_ptr<FileLocation> &location, double num_bytes) {
            this->readFile(S4U_Daemon::getRunningActorRecvMailbox(), location, num_bytes, true);
        }

        /** File write methods **/
        static void writeFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            location->getStorageService()->writeFile(location);
        }
        void writeFile(const std::shared_ptr<DataFile> &file) {
            this->writeFile(file, "/");
        }
        void writeFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->writeFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file, path));
        }
        void writeFile(const std::shared_ptr<FileLocation> &location) {
            this->writeFile(S4U_Daemon::getRunningActorRecvMailbox(), location, true);
        }

        /** Non-Simulation methods **/

        /** File lookup methods */
        bool hasFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            return this->hasFile(location);
        }
        virtual bool hasFile(const std::shared_ptr<DataFile> &file) {
            return this->hasFile(file, "/");
        }
        virtual bool hasFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->hasFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file, path));
        }
        virtual bool hasFile(const std::shared_ptr<FileLocation> &location) = 0;

        /** File creation methods */
        void createFileAtLocation(const std::shared_ptr<FileLocation> &location) {
            this->createFile(location);
        }
        virtual void createFile(const std::shared_ptr<DataFile> &file) {
            this->createFile(file, "/");
        }
        virtual void createFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->createFile(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file, path));
        }
        virtual void createFile(const std::shared_ptr<FileLocation> &location) = 0;

        /** File write date methods */
        void getFileLocationLastWriteDate(const std::shared_ptr<FileLocation> &location) {
            this->getFileLastWriteDate(location);
        }
        virtual double getFileLastWriteDate(const std::shared_ptr<DataFile> &file) {
            return this->getFileLastWriteDate(file, "/");
        }
        virtual double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &path) {
            return this->getFileLastWriteDate(wrench::FileLocation::LOCATION(this->getSharedPtr<StorageService>(), file, path));
        }
        virtual double getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) = 0;

        /** Service load methods */
        virtual double getLoad() = 0;

        /** Service total space method */
        virtual double getTotalSpace() = 0;

        /** Service free space method */
        virtual double getFreeSpace();


        virtual std::string getBaseRootPath() {
            return "/";
        }

        // TODO: BELOW IS UNCLEAR IN TERMS OF THE API REWRITE

        static void copyFile(const std::shared_ptr<FileLocation> &src_location,
                             const std::shared_ptr<FileLocation> &dst_location);


        static void initiateFileCopy(simgrid::s4u::Mailbox *answer_mailbox,
                                     const std::shared_ptr<FileLocation> &src_location,
                                     const std::shared_ptr<FileLocation> &dst_location);

        static void readFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        static void writeFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        bool isBufferized() const {
            return this->is_bufferized;
        }

        /**
         * @brief Determines whether the storage service is a scratch service of a ComputeService
         * @return true if it is, false otherwise
         */
        bool isScratch() const {
            return this->is_scratch;
        }


    protected:

        friend class Simulation;
        friend class FileRegistryService;
        friend class FileTransferThread;
        friend class SimpleStorageServiceNonBufferized;
        friend class SimpleStorageServiceBufferized;
        friend class CompoundStorageService;
        friend class ComputeService;
        friend class Node;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        virtual void deleteFile(simgrid::s4u::Mailbox *answer_mailbox,
                                const std::shared_ptr<FileLocation> &location,
                                bool wait_for_answer);

        virtual bool lookupFile(simgrid::s4u::Mailbox *answer_mailbox,
                                const std::shared_ptr<FileLocation> &location,
                                bool wait_for_answer);

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

        StorageService(const std::string &hostname,
                       const std::string &service_name);

        virtual void setScratch();



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
        bool is_bufferized;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench


#endif//WRENCH_STORAGESERVICE_H
