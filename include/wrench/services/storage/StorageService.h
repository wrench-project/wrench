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
    class FileRegistryService;

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
        static bool lookupFile(const std::shared_ptr<FileLocation> &location) {
            location->getStorageService()->lookupFile(location->getFile(), location->getPath());
        }
        bool lookupFile(const std::shared_ptr<DataFile> &file, const std::string &path = "/") {
            this->lookupFile(S4U_Daemon::getRunningActorRecvMailbox(), file, path);
        }

        /** File deletion methods **/
        static void deleteFile(const std::shared_ptr<FileLocation> &location) {
            location->getStorageService()->deleteFile(location->getFile(), location->getPath());
        }
        void deleteFile(const std::shared_ptr<DataFile> &file, const std::string &path = "/");

        /** File read methods **/
        static void readFile(const std::shared_ptr<FileLocation> &location, double num_bytes) {
            location->getStorageService()->readFile(location->getFile(), num_bytes, location->getPath());
        }
        static void readFile(const std::shared_ptr<FileLocation> &location) {
            StorageService::readFile(location, location->getFile()->getSize());
        }
        void readFile(const std::shared_ptr<DataFile> &file) {
            this->readFile(file, path, file->getSize(), "/");
        }
        void readFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->readFile(file, path, file->getSize(), path);
        }
        void readFile(const std::shared_ptr<DataFile> &file, double num_bytes) {
            this->readFile(file, path, file->getSize(), "/");
        }
        void readFile(const std::shared_ptr<DataFile> &file, double num_bytes, const std::string &path = "/") {
            this->readFile(S4U_Daemon::getRunningActorRecvMailbox(), file, num_bytes, path);
        }

        /** File write methods **/
        static void writeFile(const std::shared_ptr<FileLocation> &location) {
            location->getStorageService()->writeFile(location->getFile(), location->getPath());
        }
        void writeFile(const std::shared_ptr<DataFile> &file) {
            this->writeFile(file, "/");
        }
        void writeFile(const std::shared_ptr<DataFile> &file, const std::string &path) {
            this->writeFile(S4U_Daemon::getRunningActorRecvMailbox(), file, path);
        }

        bool isScratch() const;

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        void lookupFile(simgrid::s4u::Mailbox *answer_mailbox,
                        const std::shared_ptr<DataFile> &file,
                        const std::string &path);

        void readFile(simgrid::s4u::Mailbox *answer_mailbox,
                      const std::shared_ptr<DataFile> &file,
                      double num_bytes,
                      const std::string &path);

        void writeFile(simgrid::s4u::Mailbox *answer_mailbox,
                       const std::shared_ptr<DataFile> &file,
                       const std::string &path);

        void setScratch();

        // TODO: BELOW IS UNCLEAR

        static void copyFile(const std::shared_ptr<FileLocation> &src_location,
                             const std::shared_ptr<FileLocation> &dst_location);


        static void initiateFileCopy(simgrid::s4u::Mailbox *answer_mailbox,
                                     const std::shared_ptr<FileLocation> &src_location,
                                     const std::shared_ptr<FileLocation> &dst_location);

        static void readFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        static void writeFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        StorageService(const std::string &hostname,
                       const std::string &service_name);

    protected:
        friend class Simulation;
        friend class FileRegistryService;
        friend class FileTransferThread;
        friend class SimpleStorageServiceNonBufferized;
        friend class SimpleStorageServiceBufferized;
        friend class CompoundStorageService;

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
