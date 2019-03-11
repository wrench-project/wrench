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
#include <wrench/workflow/job/StandardJob.h>

#include "wrench/services/Service.h"
#include "wrench/workflow/execution_events/FailureCause.h"

namespace wrench {

    class Simulation;

    class WorkflowFile;

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

        virtual double getFreeSpace();

        double getTotalSpace();

        virtual bool lookupFile(WorkflowFile *file);

        virtual bool lookupFile(WorkflowFile *file, std::string dst_dir);

        virtual void deleteFile(WorkflowFile *file, FileRegistryService *file_registry_service=nullptr);

        virtual void deleteFile(WorkflowFile *file, std::string dst_dir, FileRegistryService *file_registry_service=nullptr);

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/


        virtual void readFile(WorkflowFile *file, std::string src_dir);

        virtual void writeFile(WorkflowFile *file, std::string dst_dir);

        virtual void deleteFile(WorkflowFile *file, WorkflowJob* job, FileRegistryService *file_registry_service=nullptr);

        virtual bool lookupFile(WorkflowFile *file, WorkflowJob*);

        virtual void copyFile(WorkflowFile *file, StorageService *src, std::string src_dir, std::string dst_dir);

        virtual void copyFile(WorkflowFile *file, StorageService *src);

        virtual void copyFile(WorkflowFile *file, StorageService *src, WorkflowJob* src_job, WorkflowJob* dst_job);

        virtual void initiateFileCopy(std::string answer_mailbox,
                                      WorkflowFile *file,
                                      StorageService *src,
                                      std::string src_dir,
                                      std::string dst_dir);

        virtual void readFile(WorkflowFile *file);

        virtual void readFile(WorkflowFile *file, WorkflowJob* job);

        virtual void initiateFileRead(std::string mailbox_that_should_receive_file_content, WorkflowFile *file, std::string src_dir);

        virtual void writeFile(WorkflowFile *file);

        virtual void writeFile(WorkflowFile *file, WorkflowJob* job);

        static void readFiles(std::set<WorkflowFile *> files,
                              std::map<WorkflowFile *, StorageService *> file_locations,
                              StorageService *default_storage_service,
                              std::set<WorkflowFile*>& files_in_scratch,
                              WorkflowJob* job = nullptr);

        static void writeFiles(std::set<WorkflowFile *> files,
                               std::map<WorkflowFile *, StorageService *> file_locations,
                               StorageService *default_storage_service,
                               std::set<WorkflowFile*>& files_in_scratch,
                               WorkflowJob* job = nullptr);

        static void deleteFiles(std::set<WorkflowFile *> files,
                                std::map<WorkflowFile *, StorageService *> file_locations,
                                StorageService *default_storage_service);

        StorageService(const std::string &hostname,
                       const std::string &service_name,
                       const std::string &data_mailbox_name_prefix,
                       double capacity);


    protected:

        friend class Simulation;
        friend class FileRegistryService;


        void stageFile(WorkflowFile *);

        void removeFileFromStorage(WorkflowFile *, std::string);

        /** @brief The map of file directories and the set of files stored on those directories inside the storage service */
        std::map<std::string, std::set<WorkflowFile *>> stored_files;
        /** @brief The storage service's capacity */
        double capacity;
        /** @brief The storage service's occupied space */
        double occupied_space = 0;

        /***********************/
        /** \endcond          **/
        /***********************/

    private:

        enum FileOperation {
            READ,
            WRITE,
        };

        static void writeOrReadFiles(FileOperation action, std::set<WorkflowFile *> files,
                                     std::map<WorkflowFile *, StorageService *> file_locations,
                                     StorageService *default_storage_service, std::set<WorkflowFile*>& files_in_scratch,
                                     WorkflowJob* job);


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICE_H
