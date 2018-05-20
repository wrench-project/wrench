/**
 * Copyright (c) 2017. The WRENCH Team.
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
#include "wrench/workflow/execution_events/FailureCause.h"

namespace wrench {

    class Simulation;

    class WorkflowFile;

    class FailureCause;

    class FileRegistryService;

    /**
     * @brief A simulated storage service
     */
    class StorageService : public Service {

    public:

        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        void stop();

        virtual double getFreeSpace();

        double getTotalSpace();

        virtual bool lookupFile(WorkflowFile *file);

        virtual void deleteFile(WorkflowFile *file, FileRegistryService *file_registry_service=nullptr);

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/

        virtual void copyFile(WorkflowFile *file, StorageService *src);

        virtual void initiateFileCopy(std::string answer_mailbox,
                                      WorkflowFile *file,
                                      StorageService *src);

        virtual void readFile(WorkflowFile *file);

        virtual void initiateFileRead(std::string mailbox_that_should_receive_file_content, WorkflowFile *file);

        virtual void writeFile(WorkflowFile *file);

        static void readFiles(std::set<WorkflowFile *> files,
                              std::map<WorkflowFile *, StorageService *> file_locations,
                              StorageService *default_storage_service,
                              std::set<WorkflowFile*>& files_in_scratch);

        static void writeFiles(std::set<WorkflowFile *> files,
                               std::map<WorkflowFile *, StorageService *> file_locations,
                               StorageService *default_storage_service,
                               std::set<WorkflowFile*>& files_in_scratch);

        static void deleteFiles(std::set<WorkflowFile *> files,
                                std::map<WorkflowFile *, StorageService *> file_locations,
                                StorageService *default_storage_service);

        StorageService(std::string hostname,
                       std::string service_name,
                       std::string data_mailbox_name_prefix,
                       double capacity);


    protected:

        friend class Simulation;
        friend class FileRegistryService;

        void stageFile(WorkflowFile *);

        void removeFileFromStorage(WorkflowFile *);

        /** @brief The set of files stored on the storage service */
        std::set<WorkflowFile *> stored_files;
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
                                     StorageService *default_storage_service, std::set<WorkflowFile*>& files_in_scratch);


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICE_H
