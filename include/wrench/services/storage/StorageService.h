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
#include "wrench/failure_causes/FailureCause.h"
#include "wrench/services/file_registry/FileRegistryService.h"
#include "wrench/job/StandardJob.h"
#include "wrench/services/storage/storage_helpers/LogicalFileSystem.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"


namespace wrench {

    class Simulation;
    class DataFile;
    class FailureCause;
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

        virtual std::map<std::string, double> getFreeSpace();

        virtual std::map<std::string, double> getTotalSpace();

        std::string getMountPoint();
        std::set<std::string> getMountPoints();
        bool hasMultipleMountPoints();
        bool hasMountPoint(std::string mp);

        static bool lookupFile(std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> location);
        static void deleteFile(std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> location,
                std::shared_ptr<FileRegistryService> file_registry_service = nullptr);
        static void readFile(std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> location);
        static void readFile(std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> location, double num_bytes);
        static void writeFile(std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> location);
        static void createFile(std::shared_ptr<DataFile>file, std::shared_ptr<FileLocation> location);


        /***********************/
        /** \cond INTERNAL    **/
        /***********************/
        bool isScratch();
        void setScratch();

        static void copyFile(std::shared_ptr<DataFile>file,
                                     std::shared_ptr<FileLocation> src_location,
                                     std::shared_ptr<FileLocation> dst_location);


        static void initiateFileCopy(std::string &answer_mailbox,
                                      std::shared_ptr<DataFile>file,
                                      std::shared_ptr<FileLocation> src_location,
                                      std::shared_ptr<FileLocation> dst_location);

        static void readFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);

        static void writeFiles(std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> locations);


        StorageService(const std::string &hostname,
                       const std::set<std::string> mount_points,
                       const std::string &service_name,
                       const std::string &data_mailbox_name_prefix);

    protected:

        friend class Simulation;
        friend class FileRegistryService;
        friend class FileTransferThread;

        static void stageFile(std::shared_ptr<DataFile>file , std::shared_ptr<FileLocation> location);

        /** @brief The service's buffer size */
        unsigned long buffer_size;

        /** @brief File systems */
        std::map<std::string, std::unique_ptr<LogicalFileSystem>> file_systems;

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

        void stageFile(std::shared_ptr<DataFile>file , std::string mountpoint, std::string directory);

        bool is_stratch;


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICE_H
