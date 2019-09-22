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

#include <wrench/services/Service.h>
#include <wrench/workflow/execution_events/FailureCause.h>
#include <wrench/services/file_registry/FileRegistryService.h>
#include <wrench/workflow/job/StandardJob.h>
#include <services/storage/storage_helpers/LogicalFileSystem.h>
#include <wrench/services/storage/storage_helpers/FileLocation.h>


namespace wrench {

    class Simulation;

    class WorkflowFile;

    class FailureCause;

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

        static bool lookupFile(WorkflowFile *file, std::shared_ptr<FileLocation> location);
        static void deleteFile(WorkflowFile *file, std::shared_ptr<FileLocation> location);
        static void readFile(WorkflowFile *file, std::shared_ptr<FileLocation> location);
        static void writeFile(WorkflowFile *file, std::shared_ptr<FileLocation> location);



        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        static void copyFile(WorkflowFile *file,
                                     std::shared_ptr<FileLocation> src_location,
                                     std::shared_ptr<FileLocation> dst_location);


        virtual void initiateFileCopy(std::string answer_mailbox,
                                      WorkflowFile *file,
                                      std::shared_ptr<FileLocation> src_location,
                                      std::shared_ptr<FileLocation> dst_location);

        static void readFiles(std::set<WorkflowFile *> files,
                              std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations);

        static void writeFiles(std::set<WorkflowFile *> files,
                               std::map<WorkflowFile *, std::shared_ptr<FileLocation>> file_locations);


        StorageService(const std::string &hostname,
                       const std::set<std::string> mount_points,
                       const std::string &service_name,
                       const std::string &data_mailbox_name_prefix);

    protected:

        friend class Simulation;
        friend class FileRegistryService;
        friend class FileTransferThread;

        void stageFile(WorkflowFile *file);
        void stageFile(WorkflowFile *file, std::string dir);
        static void stageFile(WorkflowFile *file , std::shared_ptr<FileLocation> location);

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

        static void writeOrReadFiles(FileOperation action, std::set<WorkflowFile *> files,
                                     std::map<WorkflowFile *, std::tuple<std::shared_ptr<StorageService>, std::string, std::string>> file_locations);



    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICE_H
